                        /*** /

This file is part of Golly, a Game of Life Simulator.
Copyright (C) 2008 Andrew Trevorrow and Tomas Rokicki.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 Web site:  http://sourceforge.net/projects/golly
 Authors:   rokicki@gmail.com  andrew@trevorrow.com

                        / ***/
#include "slifealgo.h"

// AKT: for case-insensitive string comparison
#include <string.h>
#ifndef WIN32
   #define stricmp strcasecmp
#endif

// AKT: this algo only supports B3/S23
const char SLOW_RULE[] = "B3/S23";

const char* slifealgo::setrule(const char *s) {
   if (stricmp(s, SLOW_RULE) == 0) {
      ghashbase::setrule(s) ;
      maxCellStates = 2 ;
      return NULL;
   }
   return "This algorithm only supports a single rule (B3/S23).";
}

const char* slifealgo::getrule() {
   return SLOW_RULE;
}

const char* slifealgo::DefaultRule() {
   return SLOW_RULE;
}

using namespace std ;
slifealgo::slifealgo() {
   maxCellStates = 2 ;
}
slifealgo::~slifealgo() {
}
state slifealgo::slowcalc(state nw, state n, state ne, state w, state c,
                          state e, state sw, state s, state se) {
  int sum = nw + n + ne + w + e + sw + s + se ;
  if (sum == 3 || (sum == 2 && c == 1))
    return 1 ;
  else
    return 0 ;
}
static lifealgo *creator() { return new slifealgo() ; }
void slifealgo::doInitializeAlgoInfo(staticAlgoInfo &ai) {
   ghashbase::doInitializeAlgoInfo(ai) ;
   ai.setAlgorithmName("SlowLife") ;
   ai.setAlgorithmCreator(&creator) ;
}
