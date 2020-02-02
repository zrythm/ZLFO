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

  ZtkRsvgHandle * sine_svg;
  ZtkRsvgHandle * saw_svg;
  ZtkRsvgHandle * triangle_svg;
  ZtkRsvgHandle * square_svg;
  ZtkRsvgHandle * rnd_svg;

} ZLfoUiTheme;

static ZLfoUiTheme zlfo_ui_theme;

static inline void
zlfo_ui_theme_init (void)
{
#define SET_COLOR(cname,_hex) \
  ztk_color_parse_hex ( \
    &zlfo_ui_theme.cname, _hex); \
  zlfo_ui_theme.bg.alpha = 1.0

  SET_COLOR (bg, "#323232");
  SET_COLOR (button_normal, "#5A5A5A");
  SET_COLOR (button_hover, "#6D6D6D");
  SET_COLOR (button_click, "#22DAFB");
  SET_COLOR (left_button_click, "#FF6501");
  SET_COLOR (line, "#0D5562");
  SET_COLOR (selected_bg, "#1BAEC9");

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
  LOAD_SVG (saw);
  LOAD_SVG (square);
  LOAD_SVG (rnd);
}

/**
 * Sets the cairo color to that in the theme.
 */
#define zlfo_ui_theme_set_cr_color(cr,color_name) \
  ztk_color_set_for_cairo ( \
    &zlfo_ui_theme.color_name, cr)

#endif
