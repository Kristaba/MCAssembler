#ifndef MCA_ASSEMBLER_H
#define MCA_ASSEMBLER_H

#ifndef TESTING_PC
#include <fxlib.h>
#else
#include "tests/fxlib.h"
#endif


/**
  * This function is the most important and usefull fonction of the assembler.
  * It write an object file (using a special minimalist format descibed below) that's  
  * the result of the 'in' assembly file compilation.
  * Currently, the Minimalist Object Format (MOF) version used is 0.1
  * For more informations about this format, see MOF-v01.txt
  *
  * Return 0 if sucess, a negative error value else :
  *   -1 : can't open the input file
  *   -2 : not enought memory or malloc error
  *   -3 : too many sections
  *   -4 : can't open the output file
  *   -5 : error when writing output file
  *
  * -100 : a symbol name start by a digit
  * -101 : unknow opcode
  * -102 : illegal character in opcode line
  * -103 : unknow directive
  * -104 : misc syntax error (unable to be more helpful)
  * -105 : a number have too many digit
  * -106 : code or directive outside any section
  * -107 : expected a quoted string
  * -108 : too many sections
  * -109 : expected a end of line or a comment
  * -110 : expected a valid symbol name
  * -111 : expected a comma
  * -112 : invalid number (too large)
  * -113 : invalid number (no known patern match)
  * -114 : the given symbol doesn't exist yet
  * 
  * -200 : a label is too far
  * -201 : a symbol doesn't exist or it isn't defined as external
  * -202 : two symbols are defined with the same name
  * -203 : a label or external symbol can't be used after a .byte or .word directive
  * -204 : an external symbol *can't* be flaged as global
  *
  * (Opcode line error)
  * -300 : unknow opcode error
  * -301 : illegal character
  * -302 : more than 2 arguments
  * -303 : unknow argument addressing mode
  * -304 : a number can't be converted into integer (too large)
  * -305 : a comma is missing between two arguments
  * -320 : this opcode don't exist
  *
  * -999 : the never-hapend error (if happen, the problem come from my code, not from user input)
  *
  */
int asm2objfile(FONTCHARACTER *in, FONTCHARACTER *out);


#endif //MCA_ASSEMBLER_H
