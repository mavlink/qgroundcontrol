#ifdef shell
gcc -o ${0//.c/} $0 -lxbee -g
exit
}
#endif
/*
  libxbee - a C library to aid the use of Digi's Series 1 XBee modules
            running in API mode (AP=2).

  Copyright (C) 2009  Attie Grande (attie@attie.co.uk)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* this sample will show you how to seup the xbee and ensure that it is in API mode */

#include <stdio.h>
#include <stdlib.h>
#include <xbee.h>

int main(int argc, char *argv[]) {
  /* the extra arguments are the CC ('+' by default) and GT (1000) by default AT values */
  xbee_setuplogAPI("/dev/ttyUSB0",57600,2,'+',1000);

  /* now we can do our stuff! */
  sleep(10);

  /* calling xbee_end() will return the xbee to its previous API mode */
  xbee_end();
  return 0;
}
