/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZPlugins
 *
 * ZPlugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZPlugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Common code for both the DSP and the UI.
 */

#ifndef __Z_LFO_COMMON_H__
#define __Z_LFO_COMMON_H__

#include <string.h>

#include "lv2/atom/atom.h"
#include "lv2/log/log.h"
#include "lv2/urid/urid.h"

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
  LV2_URID atom_Int;
  LV2_URID log_Entry;
  LV2_URID log_Error;
  LV2_URID log_Note;
  LV2_URID log_Trace;
  LV2_URID log_Warning;
} ZLfoUris;

typedef enum PortIndex
{
  /** GUI to plugin communication. */
  ZLFO_CONTROL,
  /** Plugin to UI communication. */
  ZLFO_NOTIFY,

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
  ZLFO_NUM_NODES,
  ZLFO_CV_OUT,
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

static inline void
map_uris (
  LV2_URID_Map* map,
  ZLfoUris* uris)
{
#define MAP(x,uri) \
  uris->x = map->map (map->handle, uri)

  MAP (atom_Blank, LV2_ATOM__Blank);
  MAP (atom_Object, LV2_ATOM__Object);
  MAP (atom_Float, LV2_ATOM__Float);
  MAP (atom_Int, LV2_ATOM__Int);
  MAP (atom_eventTransfer, LV2_ATOM__eventTransfer);
  MAP (log_Entry, LV2_LOG__Entry);
  MAP (log_Error, LV2_LOG__Error);
  MAP (log_Note, LV2_LOG__Note);
  MAP (log_Trace, LV2_LOG__Trace);
  MAP (log_Warning, LV2_LOG__Warning);
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

#endif
