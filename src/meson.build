# Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of ZLFO
#
# ZLFO is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ZLFO is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with ZLFO.  If not, see <https://www.gnu.org/licenses/>.

inc_dirs = include_directories(['.', '..'])

zlfo_dsp_lib = shared_library (
  'zlfo_dsp',
  name_prefix: '',
  sources: [
    'zlfo.c',
    ],
  dependencies: zlfo_deps,
  include_directories: inc_dirs,
  install: true,
  install_dir: zlfodir,
  )

zlfo_ui_lib = shared_library (
  'zlfo_ui',
  name_prefix: '',
  sources: [
    'zlfo_ui.c',
    ],
  dependencies: [
    zlfo_deps,
    ztoolkit_dep,
    dependency('glib-2.0'),
    ],
  include_directories: inc_dirs,
  install: true,
  install_dir: zlfodir,
  )

# create and install manifest ttl
manifest_ttl = configure_file (
  input: 'manifest.ttl.in',
  output: 'manifest.ttl',
  configuration: zlfo_cdata,
  )
install_data (
  manifest_ttl,
  install_dir: zlfodir,
  )

# create and install zlfo ttl
lv2_ttl_gen = executable (
  'lv2_ttl_gen',
  sources: [
    'zlfo_ttl_gen.c',
    ],
  include_directories: inc_dirs,
  c_args: common_cflags,
  install: false,
  )
zlfo_ttl = custom_target (
  'zlfo.ttl',
  output: 'zlfo.ttl',
  input: [ lv2_ttl_gen, zlfo_config_h ],
  command: [
    lv2_ttl_gen, '@OUTPUT@' ],
  install: true,
  install_dir: zlfodir,
  )

# used for tests
zlfo_build_dir = meson.current_build_dir ()
