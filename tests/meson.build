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

lv2lint = find_program ('lv2lint', required: false)
lv2_validate = find_program (
  'lv2_validate', required: false)
sord_validate = find_program (
  'sord_validate', required: false)

if (lv2lint.found())
  test (
    'LV2 lint', lv2lint,
    env: ['LV2_PATH=' + zlfo_build_dir + '/'],
    args: [
      '-I', zlfo_build_dir + '/',
      zlfo_cdata.get ('LFO_URI')])
endif

if lv2_validate.found() and sord_validate.found()
  test (
    'LV2 validate', lv2_validate,
    args: zlfo_ttl)
endif
