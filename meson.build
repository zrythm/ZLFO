# Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
  version: '0.1.3',
  license: 'AGPLv3+',
  meson_version: '>= 0.43.0',
  default_options: [
    'warning_level=2',
    'buildtype=debug',
    'c_std=gnu11',
    ],
  )

cc = meson.get_compiler ('c')

# detect os
os_darwin = false
os_linux = false
os_freebsd = false
os_windows = false

if host_machine.system() == 'darwin'
  os_darwin = true
elif host_machine.system() == 'linux'
  os_linux = true
elif host_machine.system() == 'freebsd'
  os_freebsd = true
elif host_machine.system() == 'windows'
  os_windows = true
endif

prefix = get_option('prefix')
lv2dir = join_paths(prefix, get_option('lv2dir'))
zlfodir = join_paths(lv2dir, 'ZLFO.lv2')

zlfo_cdata = configuration_data ()
zlfo_cdata.set (
  'PROJECT_URI',
  'https://www.zrythm.org/plugins')
zlfo_cdata.set (
  'LFO_URI',
  zlfo_cdata.get ('PROJECT_URI') + '/ZLFO')
zlfo_cdata.set (
  'LFO_UI_URI',
  zlfo_cdata.get ('LFO_URI') + '#UI')
if os_windows
  zlfo_cdata.set ('LFO_DSP_BINARY', 'zlfo_dsp.dll')
  zlfo_cdata.set ('LFO_UI_BINARY', 'zlfo_ui.dll')
else
  zlfo_cdata.set ('LFO_DSP_BINARY', 'zlfo_dsp.so')
  zlfo_cdata.set ('LFO_UI_BINARY', 'zlfo_ui.so')
endif
zlfo_cdata.set ('LFO_TTL', 'zlfo.ttl')

config_h_data = configuration_data ()
config_h_data.set_quoted (
  'PROJECT_URI',
  zlfo_cdata.get ('PROJECT_URI'))
config_h_data.set_quoted (
  'LFO_URI',
  zlfo_cdata.get ('LFO_URI'))
config_h_data.set_quoted (
  'LFO_UI_URI',
  zlfo_cdata.get ('LFO_UI_URI'))
config_h_data.set_quoted (
  'INSTALL_PATH', zlfodir)
if get_option('buildtype') == 'release'
  config_h_data.set ('RELEASE', 1)
endif
config_h_data.set_quoted (
  'ZLFO_VERSION', meson.project_version())

# create config.h
zlfo_config_h = configure_file (
  output: 'config.h',
  configuration: config_h_data,
  )
zlfo_config_h_dep = declare_dependency (
  sources: zlfo_config_h,
  )

lv2_dep = dependency (
  'lv2', version: '>=1.16.0', required: false)
if not lv2_dep.found()
  lilv_proj = subproject('lilv')
  lv2_dep = lilv_proj.get_variable('lv2_dep')
endif

zlfo_deps = [
  zlfo_config_h_dep,
  cc.find_library ('m'),
  lv2_dep,
  ]
ztoolkit_dep = dependency(
  'ztoolkit', version: '== 0.1.1',
  fallback: ['ztoolkit', 'ztoolkit_dep'],
  default_options: ['enable_rsvg=true'])

# cflags
common_cflags = cc.get_supported_arguments([
  '-fvisibility=hidden',
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

if os_windows
  common_cflags += [
    '-D_WOE32=1',
    ]
endif

add_project_arguments (
  common_cflags,
  language: [ 'c' ]
  )

# install resources
install_subdir (
  'resources', install_dir: zlfodir)

subdir('src')
subdir('tests')
