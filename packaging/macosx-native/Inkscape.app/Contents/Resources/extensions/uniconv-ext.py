#!/usr/bin/env python

"""
uniconv-ext.py
Python script for running UniConvertor in Inkscape extensions

Copyright (C) 2008 Stephen Silver

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
"""

import sys
from run_command import run
from subprocess import Popen, PIPE

cmd = 'none'

try:
    p = Popen('uniconv', shell=True, stdout=PIPE, stderr=PIPE).wait()
    if p!=127 : cmd = 'uniconv'
    p = Popen('uniconvertor', shell=True, stdout=PIPE, stderr=PIPE).wait()
    if p!=127 : cmd = 'uniconvertor'
except ImportError:
    from popen2 import Popen3
    p = Popen3('uniconv', True).wait()
    if p!=32512 : cmd = 'uniconv'
    p = Popen3('uniconvertor', True).wait()
    if p!=32512 : cmd = 'uniconvertor'

if cmd == 'none' :
    sys.stderr.write('You need to install the UniConvertor software.\n'+\
                     'For GNU/Linux: install the packge python-uniconvertor.\n'+\
                     'For Windows: download it from\n'+\
                     'http://sk1project.org/modules.php?name=Products&product=uniconvertor\n')
else :
    run((cmd+' "%s" "%%s"') % sys.argv[1].replace("%","%%"), "UniConvertor")

# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 encoding=utf-8 textwidth=99
