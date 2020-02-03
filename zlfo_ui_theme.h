/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 */

#ifndef __ZLFO_UI_THEME_H__
#define __ZLFO_UI_THEME_H__

#include "config.h"

#include <ztoolkit/ztk.h>

#include <glib.h>

/**
 * Theme for the ZLFO UI.
 */
typedef struct ZLfoUiTheme
{
  /** Background color. */
  ZtkColor bg;

  /** Selected area background color. */
  ZtkColor selected_bg;

  /** Button color. */
  ZtkColor button_normal;

  /** Button hover color. */
  ZtkColor button_hover;

  /** Button click color. */
  ZtkColor button_click;

  /** Left button click color. */
  ZtkColor left_button_click;

  /** Line/curve color. */
  ZtkColor line;

  /** Grid line color. */
  ZtkColor grid;

  /** Grid strong line color. */
  ZtkColor grid_strong;

  ZtkRsvgHandle * sine_svg;
  ZtkRsvgHandle * saw_svg;
  ZtkRsvgHandle * triangle_svg;
  ZtkRsvgHandle * square_svg;
  ZtkRsvgHandle * rnd_svg;
  ZtkRsvgHandle * curve_svg;
  ZtkRsvgHandle * step_svg;
  ZtkRsvgHandle * curve_active_svg;
  ZtkRsvgHandle * step_active_svg;

  ZtkRsvgHandle * range_svg;

  ZtkRsvgHandle * sync_svg;
  ZtkRsvgHandle * freeb_svg;
  ZtkRsvgHandle * sync_black_svg;
  ZtkRsvgHandle * freeb_black_svg;

  ZtkRsvgHandle * zrythm_svg;
  ZtkRsvgHandle * zrythm_hover_svg;
  ZtkRsvgHandle * zrythm_orange_svg;

  ZtkRsvgHandle * grid_snap_svg;
  ZtkRsvgHandle * grid_snap_hover_svg;
  ZtkRsvgHandle * grid_snap_click_svg;
  ZtkRsvgHandle * hmirror_svg;
  ZtkRsvgHandle * hmirror_hover_svg;
  ZtkRsvgHandle * hmirror_click_svg;
  ZtkRsvgHandle * vmirror_svg;
  ZtkRsvgHandle * vmirror_hover_svg;
  ZtkRsvgHandle * vmirror_click_svg;
  ZtkRsvgHandle * invert_svg;
  ZtkRsvgHandle * shift_svg;

} ZLfoUiTheme;

static ZLfoUiTheme zlfo_ui_theme;

static inline void
zlfo_ui_theme_init (void)
{
#define SET_COLOR(cname,_hex) \
  ztk_color_parse_hex ( \
    &zlfo_ui_theme.cname, _hex); \
  zlfo_ui_theme.cname.alpha = 1.0

  SET_COLOR (bg, "#323232");
  SET_COLOR (button_normal, "#5A5A5A");
  SET_COLOR (button_hover, "#6D6D6D");
  SET_COLOR (button_click, "#22DAFB");
  SET_COLOR (left_button_click, "#FF6501");
  SET_COLOR (line, "#0D5562");
  SET_COLOR (selected_bg, "#1BAEC9");
  SET_COLOR (grid_strong, "#86ECFE");
  SET_COLOR (grid, "#23D9FB");

  char * abs_path;
#define LOAD_SVG(name) \
  abs_path = \
    g_build_filename ( \
      INSTALL_PATH, "resources", #name ".svg", \
      NULL); \
  zlfo_ui_theme.name##_svg = \
    ztk_rsvg_load_svg (abs_path); \
  if (!zlfo_ui_theme.name##_svg) \
    { \
      ztk_error ( \
        "Failed loading SVG: %s", abs_path); \
      exit (1); \
    }

  LOAD_SVG (sine);
  LOAD_SVG (triangle);
  LOAD_SVG (saw);
  LOAD_SVG (square);
  LOAD_SVG (rnd);
  LOAD_SVG (curve);
  LOAD_SVG (step);
  LOAD_SVG (curve_active);
  LOAD_SVG (step_active);
  LOAD_SVG (range);
  LOAD_SVG (sync);
  LOAD_SVG (freeb);
  LOAD_SVG (sync_black);
  LOAD_SVG (freeb_black);
  LOAD_SVG (zrythm);
  LOAD_SVG (zrythm_hover);
  LOAD_SVG (zrythm_orange);
  LOAD_SVG (grid_snap);
  LOAD_SVG (grid_snap_hover);
  LOAD_SVG (grid_snap_click);
  LOAD_SVG (hmirror);
  LOAD_SVG (hmirror_hover);
  LOAD_SVG (hmirror_click);
  LOAD_SVG (vmirror);
  LOAD_SVG (vmirror_hover);
  LOAD_SVG (vmirror_click);
  LOAD_SVG (invert);
  LOAD_SVG (shift);
}

/**
 * Sets the cairo color to that in the theme.
 */
#define zlfo_ui_theme_set_cr_color(cr,color_name) \
  ztk_color_set_for_cairo ( \
    &zlfo_ui_theme.color_name, cr)

#endif
