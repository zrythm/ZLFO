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

/**
 * \file
 *
 * Common code for both the DSP and the UI.
 */

#ifndef __Z_LFO_COMMON_H__
#define __Z_LFO_COMMON_H__

#include "config.h"

#include <string.h>

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/core/lv2.h"
#include "lv2/log/log.h"
#include "lv2/urid/urid.h"
#include "lv2/time/time.h"

/** Min, max and default frequency. */
#define MIN_FREQ 0.1f
#define DEF_FREQ 1.f
#define MAX_FREQ 20.f

typedef struct ZLfoUris
{
  LV2_URID atom_eventTransfer;
  LV2_URID atom_Blank;
  LV2_URID atom_Object;
  LV2_URID atom_Float;
  LV2_URID atom_Double;
  LV2_URID atom_Int;
  LV2_URID atom_Long;
  LV2_URID log_Entry;
  LV2_URID log_Error;
  LV2_URID log_Note;
  LV2_URID log_Trace;
  LV2_URID log_Warning;
  LV2_URID time_Position;
  LV2_URID time_bar;
  LV2_URID time_barBeat;
  LV2_URID time_beatsPerMinute;
  LV2_URID time_beatUnit;
  LV2_URID time_frame;
  LV2_URID time_speed;

  /* custom URIs for communication */

  /** The object URI. */
  LV2_URID ui_state;

  /* object property URIs */
  LV2_URID ui_state_current_sample;
  LV2_URID ui_state_period_size;
  LV2_URID ui_state_samplerate;
  LV2_URID ui_state_saw_multiplier;
  LV2_URID ui_state_sine_multiplier;

  /** Messages for UI on/off. */
  LV2_URID ui_on;
  LV2_URID ui_off;
} ZLfoUris;

typedef enum PortIndex
{
  /** GUI to plugin communication. */
  ZLFO_CONTROL,
  /** Plugin to UI communication. */
  ZLFO_NOTIFY,
  /** Plugin to UI communication of the current
   * sample. */
  ZLFO_SAMPLE_TO_UI,

  ZLFO_CV_GATE,
  ZLFO_CV_TRIGGER,
  ZLFO_GATE,
  ZLFO_TRIGGER,
  ZLFO_GATED_MODE,
  ZLFO_SYNC_RATE,
  ZLFO_SYNC_RATE_TYPE,
  ZLFO_FREQ,
  ZLFO_SHIFT,
  ZLFO_RANGE_MIN,
  ZLFO_RANGE_MAX,
  ZLFO_STEP_MODE,
  ZLFO_FREE_RUNNING,
  ZLFO_GRID_STEP,
  ZLFO_HINVERT,
  ZLFO_VINVERT,
  ZLFO_SINE_TOGGLE,
  ZLFO_SAW_TOGGLE,
  ZLFO_SQUARE_TOGGLE,
  ZLFO_TRIANGLE_TOGGLE,
  ZLFO_CUSTOM_TOGGLE,
  ZLFO_NODE_1_POS,
  ZLFO_NODE_1_VAL,
  ZLFO_NODE_1_CURVE,
  ZLFO_NODE_2_POS,
  ZLFO_NODE_2_VAL,
  ZLFO_NODE_2_CURVE,
  ZLFO_NODE_3_POS,
  ZLFO_NODE_3_VAL,
  ZLFO_NODE_3_CURVE,
  ZLFO_NODE_4_POS,
  ZLFO_NODE_4_VAL,
  ZLFO_NODE_4_CURVE,
  ZLFO_NODE_5_POS,
  ZLFO_NODE_5_VAL,
  ZLFO_NODE_5_CURVE,
  ZLFO_NODE_6_POS,
  ZLFO_NODE_6_VAL,
  ZLFO_NODE_6_CURVE,
  ZLFO_NODE_7_POS,
  ZLFO_NODE_7_VAL,
  ZLFO_NODE_7_CURVE,
  ZLFO_NODE_8_POS,
  ZLFO_NODE_8_VAL,
  ZLFO_NODE_8_CURVE,
  ZLFO_NODE_9_POS,
  ZLFO_NODE_9_VAL,
  ZLFO_NODE_9_CURVE,
  ZLFO_NODE_10_POS,
  ZLFO_NODE_10_VAL,
  ZLFO_NODE_10_CURVE,
  ZLFO_NODE_11_POS,
  ZLFO_NODE_11_VAL,
  ZLFO_NODE_11_CURVE,
  ZLFO_NODE_12_POS,
  ZLFO_NODE_12_VAL,
  ZLFO_NODE_12_CURVE,
  ZLFO_NODE_13_POS,
  ZLFO_NODE_13_VAL,
  ZLFO_NODE_13_CURVE,
  ZLFO_NODE_14_POS,
  ZLFO_NODE_14_VAL,
  ZLFO_NODE_14_CURVE,
  ZLFO_NODE_15_POS,
  ZLFO_NODE_15_VAL,
  ZLFO_NODE_15_CURVE,
  ZLFO_NODE_16_POS,
  ZLFO_NODE_16_VAL,
  ZLFO_NODE_16_CURVE,
  ZLFO_NUM_NODES,
  ZLFO_SINE_OUT,
  ZLFO_TRIANGLE_OUT,
  ZLFO_SAW_OUT,
  ZLFO_SQUARE_OUT,
  ZLFO_CUSTOM_OUT,
  NUM_ZLFO_PORTS,
} PortIndex;

typedef enum GridStep
{
  GRID_STEP_FULL,
  GRID_STEP_HALF,
  GRID_STEP_FOURTH,
  GRID_STEP_EIGHTH,
  GRID_STEP_SIXTEENTH,
  GRID_STEP_THIRTY_SECOND,
  NUM_GRID_STEPS,
} GridStep;

typedef enum SyncRate
{
  SYNC_1_128,
  SYNC_1_64,
  SYNC_1_32,
  SYNC_1_16,
  SYNC_1_8,
  SYNC_1_4,
  SYNC_1_2,
  SYNC_1_1,
  SYNC_2_1,
  SYNC_4_1,
#ifdef HAVE_64_BARS
  SYNC_8_1,
  SYNC_16_1,
  SYNC_32_1,
  SYNC_64_1,
#endif
  NUM_SYNC_RATES,
} SyncRate;

typedef enum SyncRateType
{
  SYNC_TYPE_NORMAL,
  SYNC_TYPE_DOTTED,
  SYNC_TYPE_TRIPLET,
  NUM_SYNC_RATE_TYPES,
} SyncRateType;

typedef enum CurveAlgorithm
{
  CURVE_ALGORITHM_EXPONENT,
  CURVE_ALGORITHM_SUPERELLIPSE,
} CurveAlgorithm;

typedef struct HostPosition
{
  float     bpm;

  /** Current global frame. */
  long      frame;

  /** Transport speed (0.0 is stopped, 1.0 is
   * normal playback, -1.0 is reverse playback,
   * etc.). */
  float     speed;

  int       beat_unit;
} HostPosition;

/**
 * Group of variables needed by both the DSP and
 * the UI.
 */
typedef struct ZLfoCommon
{
  HostPosition   host_pos;

  /** Log feature. */
  LV2_Log_Log *        log;

  /** Map feature. */
  LV2_URID_Map *       map;

  /** Atom forge. */
  LV2_Atom_Forge forge;

  /** URIs. */
  ZLfoUris         uris;

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

  /**
   * Sine multiplier.
   *
   * This is a pre-calculated variable that is used
   * when calculating the sine value.
   */
  float         sine_multiplier;

  float         saw_multiplier;
} ZLfoCommon;

typedef struct NodeIndexElement
{
  int   index;
  float pos;
} NodeIndexElement;

static inline void
map_uris (
  LV2_URID_Map* map,
  ZLfoUris* uris)
{
#define MAP(x,uri) \
  uris->x = map->map (map->handle, uri)

  /* official URIs */
  MAP (atom_Blank, LV2_ATOM__Blank);
  MAP (atom_Object, LV2_ATOM__Object);
  MAP (atom_Float, LV2_ATOM__Float);
  MAP (atom_Double, LV2_ATOM__Double);
  MAP (atom_Int, LV2_ATOM__Int);
  MAP (atom_Long, LV2_ATOM__Long);
  MAP (atom_eventTransfer, LV2_ATOM__eventTransfer);
  MAP (log_Entry, LV2_LOG__Entry);
  MAP (log_Error, LV2_LOG__Error);
  MAP (log_Note, LV2_LOG__Note);
  MAP (log_Trace, LV2_LOG__Trace);
  MAP (log_Warning, LV2_LOG__Warning);
  MAP (time_Position, LV2_TIME__Position);
  MAP (time_bar, LV2_TIME__bar);
  MAP (time_barBeat, LV2_TIME__barBeat);
  MAP (
    time_beatsPerMinute, LV2_TIME__beatsPerMinute);
  MAP (time_beatUnit, LV2_TIME__beatUnit);
  MAP (time_frame, LV2_TIME__frame);
  MAP (time_speed, LV2_TIME__speed);

  /* custom URIs */
  MAP (ui_on, LFO_URI "#ui_on");
  MAP (ui_off, LFO_URI "#ui_off");
  MAP (ui_state, LFO_URI "#ui_state");
  MAP (
    ui_state_current_sample,
    LFO_URI "#ui_state_current_sample");
  MAP (
    ui_state_sine_multiplier,
    LFO_URI "#ui_state_sine_multiplier");
  MAP (
    ui_state_saw_multiplier,
    LFO_URI "#ui_state_saw_multiplier");
  MAP (
    ui_state_period_size,
    LFO_URI "#ui_state_period_size");
  MAP (
    ui_state_samplerate,
    LFO_URI "#ui_state_samplerate");
}

/**
 * Updates the position inside HostPosition with
 * the given time_Position atom object.
 */
static inline void
update_position_from_atom_obj (
  HostPosition *          host_pos,
  ZLfoUris *              uris,
  const LV2_Atom_Object * obj)
{
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
      host_pos->bpm = ((LV2_Atom_Float*)bpm)->body;
     }
  if (speed && speed->type == uris->atom_Float)
    {
      /* Speed changed, e.g. 0 (stop) to 1 (play) */
      host_pos->speed =
        ((LV2_Atom_Float *) speed)->body;
    }
  if (beat_unit && beat_unit->type == uris->atom_Int)
    {
      host_pos->beat_unit =
        ((LV2_Atom_Int *) beat_unit)->body;
    }
  if (frame && frame->type == uris->atom_Long)
    {
      host_pos->frame =
        ((LV2_Atom_Int *) frame)->body;
    }
  if (beat && beat->type == uris->atom_Float)
    {
      /*const float bar_beats =*/
        /*((LV2_Atom_Float *) beat)->body;*/
      /*self->beat_offset = fmodf (bar_beats, 1.f);*/
    }
}

/**
 * Logs an error.
 */
static inline void
log_error (
  LV2_Log_Log * log,
  ZLfoUris *    uris,
  const char *  _fmt,
  ...)
{
  va_list args;
  va_start (args, _fmt);

  char fmt[900];
  strcpy (fmt, _fmt);
  strcat (fmt, "\n");

  if (log)
    {
      log->vprintf (
        log->handle, uris->log_Error,
        fmt, args);
    }
  else
    {
      vfprintf (stderr, fmt, args);
    }
  va_end (args);
}

/**
 * Gets the val of the custom graph at x, with
 * x_size corresponding to the period size.
 */
static inline float
get_custom_val_at_x (
  const float        prev_node_pos,
  const float        prev_node_val,
  const float        prev_node_curve,
  const float        next_node_pos,
  const float        next_node_val,
  const float        next_node_curve,
  float              x,
  float              x_size)
{
  if (next_node_pos - prev_node_pos < 0.00000001f)
    return prev_node_val;

  float xratio = x / x_size;

  float range = next_node_pos - prev_node_pos;

  /* x relative to the start of the previous node */
  float rel_x = xratio - prev_node_pos;

  /* get slope */
  float m =
    (next_node_val - prev_node_val) / range;

  return m * (rel_x) + prev_node_val;
}

#ifndef MAX
# define MAX(x,y) (x > y ? x : y)
#endif

#ifndef MIN
# define MIN(x,y) (x < y ? x : y)
#endif

#ifndef CLAMP
# define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

#endif
