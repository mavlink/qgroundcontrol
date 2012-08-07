##
## Unofficial Qt Serial Port Library
##
## Copyright (c) 2010 Inbiza Systems Inc. All rights reserved.
##
## This program is free software: you can redistribute it and/or modify it
## under the terms of the GNU Lesser General Public License as published by the
## Free Software Foundation, either version 3 of the License, or (at your
## option) any later version.
##
## This program is distributed in the hope that it will be useful, but WITHOUT
## ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
## FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
## more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>
##
##
## @file qserialport.pro
## www.inbiza.com
##

TEMPLATE = subdirs
SUBDIRS = sub_src sub_examples sub_unittest
CONFIG += ordered

sub_src.subdir = src
sub_examples.subdir = examples
sub_examples.depends = sub_src
sub_unittest.subdir = unittest
sub_unittest.depends = sub_src

include(conf.pri)

!isEmpty(QSERIALPORT_NO_TESTS) {
  SUBDIRS -= sub_unittest
}

unix: {
  # API documentation
  doc.commands += doxygen && cd apidocs/html && ./installdox -lqt.tag@http://doc.trolltech.com/4.7 && cd ../..
  doc.target = doc
  QMAKE_EXTRA_TARGETS += doc

  # unittest
  test.commands += cd unittest && make test && cd ..
  QMAKE_EXTRA_TARGETS += test
}

