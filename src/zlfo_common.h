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

#include <math.h>
#include <string.h>

#include "lv2/atom/atom.h"
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

  /* custom URIs */
  /* object */
  LV2_URID ui_state;
  LV2_URID ui_state_current_sample;
  LV2_URID ui_state_period_size;
  LV2_URID ui_state_samplerate;

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

  ZLFO_CV_GATE,
  ZLFO_CV_TRIGGER,
  ZLFO_GATE,
  ZLFO_TRIGGER,
  ZLFO_SYNC_RATE,
  ZLFO_SYNC_RATE_TYPE,
  ZLFO_FREQ,
  ZLFO_SHIFT,
  ZLFO_RANGE_MIN,
  ZLFO_RANGE_MAX,
  ZLFO_STEP_MODE,
  ZLFO_FREE_RUNNING,
  ZLFO_HINVERT,
  ZLFO_VINVERT,
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
  ZLFO_RND_OUT,
  ZLFO_CUSTOM_OUT,
  NUM_ZLFO_PORTS,
} PortIndex;

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

static inline float
sync_rate_to_float (
  SyncRate     rate,
  SyncRateType type)
{
  float r = 0.01f;
  switch (rate)
    {
    case SYNC_1_128:
      r = 1.f / 128.f;
      break;
    case SYNC_1_64:
      r = 1.f / 64.f;
      break;
    case SYNC_1_32:
      r = 1.f / 32.f;
      break;
    case SYNC_1_16:
      r = 1.f / 16.f;
      break;
    case SYNC_1_8:
      r = 1.f / 8.f;
      break;
    case SYNC_1_4:
      r = 1.f / 4.f;
      break;
    case SYNC_1_2:
      r = 1.f / 2.f;
      break;
    case SYNC_1_1:
      r = 1.f;
      break;
    case SYNC_2_1:
      r = 2.f;
      break;
    case SYNC_4_1:
      r = 4.f;
      break;
    default:
      break;
    }

  switch (type)
    {
    case SYNC_TYPE_NORMAL:
      break;
    case SYNC_TYPE_DOTTED:
      r *= 1.5f;
      break;
    case SYNC_TYPE_TRIPLET:
      r *= (2.f / 3.f);
      break;
    default:
      break;
    }

  return r;
}

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
    ui_state_period_size,
    LFO_URI "#ui_state_period_size");
  MAP (
    ui_state_samplerate,
    LFO_URI "#ui_state_samplerate");
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
 * Gets the y value for a node at the given X coord.
 *
 * See https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
 * @param x X-coordinate.
 * @param curviness Curviness variable (1.0 is
 *   a straight line, 0.0 is full curved).
 * @param start_higher Start at higher point.
 */
double
get_y_normalized (
  double x,
  double curviness,
  CurveAlgorithm algo,
  int    start_higher,
  int    curve_up);

#ifdef pow
double
get_y_normalized (
  double x,
  double curviness,
  CurveAlgorithm algo,
  int    start_higher,
  int    curve_up)
{
  if (!start_higher)
    x = 1.0 - x;
  if (curve_up)
    x = 1.0 - x;

  double val;
  switch (algo)
    {
    case CURVE_ALGORITHM_EXPONENT:
      val =
        pow (x, curviness);
      break;
    case CURVE_ALGORITHM_SUPERELLIPSE:
      val =
        pow (
          1.0 - pow (x, curviness),
          (1.0 / curviness));
      break;
    }
  if (curve_up)
    {
      val = 1.0 - val;
    }
  return val;

  fprintf (
    stderr, "This line should not be reached");
}
#endif

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
