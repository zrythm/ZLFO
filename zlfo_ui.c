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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
  Copyright 2012-2019 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "zlfo_common.h"
#include "zlfo_ui_theme.h"

#include <cairo.h>

#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/log/log.h>
#include <lv2/patch/patch.h>
#include <lv2/urid/urid.h>
#include <lv2/ui/ui.h>
#include <ztoolkit/ztk.h>

#define TITLE "ZLFO"

/** Width and height of the window. */
#define WIDTH 480
#define HEIGHT 261

#define LEFT_BTN_WIDTH 40
#define TOP_BTN_HEIGHT 38
#define MID_REGION_WIDTH 394
#define MID_BTN_WIDTH 193
#define MID_REGION_HEIGHT 180

#define GET_HANDLE \
  ZLfoUi * self = (ZLfoUi *) puglGetHandle (view);

typedef struct ZtkApp ZtkApp;

typedef enum LeftButton
{
  LEFT_BTN_SINE,
  LEFT_BTN_TRIANGLE,
  LEFT_BTN_SAW,
  LEFT_BTN_SQUARE,
  LEFT_BTN_RND,
  NUM_LEFT_BUTTONS,
} LeftButton;

typedef enum TopButton
{
  TOP_BTN_CURVE,
  TOP_BTN_STEP,
  NUM_TOP_BUTTONS,
} TopButton;

typedef enum BotButton
{
  BOT_BTN_SYNC,
  BOT_BTN_FREE,
  NUM_BOT_BUTTONS,
} BotButton;

typedef enum GridButton
{
  GRID_BTN_SNAP,
  NUM_GRID_BUTTONS,
} GridButton;

typedef enum LabelType
{
  LBL_TYPE_INVERT,
  LBL_TYPE_SHIFT,
  NUM_LBL_TYPES,
} LabelType;

typedef enum DrawDataType
{
  DATA_TYPE_BTN_TOP,
  DATA_TYPE_BTN_LEFT,
  DATA_TYPE_BTN_BOT,
  DATA_TYPE_BTN_GRID,
  DATA_TYPE_LBL,
} DrawDataType;

typedef struct ZLfoUi
{
  /** Port values. */
  float                freq;
  float                phase;

  LV2UI_Write_Function write;
  LV2UI_Controller     controller;

  /** Map feature. */
  LV2_URID_Map *       map;

  /** Atom forge. */
  LV2_Atom_Forge       forge;

  /** Log feature. */
  LV2_Log_Log *        log;

  /** URIs. */
  ZLfoUris             uris;

  /**
   * This is the window passed in the features from
   * the host.
   *
   * The pugl window will be wrapped in here.
   */
  void *               parent_window;

  /**
   * Resize handle for the parent window.
   */
  LV2UI_Resize*        resize;

  ZtkApp *             app;
} ZLfoUi;

/**
 * Data to be passed around in the callbacks.
 */
typedef struct DrawData
{
  /** Enum value corresponding to the type. */
  int            val;
  DrawDataType   type;
  ZLfoUi *       zlfo_ui;
} DrawData;

#define SEND_PORT_EVENT(_self,idx,val) \
  _self->write ( \
     _self->controller, (uint32_t) idx, \
     sizeof (float), 0, &val)

/* FIXME uncomment */
#if 0
static void
set_freq (
  void *   obj,
  float    val)
{
  ZLfoUi * self = (ZLfoUi *) obj;
  self->freq = val;
  SEND_PORT_EVENT (self, LFO_FREQ, self->freq);
}

static float
get_freq (
  void *   obj)
{
  ZLfoUi * self = (ZLfoUi *) obj;
  return self->freq;
}
#endif

static void
bg_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr,
  ZLfoUi *    self)
{
  /* clear background to black first */
  cairo_set_source_rgba (cr, 0, 0, 0, 1);
  cairo_rectangle (
    cr, widget->rect.x, widget->rect.y,
    widget->rect.width, widget->rect.height);
  cairo_fill (cr);

  /* set theme background */
  zlfo_ui_theme_set_cr_color (cr, bg);
  cairo_rectangle (
    cr, widget->rect.x, widget->rect.y,
    widget->rect.width, widget->rect.height);
  cairo_fill (cr);
}

static void
add_bg_widget (
  ZLfoUi * self)
{
  ZtkRect rect = {
    0, 0, self->app->width, self->app->height };
  ZtkDrawingArea * da =
    ztk_drawing_area_new (
      &rect, NULL,
      (ZtkWidgetDrawCallback) bg_draw_cb,
      NULL, self);
  ztk_app_add_widget (
    self->app, (ZtkWidget *) da, 0);
}

static void
left_btn_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr,
  DrawData * data)
{
  /* set background */
  ZtkWidgetState state = widget->state;
  if (state & ZTK_WIDGET_STATE_PRESSED)
    {
      zlfo_ui_theme_set_cr_color (
        cr, left_button_click);
    }
  else if (state & ZTK_WIDGET_STATE_HOVERED)
    {
      zlfo_ui_theme_set_cr_color (cr, button_hover);
    }
  else
    {
      zlfo_ui_theme_set_cr_color (cr, button_normal);
    }
  cairo_rectangle (
    cr, widget->rect.x, widget->rect.y,
    widget->rect.width, widget->rect.height);
  cairo_fill (cr);

  /* draw svgs */
#define DRAW_SVG(caps,lowercase) \
  case LEFT_BTN_##caps: \
    { \
      ZtkRect rect = { \
        widget->rect.x + hpadding, \
        widget->rect.y + vpadding, \
        widget->rect.width - hpadding * 2, \
        widget->rect.height - vpadding * 2 }; \
      ztk_rsvg_draw ( \
        zlfo_ui_theme.lowercase##_svg, cr, &rect); \
    } \
    break

  const int hpadding = 8;
  const int vpadding = 4;
  switch (data->val)
    {
      DRAW_SVG (SINE, sine);
      DRAW_SVG (TRIANGLE, triangle);
      DRAW_SVG (SAW, saw);
      DRAW_SVG (SQUARE, square);
      DRAW_SVG (RND, rnd);
    default:
      break;
    }

#undef DRAW_SVG
}

static void
add_left_buttons (
  ZLfoUi * self)
{
  const int padding = 2;
  const int width = LEFT_BTN_WIDTH;
  const int height = 50;
  for (int i = 0; i < NUM_LEFT_BUTTONS; i++)
    {
      ZtkRect rect = {
        padding, padding + i * (height + padding),
        width, height };
      DrawData * data =
        calloc (1, sizeof (DrawData));
      data->val = i;
      data->type = DATA_TYPE_BTN_LEFT;
      data->zlfo_ui = self;
      ZtkDrawingArea * da =
        ztk_drawing_area_new (
          &rect, NULL,
          (ZtkWidgetDrawCallback) left_btn_draw_cb,
          NULL, data);
      ztk_app_add_widget (
        self->app, (ZtkWidget *) da, 1);
    }
}

static void
top_btn_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr,
  DrawData * data)
{
  /* set background */
  ZtkWidgetState state = widget->state;
  int is_normal = 0;
  int clicked = 0;
  if (state & ZTK_WIDGET_STATE_PRESSED)
    {
      zlfo_ui_theme_set_cr_color (cr, selected_bg);
      clicked = 1;
    }
  else if (state & ZTK_WIDGET_STATE_HOVERED)
    {
      zlfo_ui_theme_set_cr_color (cr, button_hover);
    }
  else
    {
      zlfo_ui_theme_set_cr_color (
        cr, button_normal);
      is_normal = 1;
    }

  cairo_rectangle (
    cr, widget->rect.x, widget->rect.y,
    widget->rect.width,
    is_normal ?
      /* show border if normal */
      widget->rect.height - 2 :
      widget->rect.height + 1);
  cairo_fill (cr);


  /* draw svgs */
#define DRAW_SVG(caps,lowercase) \
  case TOP_BTN_##caps: \
    { \
      ZtkRect rect = { \
        widget->rect.x + hpadding, \
        widget->rect.y + vpadding, \
        widget->rect.width - hpadding * 2, \
        widget->rect.height - vpadding * 2 }; \
      ztk_rsvg_draw ( \
        clicked ? \
          zlfo_ui_theme.lowercase##_active_svg : \
          zlfo_ui_theme.lowercase##_svg, \
          cr, &rect); \
    } \
    break

  const int hpadding = 6;
  const int vpadding = 6;
  switch (data->val)
    {
      DRAW_SVG (CURVE, curve);
      DRAW_SVG (STEP, step);
    default:
      break;
    }

#undef DRAW_SVG
}

static void
add_top_buttons (
  ZLfoUi * self)
{
  const int padding = 2;
  const int width = MID_BTN_WIDTH;
  const int height = TOP_BTN_HEIGHT;
  const int start = LEFT_BTN_WIDTH + padding;
  for (int i = 0; i < NUM_TOP_BUTTONS; i++)
    {
      ZtkRect rect = {
        start + padding + i * (width + padding),
        padding, width, height };
      DrawData * data =
        calloc (1, sizeof (DrawData));
      data->val = i;
      data->type = DATA_TYPE_BTN_TOP;
      data->zlfo_ui = self;
      ZtkDrawingArea * da =
        ztk_drawing_area_new (
          &rect, NULL,
          (ZtkWidgetDrawCallback) top_btn_draw_cb,
          NULL, data);
      ztk_app_add_widget (
        self->app, (ZtkWidget *) da, 1);
    }
}

static void
bot_btn_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr,
  DrawData * data)
{
  /* set background */
  ZtkWidgetState state = widget->state;
  int is_normal = 0;
  int clicked = 0;
  if (state & ZTK_WIDGET_STATE_PRESSED)
    {
      zlfo_ui_theme_set_cr_color (cr, selected_bg);
      clicked = 1;
    }
  else if (state & ZTK_WIDGET_STATE_HOVERED)
    {
      zlfo_ui_theme_set_cr_color (cr, button_hover);
    }
  else
    {
      zlfo_ui_theme_set_cr_color (cr, button_normal);
      is_normal = 1;
    }

  cairo_rectangle (
    cr, widget->rect.x,
    is_normal ?
      /* show border if normal */
      widget->rect.y :
      widget->rect.y - 4,
    widget->rect.width,
    is_normal ?
      /* show border if normal */
      widget->rect.height :
      widget->rect.height + 4);
  cairo_fill (cr);


  /* draw svgs */
#define DRAW_SVG(caps,lowercase) \
  case BOT_BTN_##caps: \
    { \
      ZtkRect rect = { \
        widget->rect.x + hpadding, \
        widget->rect.y + vpadding, \
        widget->rect.width - hpadding * 2, \
        widget->rect.height - vpadding * 2 }; \
      ztk_rsvg_draw ( \
        clicked ? \
          zlfo_ui_theme.lowercase##_black_svg : \
          zlfo_ui_theme.lowercase##_svg, \
          cr, &rect); \
    } \
    break

  const int hpadding = 6;
  const int vpadding = 0;
  switch (data->val)
    {
      DRAW_SVG (SYNC, sync);
      DRAW_SVG (FREE, freeb);
    default:
      break;
    }

#undef DRAW_SVG
}

static void
add_bot_buttons (
  ZLfoUi * self)
{
  const int padding = 2;
  const int width = MID_BTN_WIDTH;
  const int height = TOP_BTN_HEIGHT;
  const int start = LEFT_BTN_WIDTH + padding;
  for (int i = 0; i < NUM_BOT_BUTTONS; i++)
    {
      ZtkRect rect = {
        start + padding + i * (width + padding),
        TOP_BTN_HEIGHT + 4 + MID_REGION_HEIGHT,
        width, height };
      DrawData * data =
        calloc (1, sizeof (DrawData));
      data->val = i;
      data->type = DATA_TYPE_BTN_BOT;
      data->zlfo_ui = self;
      ZtkDrawingArea * da =
        ztk_drawing_area_new (
          &rect, NULL,
          (ZtkWidgetDrawCallback) bot_btn_draw_cb,
          NULL, data);
      ztk_app_add_widget (
        self->app, (ZtkWidget *) da, 1);
    }
}

static void
mid_region_bg_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr,
  ZLfoUi *    self)
{
  /* set background */
  zlfo_ui_theme_set_cr_color (cr, selected_bg);
  cairo_rectangle (
    cr, widget->rect.x, widget->rect.y,
    widget->rect.width, widget->rect.height);
  cairo_fill (cr);

  /* draw grid */
  const int hpadding = 26;
  const int space = 42;
  for (int i = 0; i < 9; i++)
    {
      if ((i % 4) == 0)
        {
          zlfo_ui_theme_set_cr_color (
            cr, grid_strong);
        }
      else
        {
          zlfo_ui_theme_set_cr_color (
            cr, grid);
        }
      cairo_move_to (
        cr,
        widget->rect.x + hpadding + i * space,
        widget->rect.y + 46);
      cairo_line_to (
        cr,
        widget->rect.x + hpadding + i * space,
        widget->rect.y + 164);
      cairo_stroke (cr);
    }
  zlfo_ui_theme_set_cr_color (
    cr, grid_strong);
  cairo_move_to (
    cr,
    widget->rect.x + hpadding,
    widget->rect.y + 105);
  cairo_line_to (
    cr,
    widget->rect.x + hpadding + 8 * space,
    widget->rect.y + 105);
  cairo_stroke (cr);
}

static void
add_mid_region_bg (
  ZLfoUi * self)
{
  const int padding = 4;
  ZtkRect rect = {
    LEFT_BTN_WIDTH + padding,
    TOP_BTN_HEIGHT + 2,
    MID_REGION_WIDTH - 6, MID_REGION_HEIGHT };
  ZtkDrawingArea * da =
    ztk_drawing_area_new (
      &rect, NULL,
      (ZtkWidgetDrawCallback) mid_region_bg_draw_cb,
      NULL, self);
  ztk_app_add_widget (
    self->app, (ZtkWidget *) da, 0);
}

static void
range_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr,
  ZLfoUi *    self)
{
  ZtkRect rect = {
    widget->rect.x,
    widget->rect.y,
    widget->rect.width,
    widget->rect.height };
  ztk_rsvg_draw (
    zlfo_ui_theme.range_svg, cr, &rect);
}

static void
add_range (
  ZLfoUi * self)
{
  ZtkRect rect = {
    (LEFT_BTN_WIDTH + MID_REGION_WIDTH) - 10,
    58,
    62, 180 };
  ZtkDrawingArea * da =
    ztk_drawing_area_new (
      &rect, NULL,
      (ZtkWidgetDrawCallback) range_draw_cb,
      NULL, self);
  ztk_app_add_widget (
    self->app, (ZtkWidget *) da, 0);
}

static void
zrythm_icon_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr,
  ZLfoUi *    self)
{
  ZtkRect rect = {
    widget->rect.x,
    widget->rect.y,
    widget->rect.width,
    widget->rect.height };
  ZtkWidgetState state = widget->state;
  if (state & ZTK_WIDGET_STATE_PRESSED)
    {
      zlfo_ui_theme_set_cr_color (cr, button_hover);
      /*cairo_rectangle (*/
        /*cr, widget->rect.x, widget->rect.y,*/
        /*widget->rect.width, widget->rect.height);*/
      /*cairo_fill (cr);*/
      ztk_rsvg_draw (
        zlfo_ui_theme.zrythm_orange_svg, cr, &rect);
    }
  else if (state & ZTK_WIDGET_STATE_HOVERED)
    {
      zlfo_ui_theme_set_cr_color (cr, button_hover);
      /*cairo_rectangle (*/
        /*cr, widget->rect.x, widget->rect.y,*/
        /*widget->rect.width, widget->rect.height);*/
      /*cairo_fill (cr);*/
      ztk_rsvg_draw (
        zlfo_ui_theme.zrythm_hover_svg, cr, &rect);
    }
  else
    {
      zlfo_ui_theme_set_cr_color (cr, button_normal);
      ztk_rsvg_draw (
        zlfo_ui_theme.zrythm_svg, cr, &rect);
    }
}

static void
add_zrythm_icon (
  ZLfoUi * self)
{
  ZtkRect rect = {
    LEFT_BTN_WIDTH + MID_REGION_WIDTH + 8,
    6, 30, 30 };
  ZtkDrawingArea * da =
    ztk_drawing_area_new (
      &rect, NULL,
      (ZtkWidgetDrawCallback) zrythm_icon_draw_cb,
      NULL, self);
  ztk_app_add_widget (
    self->app, (ZtkWidget *) da, 0);
}

static void
grid_btn_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr,
  DrawData *  data)
{
  /* set background */
  ZtkWidgetState state = widget->state;
  int hover = 0;
  int pressed = 0;
  if (state & ZTK_WIDGET_STATE_PRESSED)
    {
      pressed = 1;
    }
  else if (state & ZTK_WIDGET_STATE_HOVERED)
    {
      hover = 1;
    }

  /* draw svgs */
#define DRAW_SVG(caps,lowercase) \
  case GRID_BTN_##caps: \
    { \
      ZtkRect rect = { \
        widget->rect.x, \
        widget->rect.y, \
        widget->rect.width, \
        widget->rect.height }; \
      if (pressed) \
        { \
          ztk_rsvg_draw ( \
            zlfo_ui_theme.lowercase##_click_svg, \
            cr, &rect); \
        } \
      else if (hover) \
        { \
          ztk_rsvg_draw ( \
            zlfo_ui_theme.lowercase##_hover_svg, \
            cr, &rect); \
        } \
      else \
        { \
          ztk_rsvg_draw ( \
            zlfo_ui_theme.lowercase##_svg, \
            cr, &rect); \
        } \
    } \
    break

  switch (data->val)
    {
      DRAW_SVG (SNAP, grid_snap);
    default:
      break;
    }

#undef DRAW_SVG
}

static void
grid_lbl_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr,
  DrawData *  data)
{
  /* draw svgs */
#define DRAW_SVG(caps,lowercase) \
  case LBL_TYPE_##caps: \
    { \
      ZtkRect rect = { \
        widget->rect.x, \
        widget->rect.y, \
        widget->rect.width, \
        widget->rect.height }; \
      ztk_rsvg_draw ( \
        zlfo_ui_theme.lowercase##_svg, \
        cr, &rect); \
    } \
    break

  switch (data->val)
    {
      DRAW_SVG (INVERT, invert);
      DRAW_SVG (SHIFT, shift);
    default:
      break;
    }

#undef DRAW_SVG
}

static void
add_grid_controls (
  ZLfoUi * self)
{
  int padding = 2;
  int width = 76;
  int height = 22;
  int start = LEFT_BTN_WIDTH + padding + 12;
  for (int i = 0; i < NUM_GRID_BUTTONS; i++)
    {
      ZtkRect rect = {
        start + padding + i * (width + padding),
        TOP_BTN_HEIGHT + 12,
        width, height };
      DrawData * data =
        calloc (1, sizeof (DrawData));
      data->val = i;
      data->type = DATA_TYPE_BTN_GRID;
      data->zlfo_ui = self;
      ZtkDrawingArea * da =
        ztk_drawing_area_new (
          &rect, NULL,
          (ZtkWidgetDrawCallback) grid_btn_draw_cb,
          NULL, data);
      ztk_app_add_widget (
        self->app, (ZtkWidget *) da, 1);
    }

  /* add labels */
  padding = 2;
  width = 76;
  height = 22;
  start = LEFT_BTN_WIDTH + padding;
  for (int i = 0; i < NUM_LBL_TYPES; i++)
    {
      ZtkRect rect;

      if (i == LBL_TYPE_INVERT)
        {
          rect.x = 156;
        }
      else if (i == LBL_TYPE_SHIFT)
        {
          rect.x = 300;
        }
      rect.y = TOP_BTN_HEIGHT + 12;
      rect.width = width;
      rect.height = height;

      DrawData * data =
        calloc (1, sizeof (DrawData));
      data->val = i;
      data->type = DATA_TYPE_LBL;
      data->zlfo_ui = self;
      ZtkDrawingArea * da =
        ztk_drawing_area_new (
          &rect, NULL,
          (ZtkWidgetDrawCallback) grid_lbl_draw_cb,
          NULL, data);
      ztk_app_add_widget (
        self->app, (ZtkWidget *) da, 1);
    }
}

static void
create_ui (
  ZLfoUi * self)
{
  /* resize the host's window. */
  self->resize->ui_resize (
    self->resize->handle, WIDTH, HEIGHT);

  self->app = ztk_app_new (
    TITLE, self->parent_window,
    WIDTH, HEIGHT);

  /* init the theme */
  zlfo_ui_theme_init ();

  /** add each control */
  add_bg_widget (self);
  add_left_buttons (self);
  add_top_buttons (self);
  add_bot_buttons (self);
  add_mid_region_bg (self);
  add_grid_controls (self);
  add_range (self);
  add_zrythm_icon (self);
}

static LV2UI_Handle
instantiate (
  const LV2UI_Descriptor*   descriptor,
  const char*               plugin_uri,
  const char*               bundle_path,
  LV2UI_Write_Function      write_function,
  LV2UI_Controller          controller,
  LV2UI_Widget*             widget,
  const LV2_Feature* const* features)
{
  ZLfoUi * self = calloc (1, sizeof (ZLfoUi));
  self->write = write_function;
  self->controller = controller;

#define HAVE_FEATURE(x) \
  (!strcmp(features[i]->URI, x))

  for (int i = 0; features[i]; ++i)
    {
      if (HAVE_FEATURE (LV2_UI__parent))
        {
          self->parent_window = features[i]->data;
        }
      else if (HAVE_FEATURE (LV2_UI__resize))
        {
          self->resize =
            (LV2UI_Resize*)features[i]->data;
        }
      else if (HAVE_FEATURE (LV2_URID__map))
        {
          self->map =
            (LV2_URID_Map *) features[i]->data;
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
      log_error (
        self->log, &self->uris,
        "Missing feature urid:map");
    }

  /* map uris */
  map_uris (self->map, &self->uris);

  lv2_atom_forge_init (
    &self->forge, self->map);

  /* create UI and set the native window to the
   * widget */
  create_ui (self);
  *widget =
    (LV2UI_Widget)
    puglGetNativeWindow (self->app->view);

  return self;
}

static void
cleanup (LV2UI_Handle handle)
{
  ZLfoUi * self = (ZLfoUi *) handle;

  ztk_app_free (self->app);

  free (self);
}

/**
 * Port event from the plugin.
 */
static void
port_event (
  LV2UI_Handle handle,
  uint32_t     port_index,
  uint32_t     buffer_size,
  uint32_t     format,
  const void*  buffer)
{
  ZLfoUi * self = (ZLfoUi *) handle;

  /* check type of data received
   *  format == 0: [float] control-port event
   *  format > 0: message
   *  Every event message is sent as separate
   *  port-event
   */
  if (format == 0)
    {
      switch (port_index)
        {
        case LFO_FREQ:
          self->freq = * (const float *) buffer;
          break;
        default:
          break;
        }
      puglPostRedisplay (self->app->view);
    }
  else if (format == self->uris.atom_eventTransfer)
    {
      const LV2_Atom* atom =
        (const LV2_Atom*) buffer;
      if (lv2_atom_forge_is_object_type (
            &self->forge, atom->type))
        {
          /*const LV2_Atom_Object* obj =*/
            /*(const LV2_Atom_Object*) atom;*/

          /*const char* uri =*/
            /*(const char*) LV2_ATOM_BODY_CONST (*/
              /*obj);*/
        }
      else
        {
          log_error (
            self->log, &self->uris,
            "Unknown message type");
        }
    }
  else
    {
      log_error (
        self->log, &self->uris,
        "Unknown format");
    }
}

/* Optional non-embedded UI show interface. */
/*static int*/
/*ui_show (LV2UI_Handle handle)*/
/*{*/
  /*printf ("show called\n");*/
  /*ZLfoUi * self = (ZLfoUi *) handle;*/
  /*ztk_app_show_window (self->app);*/
  /*return 0;*/
/*}*/

/* Optional non-embedded UI hide interface. */
/*static int*/
/*ui_hide (LV2UI_Handle handle)*/
/*{*/
  /*printf ("hide called\n");*/
  /*ZLfoUi * self = (ZLfoUi *) handle;*/
  /*ztk_app_hide_window (self->app);*/

  /*return 0;*/
/*}*/

/**
 * LV2 idle interface for optional non-embedded
 * UI.
 */
static int
ui_idle (LV2UI_Handle handle)
{
  ZLfoUi * self = (ZLfoUi *) handle;

  ztk_app_idle (self->app);

  return 0;
}

/**
 * LV2 resize interface for the host.
 */
static int
ui_resize (
  LV2UI_Feature_Handle handle, int w, int h)
{
  ZLfoUi * self = (ZLfoUi *) handle;
  self->resize->ui_resize (
    self->resize->handle, WIDTH, HEIGHT);
  return 0;
}

/**
 * Called by the host to get the idle and resize
 * functions.
 */
static const void*
extension_data (const char* uri)
{
  static const LV2UI_Idle_Interface idle = {
    ui_idle };
  static const LV2UI_Resize resize = {
    0 ,ui_resize };
  if (!strcmp(uri, LV2_UI__idleInterface))
    {
      return &idle;
    }
  if (!strcmp(uri, LV2_UI__resize))
    {
      return &resize;
    }
  return NULL;
}

static const LV2UI_Descriptor descriptor = {
  LFO_UI_URI,
  instantiate,
  cleanup,
  port_event,
  extension_data,
};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor (uint32_t index)
{
  switch (index)
    {
    case 0:
      return &descriptor;
    default:
      return NULL;
    }
}
