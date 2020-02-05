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
  GRID_BTN_HMIRROR,
  GRID_BTN_VMIRROR,
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
  float            freq;
  float            shift;
  float            range_min;
  float            range_max;
  int              step_mode;
  int              freerun;

  LV2UI_Write_Function write;
  LV2UI_Controller controller;

  /** Map feature. */
  LV2_URID_Map *   map;

  /** Atom forge. */
  LV2_Atom_Forge   forge;

  /** Log feature. */
  LV2_Log_Log *    log;

  /** URIs. */
  ZLfoUris         uris;

  /**
   * This is the window passed in the features from
   * the host.
   *
   * The pugl window will be wrapped in here.
   */
  void *           parent_window;

  /**
   * Resize handle for the parent window.
   */
  LV2UI_Resize*    resize;

  ZtkApp *         app;
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

/**
 * Called when one of the buttons was clicked.
 */
static void
on_btn_clicked (
  ZtkWidget * widget,
  DrawData *  data)
{
  ztk_message ("%s", "Button clicked!");
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
      ZtkButton * btn =
        ztk_button_new (
          &rect,
          (ZtkWidgetActivateCallback)
          on_btn_clicked, data);
      ztk_button_set_background_colors (
        btn,
        &zlfo_ui_theme.button_normal,
        &zlfo_ui_theme.button_hover,
        &zlfo_ui_theme.left_button_click);

#define MAKE_BUTTON_SVGED(caps,lowercase) \
  case LEFT_BTN_##caps: \
    { \
      ztk_button_make_svged (\
        btn, hpadding, vpadding, \
        zlfo_ui_theme.lowercase##_svg, \
        zlfo_ui_theme.lowercase##_svg, \
        zlfo_ui_theme.lowercase##_svg); \
    } \
    break

      int hpadding = 8;
      int vpadding = 4;
      switch (data->val)
        {
          MAKE_BUTTON_SVGED (SINE, sine);
          MAKE_BUTTON_SVGED (TRIANGLE, triangle);
          MAKE_BUTTON_SVGED (SAW, saw);
          MAKE_BUTTON_SVGED (SQUARE, square);
          MAKE_BUTTON_SVGED (RND, rnd);
        }

#undef MAKE_BUTTON_SVGED

      ztk_app_add_widget (
        self->app, (ZtkWidget *) btn, 1);
    }
}

static int
get_button_active (
  ZtkButton * btn,
  DrawData *  data)
{
  ZLfoUi * self = data->zlfo_ui;

  switch (data->type)
    {
    case DATA_TYPE_BTN_TOP:
      switch (data->val)
        {
        case TOP_BTN_CURVE:
          return !self->step_mode;
          break;
        case TOP_BTN_STEP:
          return self->step_mode;
          break;
        }
      break;
    case DATA_TYPE_BTN_LEFT:
      break;
    case DATA_TYPE_BTN_BOT:
      switch (data->val)
        {
        case BOT_BTN_SYNC:
          return !self->freerun;
          break;
        case BOT_BTN_FREE:
          return self->freerun;
          break;
        }
      break;
    case DATA_TYPE_BTN_GRID:
      break;
    case DATA_TYPE_LBL:
      break;
    }

  return 0;
}

static void
top_and_bot_btn_bg_cb (
  ZtkWidget * w,
  cairo_t *   cr,
  DrawData *  data)
{
  /* set background */
  ZtkWidgetState state = w->state;
  int is_normal = 0;
  if (state & ZTK_WIDGET_STATE_PRESSED ||
      get_button_active ((ZtkButton *) w, data))
    {
      zlfo_ui_theme_set_cr_color (cr, selected_bg);
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

  if (data->type == DATA_TYPE_BTN_TOP)
    {
      cairo_rectangle (
        cr, w->rect.x, w->rect.y, w->rect.width,
        is_normal ?
          /* show border if normal */
          w->rect.height - 2 :
          w->rect.height + 1);
    }
  else if (data->type == DATA_TYPE_BTN_BOT)
    {
      cairo_rectangle (
        cr, w->rect.x,
        is_normal ?
          /* show border if normal */
          w->rect.y :
          w->rect.y - 3,
        w->rect.width,
        is_normal ?
          /* show border if normal */
          w->rect.height :
          w->rect.height + 3);
    }
  cairo_fill (cr);
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
      ZtkButton * btn =
        ztk_button_new (
          &rect,
          (ZtkWidgetActivateCallback)
          on_btn_clicked, data);
      ztk_button_add_background_callback (
        btn,
        (ZtkWidgetDrawCallback)
        top_and_bot_btn_bg_cb);
      ztk_button_make_toggled (
        btn,
        (ZtkButtonToggledGetter)
        get_button_active);

#define MAKE_BUTTON_SVGED(caps,lowercase) \
  case TOP_BTN_##caps: \
    { \
      ztk_button_make_svged (\
        btn, hpadding, vpadding, \
        zlfo_ui_theme.lowercase##_svg, \
        zlfo_ui_theme.lowercase##_svg, \
        zlfo_ui_theme.lowercase##_svg); \
    } \
    break

      int hpadding = 6;
      int vpadding = 6;
      switch (data->val)
        {
          MAKE_BUTTON_SVGED (CURVE, curve);
          MAKE_BUTTON_SVGED (STEP, step);
        }

#undef MAKE_BUTTON_SVGED

      ztk_app_add_widget (
        self->app, (ZtkWidget *) btn, 1);
    }
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
      ZtkButton * btn =
        ztk_button_new (
          &rect,
          (ZtkWidgetActivateCallback)
          on_btn_clicked, data);
      ztk_button_add_background_callback (
        btn,
        (ZtkWidgetDrawCallback)
        top_and_bot_btn_bg_cb);
      ztk_button_make_toggled (
        btn,
        (ZtkButtonToggledGetter)
        get_button_active);

#define MAKE_BUTTON_SVGED(caps,lowercase) \
  case BOT_BTN_##caps: \
    { \
      ztk_button_make_svged (\
        btn, hpadding, vpadding, \
        zlfo_ui_theme.lowercase##_svg, \
        zlfo_ui_theme.lowercase##_svg, \
        zlfo_ui_theme.lowercase##_svg); \
    } \
    break

      int hpadding = 6;
      int vpadding = 0;
      switch (data->val)
        {
          MAKE_BUTTON_SVGED (SYNC, sync);
          MAKE_BUTTON_SVGED (FREE, freeb);
        }

#undef MAKE_BUTTON_SVGED

      ztk_app_add_widget (
        self->app, (ZtkWidget *) btn, 1);
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
      DRAW_SVG (HMIRROR, hmirror);
      DRAW_SVG (VMIRROR, vmirror);
    default:
      break;
    }

#undef DRAW_SVG
}

static float
shift_control_getter (
  ZtkControl * control,
  ZLfoUi *     self)
{
  return self->shift;
}

static void
shift_control_setter (
  ZtkControl * control,
  ZLfoUi *     self,
  float        val)
{
  self->shift = val;
  SEND_PORT_EVENT (self, ZLFO_SHIFT, self->shift);
}

/**
 * Macro to get real value.
 */
#define GET_REAL_VAL \
  ((*ctrl->getter) (ctrl, ctrl->object))

/**
 * MAcro to get real value from knob value.
 */
#define REAL_VAL_FROM_KNOB(knob) \
  (ctrl->min + (float) knob * \
   (ctrl->max - ctrl->min))

/**
 * Converts from real value to knob value
 */
#define KNOB_VAL_FROM_REAL(real) \
  (((float) real - ctrl->min) / \
   (ctrl->max - ctrl->min))

/**
 * Sets real val
 */
#define SET_REAL_VAL(real) \
   ((*ctrl->setter)(ctrl->object, (float) real))

static void
shift_control_draw_cb (
  ZtkWidget * widget,
  cairo_t *   cr,
  ZLfoUi *    self)
{
  ZtkControl * ctrl = (ZtkControl *) widget;

  /* draw bg */
  zlfo_ui_theme_set_cr_color (cr, button_normal);
  cairo_rectangle (
    cr, widget->rect.x, widget->rect.y,
    widget->rect.width, widget->rect.height);
  cairo_fill (cr);

  /* draw black bg */
  const int bg_padding = 2;
  zlfo_ui_theme_set_cr_color (cr, bg);
  cairo_rectangle (
    cr, widget->rect.x + bg_padding,
    widget->rect.y + bg_padding,
    widget->rect.width - bg_padding * 2,
    widget->rect.height - bg_padding * 2);
  cairo_fill (cr);

  /* set color */
  if (widget->state & ZTK_WIDGET_STATE_PRESSED)
    {
      cairo_set_source_rgba (cr, 0.9, 0.9, 0.9, 1);
    }
  else if (widget->state & ZTK_WIDGET_STATE_HOVERED)
    {
      cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, 1);
    }
  else
    {
      cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, 1);
    }

  /* the half width of the available bar area */
  double half_width =
    (widget->rect.width  - bg_padding * 2.0) / 2.0;

  double handle_size = 12.0;

  /* draw bar */
  double real_val = (double) GET_REAL_VAL;
  if (real_val < 0.5)
    {
      double work_val = real_val / 0.5;
      double start_x =
        work_val * half_width - handle_size / 2.0;
      cairo_rectangle (
        cr,
        widget->rect.x + bg_padding +
          (start_x < 0.0 ? 0.0 : start_x),
        widget->rect.y + bg_padding,
        start_x < 0.0 ?
          handle_size + start_x : handle_size,
        widget->rect.height - bg_padding * 2);
    }
  else
    {
      double work_val = (real_val - 0.5) / 0.5;
      double start_x =
        widget->rect.x + bg_padding + half_width +
        (work_val * half_width - handle_size / 2.0);
      double extrusion =
        (start_x + handle_size) -
          ((widget->rect.x + widget->rect.width) - bg_padding);
      cairo_rectangle (
        cr, start_x,
        widget->rect.y + bg_padding,
        extrusion > 0.0 ?
          handle_size - extrusion : handle_size,
        widget->rect.height - bg_padding * 2);
    }
  cairo_fill (cr);
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

typedef struct TestStruct
{
  int id;
  char label[600];
  ZtkComboBox * combo;
} TestStruct;

static void
activate_cb (
  ZtkWidget *  widget,
  TestStruct * test)
{
  ztk_message (
    "activate %p %d %s", test->combo,
    test->id, test->label);
}

static void
button_event_cb (
  ZtkWidget * widget,
  const PuglEventButton * btn,
  ZLfoUi * self)
{
  if ((((PuglEvent *) btn)->type !=
         PUGL_BUTTON_RELEASE) ||
      (!ztk_widget_is_hit (widget, btn->x, btn->y)))
    return;

  ZtkComboBox * combo =
    ztk_combo_box_new (
      widget, 0, 0);
  ztk_app_add_widget (
    widget->app, (ZtkWidget *) combo, 100);

  for (int i = 0; i < 90; i++)
    {
      TestStruct * test =
        calloc (1, sizeof (TestStruct));
      test->id = i;
      test->combo = combo;
      sprintf (test->label, "Test %d", i);
      ztk_combo_box_add_text_element (
        combo, test->label,
        (ZtkWidgetActivateCallback) activate_cb,
        test);
      if (i % 3 == 0)
        ztk_combo_box_add_separator (combo);
    }
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
      ZtkRect rect;
      switch (i)
        {
        case GRID_BTN_SNAP:
          rect.x =
            start + padding;
          rect.width = 76;
          break;
        case GRID_BTN_HMIRROR:
          rect.x =
            start + padding + width + padding +
            68;
          rect.width = 40;
          break;
        case GRID_BTN_VMIRROR:
          rect.x =
            start + padding + width + padding +
            110;
          rect.width = 40;
          break;
        default:
          break;
        }
      rect.height = 22;
      rect.y = TOP_BTN_HEIGHT + 12;
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

      ((ZtkWidget *) da)->button_event_cb =
        (ZtkWidgetButtonEventCallback)
        button_event_cb;
    }

  /* add shift control */
  ZtkRect rect = {
    start + padding + width + padding + 210,
    TOP_BTN_HEIGHT + 12, 76, 22 };
  ZtkControl * control =
    ztk_control_new (
      &rect,
      (ZtkControlGetter) shift_control_getter,
      (ZtkControlSetter) shift_control_setter,
      (ZtkWidgetDrawCallback) shift_control_draw_cb,
      ZTK_CTRL_DRAG_HORIZONTAL,
      self, 0.f, 1.f, 0.5f);
  control->sensitivity = 0.02f;
  ztk_app_add_widget (
    self->app, (ZtkWidget *) control, 1);

  /* add labels */
  padding = 2;
  width = 76;
  height = 22;
  start = LEFT_BTN_WIDTH + padding;
  for (int i = 0; i < NUM_LBL_TYPES; i++)
    {
      if (i == LBL_TYPE_INVERT)
        {
          rect.x = 138;
        }
      else if (i == LBL_TYPE_SHIFT)
        {
          rect.x = 282;
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
        case ZLFO_FREQ:
          self->freq = * (const float *) buffer;
          break;
        case ZLFO_SHIFT:
          self->shift = * (const float *) buffer;
          break;
        case ZLFO_RANGE_MIN:
          self->range_min =
            * (const float *) buffer;
          break;
        case ZLFO_RANGE_MAX:
          self->range_max =
            * (const float *) buffer;
          break;
        case ZLFO_STEP_MODE:
          self->step_mode =
            (int) * (const float *) buffer;
          break;
        case ZLFO_FREE_RUNNING:
          self->freerun =
            (int) * (const float *) buffer;
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
