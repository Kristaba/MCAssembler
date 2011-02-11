#ifndef MCA_LINKER_H
#define MCA_LINKER_H

#ifndef TESTING_PC
#include <fxlib.h>
#else
#include "tests/fxlib.h"
#endif


// outformat values :
#define FORMAT_BINARY	0
#define FORMAT_G1A		1


/**
  * linkobjfiles() is the primordial function of the Minimalist Object Format (MOF) linker.
  * This function take a list of MOF files and link them into a single binary file.
  * The output file is *not* an object file, it's a fully binary file, so you can't add anything
  * to it later.
  * The output format (only binary or G1A file) can be set using outformat argument.
  * For now, you can't set manually the memories and sections mapping. Only the most used sections
  * are pre-defined for convenience (.pretext, .text, .rodata, .data, .bss and .comment)
  * Also, the files order has a great importance for their data positions (so place initialisations
  * files in first).
  * See MOF-v01.txt for more informations about the Minimalist Object Format.
  *
  * Return value :
  *    0 : sucess
  *   -1 : error when opening an input file
  *   -2 : error when reading an input file
  *   -3 : bad file header
  *   -4 : MOF version not supported
  *   -5 : malloc error
  *   -6 : incoherent informations in the file (corrupted?)
  *   -7 : error when opening the output file
  *   -8 : error when writing the output file
  *
  * -100 : 2 symbols with the same name
  * -101 : unknown section used (not defined in the sections mapping)
  * -102 : a section is mapped to an unexisting memory
  * -103 : a destination memory is undersized (section data out of corresponding memory range)
  * -104 : reference to a symbol not found
  * 
  */
int linkobjfiles(int filesnum, FONTCHARACTER **files, FONTCHARACTER *out, int outformat);

#endif //MCA_LINKER_H
