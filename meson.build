# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

project (
  'ZLFO', ['c'],
  version: '0.1.0',
  license: 'AGPLv3+',
  meson_version: '>= 0.50.0',
  default_options: [
    'warning_level=2',
    'buildtype=debug',
    'c_std=gnu11',
    ],
  )

cc = meson.get_compiler ('c')

prefix = get_option('prefix')
bindir = join_paths(prefix, get_option('bindir'))
libdir = join_paths(prefix, get_option ('libdir'))
lv2dir = join_paths(libdir, 'lv2')
zlfodir = join_paths(lv2dir, 'ZLFO.lv2')

zlfo_cdata = configuration_data ()
zlfo_cdata.set (
  'PROJECT_URI',
  'https://lv2.zrythm.org/plugins')
zlfo_cdata.set (
  'LFO_URI',
  zlfo_cdata.get ('PROJECT_URI') + '/ZLFO')
zlfo_cdata.set (
  'LFO_UI_URI',
  zlfo_cdata.get ('LFO_URI') + '#UI')
zlfo_cdata.set ('LFO_DSP_BINARY', 'zlfo_dsp.so')
zlfo_cdata.set ('LFO_UI_BINARY', 'zlfo_ui.so')
zlfo_cdata.set ('LFO_TTL', 'zlfo.ttl')
if get_option('buildtype') == 'release'
  zlfo_cdata.set ('RELEASE', 1)
endif

zlfo_cdata_quoted = configuration_data ()
zlfo_cdata_quoted.set_quoted (
  'PROJECT_URI',
  zlfo_cdata.get ('PROJECT_URI'))
zlfo_cdata_quoted.set_quoted (
  'LFO_URI',
  zlfo_cdata.get ('LFO_URI'))
zlfo_cdata_quoted.set_quoted (
  'LFO_UI_URI',
  zlfo_cdata.get ('LFO_UI_URI'))
zlfo_cdata_quoted.set_quoted (
  'INSTALL_PATH', zlfodir)

# create config.h
zlfo_config_h = configure_file (
  output: 'config.h',
  configuration: zlfo_cdata_quoted,
  )
zlfo_config_h_dep = declare_dependency (
  sources: zlfo_config_h,
  )

zlfo_deps = [
  zlfo_config_h_dep,
  cc.find_library ('m'),
  ]
ztoolkit_dep = dependency(
  'ztoolkit', version: '>= 0.1',
  fallback: ['ztoolkit', 'ztoolkit_dep'],
  default_options: ['enable_rsvg=true'])

# cflags
common_cflags = cc.get_supported_arguments([
  '-Wformat=2',
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter',
  '-Wno-sequence-point',
  '-Wignored-qualifiers',
  '-Wno-cast-function-type',
  ])
if get_option ('strict_flags')
  common_cflags += cc.get_supported_arguments([
    #'-Werror=cast-qual',
    '-Werror=clobbered',
    #'-Werror=conversion',
    '-Werror=disabled-optimization',
    '-Werror=double-promotion',
    '-Werror=float-equal',
    '-Werror=logical-op',
    '-Werror=pointer-arith',
    '-Werror=sign-conversion',
    '-Werror=overlength-strings',
    '-Werror=stringop-truncation',
    '-Werror=missing-declarations',
    #'-Werror=redundant-decls',
    '-Werror=shadow',
    '-Werror=undef',
    '-Werror=unused',
    '-Werror=strict-aliasing',
    '-fstrict-aliasing',
    #'-Werror=strict-overflow',
    '-Wstrict-overflow=2',
    '-fstrict-overflow',
    '-Werror=duplicated-branches',
    '-Werror=duplicated-cond',
    '-Werror=null-dereference',
    '-Werror=init-self',
    '-Werror=jump-misses-init',
    '-Werror=missing-prototypes',
    '-Werror=nested-externs',
    '-Werror=write-strings',
    '-Werror=implicit-fallthrough',
    '-Werror=sign-compare',
    '-Werror=discarded-qualifiers',
    '-Werror=float-conversion',
    '-Werror=implicit-function-declaration',
    '-Werror=uninitialized',
    '-Werror=maybe-uninitialized',
    '-Werror=return-type',
    '-Werror=int-conversion',
    '-Werror=format-security',
    '-Werror=incompatible-pointer-types',
    '-Werror=implicit-int',
    '-Werror=multistatement-macros',
    '-Werror=switch',
    '-Werror=overflow',
    '-Werror=array-bounds',
    '-Werror=enum-compare',
    '-Werror=misleading-indentation',
    '-Werror=int-in-bool-context',
    '-Werror=type-limits',
    '-Werror=deprecated-declarations',
    '-Werror=format-extra-args',
    '-Werror=format',
    ])
  if cc.get_id() == 'gcc'
    common_cflags += cc.get_supported_arguments([
      '-Wextra',
      '-Weverything',
      ])
  endif
endif

add_project_arguments (
  common_cflags,
  language: [ 'c' ]
  )

zlfo_dsp_lib = shared_library (
  'zlfo_dsp',
  name_prefix: '',
  sources: [
    'zlfo.c',
    ],
  dependencies: zlfo_deps,
  include_directories: '.',
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
  include_directories: '.',
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
  include_directories: '.',
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

# install resources
install_subdir (
  'resources', install_dir: zlfodir)
