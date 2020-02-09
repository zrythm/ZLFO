/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZLFO
 *
 * ZLFO is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZLFO is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with ZLFO.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "zlfo_common.h"
#include "zlfo_math.h"

#define math_floats_equal(a,b) \
  (a > b ? \
   (a - b) < 0.0001f : \
   (b - a) < 0.0001f)

#define IS_FREERUN(x) (*x->freerun > 0.001f)
#define IS_STEP_MODE(x) (*x->step_mode > 0.001f)
#define IS_TRIGGERED(x) (*x->trigger > 0.001f)

typedef struct ZLFO
{
  /** Plugin ports. */
  const LV2_Atom_Sequence* control;
  LV2_Atom_Sequence* notify;
  const float * gate;
  const float * trigger;
  const float * cv_gate;
  const float * cv_trigger;
  const float * freq;
  const float * shift;
  const float * range_min;
  const float * range_max;
  const float * step_mode;
  const float * freerun;
  const float * grid_step;
  const float * sync_rate;
  const float * sync_rate_type;
  const float * hinvert;
  const float * vinvert;
  const float * nodes[ZLFO_NUM_NODES][3];

  /* outputs */
  float *       cv_out;
  float *       sine_out;
  float *       saw_out;
  float *       triangle_out;
  float *       square_out;
  float *       rnd_out;
  float *       custom_out;

  /** This is how far we are inside a beat, from 0.0
   * to 1.0. */
  /*float         beat_offset;*/

  ZLfoCommon    common;

  /* FIXME this can be a local variable */
  LV2_Atom_Forge_Frame notify_frame;

  /** Whether the UI is active or not. */
  int           ui_active;

  /** Temporary variables. */

  /* whether the plugin was freerunning in the
   * last cycle. this is used to detect changes
   * in freerunning/sync. */
  int was_freerunning;

  /** Frequency during the last run. */
  float last_freq;
  float last_sync_rate;
  float last_sync_rate_type;

} ZLFO;

static LV2_Handle
instantiate (
  const LV2_Descriptor*     descriptor,
  double                    rate,
  const char*               bundle_path,
  const LV2_Feature* const* features)
{
  ZLFO * self = calloc (1, sizeof (ZLFO));

  self->common.samplerate = rate;

#define HAVE_FEATURE(x) \
  (!strcmp(features[i]->URI, x))

  for (int i = 0; features[i]; ++i)
    {
      if (HAVE_FEATURE (LV2_URID__map))
        {
          self->common.map =
            (LV2_URID_Map*) features[i]->data;
        }
      else if (HAVE_FEATURE (LV2_LOG__log))
        {
          self->common.log =
            (LV2_Log_Log *) features[i]->data;
        }
    }
#undef HAVE_FEATURE

  if (!self->common.map)
    {
      fprintf (stderr, "Missing feature urid:map\n");
      return NULL;
    }

  /* map uris */
  map_uris (self->common.map, &self->common.uris);

  /* init atom forge */
  lv2_atom_forge_init (
    &self->common.forge, self->common.map);

  return (LV2_Handle) self;
}

static void
recalc_multipliers (
  ZLFO * self)
{
  /* no ports connected yet */
  if (!self->freerun)
    return;

  float sync_rate_float =
    sync_rate_to_float (
      *self->sync_rate,
      *self->sync_rate_type);

  /**
   * Effective frequency.
   *
   * This is either the free-running frequency,
   * or the frequency corresponding to the current
   * sync rate.
   */
  float effective_freq =
    get_effective_freq (
      IS_FREERUN (self), *self->freq,
      &self->common.host_pos, sync_rate_float);

  recalc_vars (
    IS_FREERUN (self),
    &self->common.sine_multiplier,
    &self->common.saw_multiplier,
    &self->common.period_size,
    &self->common.current_sample,
    &self->common.host_pos, effective_freq,
    sync_rate_float,
    (float) self->common.samplerate);
}

static void
connect_port (
  LV2_Handle instance,
  uint32_t   port,
  void *     data)
{
  ZLFO * self = (ZLFO*) instance;

  switch ((PortIndex) port)
    {
    case ZLFO_CONTROL:
      self->control =
        (const LV2_Atom_Sequence *) data;
      break;
    case ZLFO_NOTIFY:
      self->notify =
        (LV2_Atom_Sequence *) data;
      break;
    case ZLFO_CV_GATE:
      self->cv_gate = (const float *) data;
      break;
    case ZLFO_CV_TRIGGER:
      self->cv_trigger = (const float *) data;
      break;
    case ZLFO_GATE:
      self->gate = (const float *) data;
      break;
    case ZLFO_TRIGGER:
      self->trigger = (const float *) data;
      break;
    case ZLFO_FREQ:
      self->freq = (const float *) data;
      break;
    case ZLFO_SHIFT:
      self->shift = (const float *) data;
      break;
    case ZLFO_RANGE_MIN:
      self->range_min = (const float *) data;
      break;
    case ZLFO_RANGE_MAX:
      self->range_max = (const float *) data;
      break;
    case ZLFO_STEP_MODE:
      self->step_mode = (const float *) data;
      break;
    case ZLFO_FREE_RUNNING:
      self->freerun = (const float *) data;
      break;
    case ZLFO_GRID_STEP:
      self->grid_step = (const float *) data;
      break;
    case ZLFO_SYNC_RATE:
      self->sync_rate = (const float *) data;
      break;
    case ZLFO_SYNC_RATE_TYPE:
      self->sync_rate_type = (const float *) data;
      break;
    case ZLFO_HINVERT:
      self->hinvert = (const float *) data;
      break;
    case ZLFO_VINVERT:
      self->vinvert = (const float *) data;
      break;
    case ZLFO_SINE_OUT:
      self->sine_out = (float *) data;
      break;
    case ZLFO_SAW_OUT:
      self->saw_out = (float *) data;
      break;
    case ZLFO_TRIANGLE_OUT:
      self->triangle_out = (float *) data;
      break;
    case ZLFO_SQUARE_OUT:
      self->square_out = (float *) data;
      break;
    case ZLFO_RND_OUT:
      self->rnd_out = (float *) data;
      break;
    case ZLFO_CUSTOM_OUT:
      self->custom_out = (float *) data;
      break;
    default:
      break;
    }

  if (port >= ZLFO_NODE_1_POS &&
      port <= ZLFO_NODE_16_CURVE)
    {
      unsigned int prop =
        (port - ZLFO_NODE_1_POS) % 3;
      unsigned int node_id =
        (port - ZLFO_NODE_1_POS) / 3;
      self->nodes[node_id][prop] =
        (const float *) data;
    }
}

static void
send_position_to_ui (
  ZLFO *  self)
{
  /* forge container object of type time_Position */
  lv2_atom_forge_frame_time (&self->common.forge, 0);
  LV2_Atom_Forge_Frame frame;
  lv2_atom_forge_object (
    &self->common.forge, &frame, 0,
    self->common.uris.time_Position);

  /* append property for bpm */
  lv2_atom_forge_key (
    &self->common.forge,
    self->common.uris.time_beatsPerMinute);
  lv2_atom_forge_float (
    &self->common.forge, self->common.host_pos.bpm);

  /* append property for current sample */
  lv2_atom_forge_key (
    &self->common.forge,
    self->common.uris.time_frame);
  lv2_atom_forge_long (
    &self->common.forge,
    self->common.host_pos.frame);

  /* append speed */
  lv2_atom_forge_key (
    &self->common.forge,
    self->common.uris.time_speed);
  lv2_atom_forge_float (
    &self->common.forge,
    self->common.host_pos.speed);

  /* append beat unit */
  lv2_atom_forge_key (
    &self->common.forge,
    self->common.uris.time_beatUnit);
  lv2_atom_forge_int (
    &self->common.forge,
    self->common.host_pos.beat_unit);

  /* finish object */
  lv2_atom_forge_pop (&self->common.forge, &frame);
}

static void
send_messages_to_ui (
  ZLFO * self,
  int    send_position)
{
  /* set up forge to write directly to notify
   * output port */
  const uint32_t notify_capacity =
    self->notify->atom.size;
  lv2_atom_forge_set_buffer (
    &self->common.forge, (uint8_t*) self->notify,
    notify_capacity);

  /* start a sequence in the notify output port */
  lv2_atom_forge_sequence_head (
    &self->common.forge, &self->notify_frame, 0);

  /* forge container object of type "ui_state" */
  lv2_atom_forge_frame_time (&self->common.forge, 0);
  LV2_Atom_Forge_Frame frame;
  lv2_atom_forge_object (
    &self->common.forge, &frame, 0,
    self->common.uris.ui_state);

  /* append property for current sample */
  lv2_atom_forge_key (
    &self->common.forge,
    self->common.uris.ui_state_current_sample);
  lv2_atom_forge_long (
    &self->common.forge,
    self->common.current_sample);

  /* append property for period size */
  lv2_atom_forge_key (
    &self->common.forge,
    self->common.uris.ui_state_period_size);
  lv2_atom_forge_long (
    &self->common.forge, self->common.period_size);

  /* append samplerate */
  lv2_atom_forge_key (
    &self->common.forge,
    self->common.uris.ui_state_samplerate);
  lv2_atom_forge_double (
    &self->common.forge, self->common.samplerate);

  /* append sine multiplier */
  lv2_atom_forge_key (
    &self->common.forge,
    self->common.uris.ui_state_sine_multiplier);
  lv2_atom_forge_float (
    &self->common.forge,
    self->common.sine_multiplier);

  /* append saw multiplier */
  lv2_atom_forge_key (
    &self->common.forge,
    self->common.uris.ui_state_saw_multiplier);
  lv2_atom_forge_float (
    &self->common.forge,
    self->common.saw_multiplier);

  /* finish object */
  lv2_atom_forge_pop (&self->common.forge, &frame);

  if (send_position)
    {
      send_position_to_ui (self);
    }
}

static void
activate (
  LV2_Handle instance)
{
  ZLFO * self = (ZLFO*) instance;

  recalc_multipliers (self);
}

static void
run (
  LV2_Handle instance,
  uint32_t n_samples)
{
  ZLFO * self = (ZLFO *) instance;

  int xport_changed = 0;

  /* read incoming events from host and UI */
  LV2_ATOM_SEQUENCE_FOREACH (
    self->control, ev)
    {
      if (lv2_atom_forge_is_object_type (
            &self->common.forge, ev->body.type))
        {
          const LV2_Atom_Object * obj =
            (const LV2_Atom_Object*)&ev->body;
          if (obj->body.otype ==
                self->common.uris.time_Position)
            {
              update_position_from_atom_obj (
                &self->common.host_pos,
                &self->common.uris, obj);
              xport_changed = 1;
            }
          else if (obj->body.otype ==
                     self->common.uris.ui_on)
            {
              self->ui_active = 1;
            }
          else if (obj->body.otype ==
                     self->common.uris.ui_off)
            {
              self->ui_active = 0;
            }
        }
    }

  int freq_changed =
    !math_floats_equal (
      self->last_freq, *self->freq);
  int is_freerunning = *self->freerun > 0.0001f;
  int sync_or_freerun_mode_changed =
    self->was_freerunning && !is_freerunning;
  int sync_rate_changed =
    !(math_floats_equal (
      self->last_sync_rate, *self->sync_rate) &&
    math_floats_equal (
      self->last_sync_rate_type,
      *self->sync_rate_type));

  /* if freq or transport changed, reset the
   * multipliers */
  if (xport_changed || freq_changed ||
      sync_rate_changed ||
      sync_or_freerun_mode_changed)
    {
#if 0
      fprintf (
        stderr, "xport %d freq %d sync %d\n",
        xport_changed, freq_changed,
        sync_or_freerun_mode_changed);
#endif
      recalc_multipliers (self);
    }

  float max_range =
    MAX (*self->range_max, *self->range_min);
  float min_range =
    MIN (*self->range_max, *self->range_min);
  float range = max_range - min_range;

  /* for random out */
  float rnd_point =
    ((float) rand () / (float) ((float) RAND_MAX / 2.f)) -
      1.f;
  float prev_rnd_point = 0.f;
  float m = 0.f;
  uint32_t rnd_step =
    n_samples < 16 ? 1 : n_samples / 16;
  /*uint32_t prev_rnd_x = 0;*/
  uint32_t rnd_x = 0;

  float grid_step_divisor =
    (float)
    grid_step_to_divisor (
      (GridStep) *self->grid_step);
  long step_frames =
    (long)
    ((float) self->common.period_size /
     grid_step_divisor);

  /* handle triggers
   *
   * FIXME CV trigger requires splitting the cycle,
   * but for now it applies to the whole cycle */
  if (IS_TRIGGERED (self) ||
      float_array_contains_nonzero (
        self->cv_trigger, n_samples))
    {
      self->common.current_sample = 0;
    }

  for (uint32_t i = 0; i < n_samples; i++)
    {
      /* invert horizontally */
      long shifted_current_sample =
        self->common.current_sample;
      if (*self->hinvert >= 0.01f)
        {
          shifted_current_sample =
            self->common.period_size -
            self->common.current_sample;
          if (shifted_current_sample ==
                self->common.period_size)
            shifted_current_sample = 0;
        }

      /* shift */
      if (*self->shift >= 0.5f)
        {
          /* add the samples to shift from 0 to
           * (half the period width) */
          shifted_current_sample +=
            (long)
            /* shift ratio */
            (((*self->shift - 0.5f) * 2.f) *
            /* half a period */
            (self->common.period_size / 2.f));

          /* readjust */
          shifted_current_sample =
            shifted_current_sample %
              self->common.period_size;
        }
      else
        {
          /* subtract the samples to shift between
           * 0 and (half the period width) */
          shifted_current_sample -=
            (long)
            /* shift ratio */
            (((0.5f - *self->shift) * 2.f) *
            /* half a period */
            (self->common.period_size / 2.f));

          /* readjust */
          while (shifted_current_sample < 0)
            {
              shifted_current_sample +=
                self->common.period_size;
            }
        }

      if (IS_STEP_MODE (self))
        {
          /* find closest step and set the current
           * sample to the middle of it */
          shifted_current_sample =
            (shifted_current_sample / step_frames) *
              step_frames +
            step_frames / 2;
        }

      /* calculate sine */
      self->sine_out[i] =
        sinf (
          ((float) shifted_current_sample *
              self->common.sine_multiplier));

      /* calculate saw */
      self->saw_out[i] =
        fmodf (
          (float) shifted_current_sample *
            self->common.saw_multiplier, 1.f) * 2.f -
        1.f;
      self->saw_out[i] = - self->saw_out[i];

      /* triangle can be calculated based on the
       * saw */
      if (self->saw_out[i] > 0.f)
        self->triangle_out[i] =
          ((- self->saw_out[i]) + 1.f) * 2.f - 1.f;
      else
        self->triangle_out[i] =
          (self->saw_out[i] + 1.f) * 2.f - 1.f;

      /* square too */
      self->square_out[i] =
        self->saw_out[i] < 0.f ? -1.f : 1.f;

      /* for random, calculate 16 random points and
       * connect them with straight lines */
      /* FIXME this is not working properly */
      if (i % rnd_step == 0)
        {
          prev_rnd_point = rnd_point;
          /*prev_rnd_x = rnd_x;*/
          rnd_point =
            ((float) rand () /
             (float) ((float) RAND_MAX / 2.f)) -
              1.f;
          rnd_x = i / rnd_step;

          /* get slope */
          m =
            (rnd_point - prev_rnd_point) / (rnd_step);
        }
      self->rnd_out[i] = m * (i - rnd_x) + rnd_point;


      /* invert vertically */
      if (*self->vinvert >= 0.01f)
        {
#define INVERT(x) \
  self->x##_out[i] = - self->x##_out[i]

          INVERT (sine);
          INVERT (saw);
          INVERT (triangle);
          INVERT (square);
          INVERT (rnd);
          INVERT (custom);

#undef INVERT
        }

      /* adjust range */
#define ADJUST_RANGE(x) \
  self->x##_out[i] = \
    min_range + \
    ((self->x##_out[i] + 1.f) / 2.f) * range

      ADJUST_RANGE (sine);
      ADJUST_RANGE (saw);
      ADJUST_RANGE (triangle);
      ADJUST_RANGE (square);
      ADJUST_RANGE (rnd);
      ADJUST_RANGE (custom);

#undef ADJUST_RANGE

      if (is_freerunning ||
          (!is_freerunning &&
           self->common.host_pos.speed >
             0.00001f))
        {
          self->common.current_sample++;
        }
      if (self->common.current_sample ==
            self->common.period_size)
        self->common.current_sample = 0;
    }
#if 0
  fprintf (
    stderr, "current sample %ld, "
    "period size%ld\n",
    self->current_sample, self->period_size);
#endif

  /* remember values */
  self->last_freq = *self->freq;
  self->last_sync_rate = *self->sync_rate;
  self->last_sync_rate_type = *self->sync_rate_type;
  self->was_freerunning = is_freerunning;

  if (self->ui_active)
    {
      send_messages_to_ui (self, xport_changed);
    }
}

static void
deactivate (
  LV2_Handle instance)
{
}

static void
cleanup (
  LV2_Handle instance)
{
  free (instance);
}

static const void*
extension_data (
  const char* uri)
{
  return NULL;
}

static const LV2_Descriptor descriptor = {
  LFO_URI,
  instantiate,
  connect_port,
  activate,
  run,
  deactivate,
  cleanup,
  extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor (
  uint32_t index)
{
  switch (index)
    {
    case 0:
      return &descriptor;
    default:
      return NULL;
    }
}
