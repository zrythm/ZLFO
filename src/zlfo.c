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

#include "lv2/atom/forge.h"
#include "lv2/log/log.h"
#include "lv2/core/lv2.h"

#define math_floats_equal(a,b) \
  (a > b ? \
   (a - b) < 0.0001f : \
   (b - a) < 0.0001f)

static const float PI = (float) M_PI;

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

  /** Transport speed (0.0 is stopped, 1.0 is
   * normal playback, -1.0 is reverse playback,
   * etc.). */
  float         speed;

  float         bpm;

  /** Frames per beat. */
  float         frames_per_beat;

  int           beat_unit;

  /** This is how far we are inside a beat, from 0.0
   * to 1.0. */
  float         beat_offset;

  /**
   * Effective frequency.
   *
   * This is either the free-running frequency,
   * or the frequency corresponding to the current
   * sync rate.
   */
  float         effective_freq;

  /* --- values in the last run --- */

  /** Frequency during the last run. */
  float         last_freq;
  float         last_sync_rate;
  float         last_sync_rate_type;

  /**
   * Whether the plugin was freerunning in the
   * last cycle.
   *
   * This is used to detect changes in freerunning/
   * sync.
   */
  int           was_freerunning;

  /** Plugin samplerate. */
  double        samplerate;

  /** Size of 1 LFO period in samples. */
  long          period_size;

  /**
   * Current sample index in the period.
   *
   * This should be sent to the UI.
   */
  long          current_sample;

  /** Global current sample in the host. */
  long          host_current_sample;

  /** Atom forge. */
  LV2_Atom_Forge forge;
  LV2_Atom_Forge_Frame notify_frame;

  /** Log feature. */
  LV2_Log_Log *        log;

  /** Map feature. */
  LV2_URID_Map *       map;

  ZLfoUris      uris;

  /**
   * Sine multiplier.
   *
   * This is a pre-calculated variable that is used
   * when calculating the sine value.
   */
  float         sine_multiplier;

  float         saw_up_multiplier;

  /** Whether the UI is active or not. */
  int           ui_active;

} ZLFO;

static LV2_Handle
instantiate (
  const LV2_Descriptor*     descriptor,
  double                    rate,
  const char*               bundle_path,
  const LV2_Feature* const* features)
{
  ZLFO * self = calloc (1, sizeof (ZLFO));

  self->samplerate = rate;

#define HAVE_FEATURE(x) \
  (!strcmp(features[i]->URI, x))

  for (int i = 0; features[i]; ++i)
    {
      if (HAVE_FEATURE (LV2_URID__map))
        {
          self->map =
            (LV2_URID_Map*) features[i]->data;
        }
      else if (HAVE_FEATURE (LV2_LOG__log))
        {
          self->log =
            (LV2_Log_Log *) features[i]->data;
        }
    }
#undef HAVE_FEATURE

  if (!self->map)
    {
      fprintf (stderr, "Missing feature urid:map\n");
      return NULL;
    }

  /* map uris */
  map_uris (self->map, &self->uris);

  lv2_atom_forge_init (&self->forge, self->map);

  return (LV2_Handle) self;
}

/**
 * Gets the current frequency.
 */
static float
get_freq (
  ZLFO * self)
{
  if (*self->freerun > 0.f)
    {
      return *self->freq;
    }
  else
    {
      /* if host does not send position info,
       * send frequency back instead */
      if (self->beat_unit == 0)
        {
          log_error (
            self->log, &self->uris,
            "Have not received time info from host "
            "yet. Beat unit is unknown.");
          return * self->freq;
        }

      /* bpm / (60 * BU * sync note) */
      float sync_note =
        sync_rate_to_float (
          (SyncRate) *self->sync_rate,
          (SyncRateType) *self->sync_rate_type);
      return
        self->bpm /
        (60.f * self->beat_unit * sync_note);
    }
}

static void
recalc_multipliers (
  ZLFO * self)
{
  /* no ports connected yet */
  if (!self->freerun)
    return;

  self->effective_freq = get_freq (self);

  /*
   * F = frequency
   * X = samples processed
   * SR = sample rate
   *
   * First, get the radians.
   * ? radians =
   *   (2 * PI) radians per LFO cycle *
   *   F cycles per second *
   *   (1 / SR samples per second) *
   *   X samples
   *
   * Then the LFO value is the sine of
   * (radians % (2 * PI)).
   *
   * This multiplier handles the part known by now
   * and the first part of the calculation should
   * become:
   * ? radians = X samples * sine_multiplier
   */
  self->sine_multiplier =
    (self->effective_freq /
     (float) self->samplerate) *
    2.f * PI;

  /*
   * F = frequency
   * X = samples processed
   * SR = sample rate
   *
   * First, get the value.
   * ? value =
   *   (1 value per LFO cycle *
   *    F cycles per second *
   *    1 / SR samples per second *
   *    X samples) % 1
   *
   * Then the LFO value is value * 2 - 1 (to make
   * it start from -1 and end at 1).
   *
   * This multiplier handles the part known by now and the
   * first part of the calculation should become:
   * ? value = ((X samples * saw_multiplier) % 1) * 2 - 1
   */
  self->saw_up_multiplier =
    (self->effective_freq /
     (float) self->samplerate);

  if (*self->freerun > 0.0001f) /* freerunning */
    {
      self->period_size =
        (uint32_t)
        ((float) self->samplerate /
         self->effective_freq);
      self->current_sample = 0;
    }
  else /* synced */
    {
      if (self->beat_unit == 0)
        {
          /* set reasonable values if host does not
           * send time info */
          log_error (
            self->log, &self->uris,
            "Host did not send time info. Beat "
            "unit is unknown.");
          self->period_size =
            (uint32_t)
            ((float) self->samplerate /
             self->effective_freq);
          self->current_sample = 0;
        }
      else
        {
          self->period_size =
            (uint32_t)
            (self->frames_per_beat *
            self->beat_unit *
            sync_rate_to_float (
              *self->sync_rate,
              *self->sync_rate_type));
          self->current_sample =
            (self->host_current_sample %
                self->period_size);
        }
    }
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
send_messages_to_ui (
  ZLFO * self)
{
  /* set up forge to write directly to notify
   * output port */
  const uint32_t notify_capacity =
    self->notify->atom.size;
  lv2_atom_forge_set_buffer (
    &self->forge, (uint8_t*) self->notify,
    notify_capacity);

  /* start a sequence in the notify output port */
  lv2_atom_forge_sequence_head (
    &self->forge, &self->notify_frame, 0);

  /* forge container object of type "ui_state" */
  lv2_atom_forge_frame_time (&self->forge, 0);
  LV2_Atom_Forge_Frame frame;
  lv2_atom_forge_object (
    &self->forge, &frame, 1,
    self->uris.ui_state);

  /* append property for current sample */
  lv2_atom_forge_key (
    &self->forge,
    self->uris.ui_state_current_sample);
  lv2_atom_forge_long (
    &self->forge, self->current_sample);

  /* append property for period size */
  lv2_atom_forge_key (
    &self->forge,
    self->uris.ui_state_period_size);
  lv2_atom_forge_long (
    &self->forge, self->period_size);

  /* append samplerate */
  lv2_atom_forge_key (
    &self->forge,
    self->uris.ui_state_samplerate);
  lv2_atom_forge_double (
    &self->forge, self->samplerate);

  /* finish object */
  lv2_atom_forge_pop (&self->forge, &frame);
}

static void
activate (
  LV2_Handle instance)
{
  ZLFO * self = (ZLFO*) instance;

  recalc_multipliers (self);
}

static void
update_position (
  ZLFO *                  self,
  const LV2_Atom_Object * obj)
{
  ZLfoUris * uris = &self->uris;

  /* Received new transport position/speed */
  LV2_Atom *beat = NULL,
           *bpm = NULL,
           *beat_unit = NULL,
           *speed = NULL,
           *frame = NULL;
  lv2_atom_object_get (
    obj, uris->time_barBeat, &beat,
    uris->time_beatUnit, &beat_unit,
    uris->time_beatsPerMinute, &bpm,
    uris->time_frame, &frame,
    uris->time_speed, &speed, NULL);
  if (bpm && bpm->type == uris->atom_Float)
    {
      /* Tempo changed, update BPM */
      self->bpm = ((LV2_Atom_Float*)bpm)->body;
     }
  if (speed && speed->type == uris->atom_Float)
    {
      /* Speed changed, e.g. 0 (stop) to 1 (play) */
      self->speed =
        ((LV2_Atom_Float *) speed)->body;
    }
  if (beat_unit && beat_unit->type == uris->atom_Int)
    {
      self->beat_unit =
        ((LV2_Atom_Int *) beat_unit)->body;
    }
  if (frame && frame->type == uris->atom_Long)
    {
      self->host_current_sample =
        ((LV2_Atom_Int *) frame)->body;
    }
  if (beat && beat->type == uris->atom_Float)
    {
      self->frames_per_beat =
        60.0f / self->bpm * (float) self->samplerate;
      const float bar_beats =
        ((LV2_Atom_Float *) beat)->body;
      self->beat_offset = fmodf (bar_beats, 1.f);
    }
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
            &self->forge, ev->body.type))
        {
          const LV2_Atom_Object * obj =
            (const LV2_Atom_Object*)&ev->body;
          if (obj->body.otype ==
                self->uris.time_Position)
            {
              update_position (self, obj);
              xport_changed = 1;
            }
          else if (obj->body.otype ==
                     self->uris.ui_on)
            {
              self->ui_active = 1;
              fprintf (stderr, "UI IS ACTIVE\n");
            }
          else if (obj->body.otype ==
                     self->uris.ui_off)
            {
              self->ui_active = 0;
              fprintf (stderr, "UI IS OFF\n");
            }
        }
    }

  int freq_changed =
    !math_floats_equal (
      self->last_freq, *self->freq);
  int is_freerunning = *self->freerun > 0.0001f;
  int sync_or_freerun_mode_changed =
    self->was_freerunning &&
    !is_freerunning;
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
  uint32_t rnd_step = n_samples < 16 ? 1 : n_samples / 16;
  /*uint32_t prev_rnd_x = 0;*/
  uint32_t rnd_x = 0;

  for (uint32_t i = 0; i < n_samples; i++)
    {
      /* invert horizontally */
      long shifted_current_sample =
        self->current_sample;
      if (*self->hinvert >= 0.01f)
        {
          shifted_current_sample =
            self->period_size - self->current_sample;
          if (shifted_current_sample ==
                self->period_size)
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
            (self->period_size / 2.f));

          /* readjust */
          shifted_current_sample =
            shifted_current_sample %
              self->period_size;
        }
      else
        {
          /* subtract the samples to shift between
           * 0 and (half the period width) */
          shifted_current_sample -=
            (long)
            /* shift ratio */
            ((*self->shift * 2.f) *
            /* half a period */
            (self->period_size / 2.f));

          /* readjust */
          while (shifted_current_sample < 0)
            {
              shifted_current_sample +=
                self->period_size;
            }
        }

      /* calculate sine */
      self->sine_out[i] =
        sinf (
          ((float) shifted_current_sample *
              self->sine_multiplier));

      /* calculate saw */
      self->saw_out[i] =
        fmodf (
          (float) shifted_current_sample *
            self->saw_up_multiplier, 1.f) * 2.f -
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
          (!is_freerunning && self->speed >
             0.00001f))
        {
          self->current_sample++;
        }
      if (self->current_sample ==
            self->period_size)
        self->current_sample = 0;
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
      send_messages_to_ui (self);
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
