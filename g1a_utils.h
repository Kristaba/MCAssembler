/**************************************************************************
 	This file is part of Minimalist Casio Assembler (MCA).

    MCA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MCA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MCA.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/


/**
  * g1a_utils.h provide some function to work on G1A files
  * 
  * Special thanks to Andreas Bertheussen and Simon Lothar for their great reverse engineering
  * of the fx9860g calculators!
  */

#ifndef G1A_UTILS_H
#define G1A_UTILS_H

#ifndef TESTING_PC
#include <fxlib.h>
#else
#include "tests/fxlib.h"
#endif

struct G1A_Header;


/**
  * Return a constant pointer to the default icon bitmap array
  */
const unsigned char *defaultIconG1A();


/**
  * This function is a C port of Andreas' g1awrapper tool...
  * Write a G1A file using the content of the file named bin, into the newly created
  * outputname. Use the other arguments for setting the G1A header.
  * icon must be an array of 68 bytes containing the bitmap of your menu icon (30*17 pxl)
  * Return 0 if the function suceed, or an error code (negative value) else.
  * Error codes :
  * -1 : the bin file can't be opened
  * -2 : error when reading the bin file
  * -3 : the output file can't be opened
  * -4 : error when writing the output file
  */
int writeG1AFromFile(const FONTCHARACTER *bin, const FONTCHARACTER *outputname, const unsigned char *icon, const char *name);


/**
  * Same as writeG1AFromFile except the binary is given as an array and no as a file.
  * Error codes :
  * -1 : the output file can't be opened
  * -2 : error when writing the output file
  */
int writeG1AFromBuffer(unsigned char *bin, unsigned int binsize, const FONTCHARACTER *outputname, const unsigned char *icon, const char *name);


/**
  * Fill the given G1A_Header structure using the other informations given in arguments.
  */
void fillG1AHeader(struct G1A_Header *header, const unsigned char *icon, const char *name, unsigned int binsize);

#endif
