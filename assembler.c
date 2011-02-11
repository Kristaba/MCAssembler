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


#include "assembler.h"
#include "assembler_private.h"
#include "opcodes.h"

#ifndef TESTING_PC
#include <fxstdio.h>
#else
#include <stdio.h>
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>



#define COUNT_STEP		0
#define SYMBOLS_STEP	1
#define OPCODE_STEP		2
#define FIND_ABS_STEP	3
#define STEP_NUMBER		4

// Flag used in the symbols_flags array
#define SYMFLAG_NOTHING		0x00
#define SYMFLAG_EXTERN		0x01
#define SYMFLAG_GLOBAL		0x02
#define SYMFLAG_LABEL		0x04
#define SYMFLAG_CONSTVAL	0x08
#define SYMFLAG_EXT_USED	0x10

#define MAX_SECTIONS 20
#define SIZEOF_SECTIONDATA_4 (sizeof(int))

struct _SectionData {
	char *name;
	int data_size;
	int data_pos; // in bytes, *not* in instruction number (words)
	unsigned char *data;
	// To store all the 'absolute' address positions (the object file need that) :
	int abs_size;
	int abs_pos;  // current position in abs_address array
	unsigned int *abs_address;  // address in the section where a .long data must be converted to absolute address
	unsigned char *abs_section_id;  // the section associated with the address
	// to store external symbols informations of this section
	int ext_size;
	int ext_pos;
	unsigned int *ext_address;
	int *ext_symbols_id;   // the symbol ID of each external symbol
};


#define SIZEOF_SYMBOLDATA_1 (sizeof(unsigned char) + sizeof(unsigned char))
#define SIZEOF_SYMBOLDATA_4 (sizeof(char*)+sizeof(unsigned int))


// Idée : pour DO_STEP, faire comme si on ne connaissait aucun symbol pour les opcodes
// (les remplacer par 0), et enregistrer une table de correspondance label->position
// pour SYMBOL_STEP, parser le fichier à la recherche d'opcodes contenant un Label,
// et réécrire chacun d'eux, tout en notant les symboles non résolus


// WARNING : this is a spaghetti function, so don't worry if you see a lot of strange
// branching, as 'continue', and please don't blame me : write a parser in C with these
// constraints isn't a good way to follow 'good programing' conventions ;) 
int asm2objfile(FONTCHARACTER *in, FONTCHARACTER *out) {
	FILE *infile = NULL;
	char str[40];
	char linebuf[200];
	char *line;
	char *tmpline;
	int i;
	
	for(i=0; (i<40) && ((str[i]=(char)in[i]) != 0); i++);
	infile = fopen(str, "r");
	if(infile == NULL) return -1;

	int error = 0;
	int parse_step = 0;
	int current_line = 0;

	// For symbol (label or constant value) computing
	int symbols_number = 0;
	int current_symbol = 0;
	char **symbols_name = NULL;
	unsigned int *symbols_value = NULL;
	// To indicate misc symbols informations (as "external" or "to export") :
	unsigned char *symbols_flags = NULL;
	unsigned char *symbols_section_id = NULL; // only used if the symbol isn't a constant nor an exported symbol

	// For sections informations :
	struct _SectionData *sections[MAX_SECTIONS];
// nothing ?
	sections[0] = calloc(1, sizeof(struct _SectionData));
	sections[0]->name = ".text";
// 0
	int sections_number = 1;
	int current_section = 0;
	
	// Internal memory pool system :
	int mempool_pos_1 = 0;
	int mempool_size_1 = 0;
	void *mempool_1 = NULL;  // memory pool for '.align 1' data (as char)
	int mempool_pos_4 = 0;
	int mempool_size_4 = 0;
	void *mempool_4 = NULL;  // memory pool for '.align 4' data (as int or pointer)
	void *mempool_absdata = NULL;  // special memory pool for absolute address informations
	void *mempool_extdata = NULL;  // special memory pool for external address informations
	

	int line_number;

	while(parse_step<STEP_NUMBER && !error) {
		line_number = 0;
		while(fgets(linebuf, 200, infile)!=NULL && !error) {
			int tmp;
			
			line_number++;
			
			line = linebuf;
			SKIP_BLANK(line);

			// If the line is a comment or is empty, go next line
			if(line[0]=='!' || line[0]==';' || line[0]=='\n' || line[0]==0) continue;

			// Is the line a label?
			for(i=0; i<40 && isalnum(line[i]) || line[i]=='_'; i++);
			if(i<40 && line[i]==':') {
				// A label name can't start by a digit (0~9)
				if(isdigit(line[0])) {
					error = -100;
					continue;
				}
				if(sections_number <= 0) {
					error = -106;
					continue;
				}

				tmpline = (char*)((int)line+i+1);
				SKIP_BLANK(tmpline);
				// Nothing else a end of line/string or a comment after a label
				if(tmpline[0]!='!' && tmpline[0]!=';' && tmpline[0]!='\n' && tmpline[0]!=0) {
					error = -109;
					continue;
				}

				if(parse_step == COUNT_STEP) {
					mempool_size_1 += i+1+SIZEOF_SYMBOLDATA_1;
					mempool_size_4 += SIZEOF_SYMBOLDATA_4;
					symbols_number++;
				}
				else if(parse_step == SYMBOLS_STEP) {
					memcpy(str, line, i);
					str[i]=0;
					int endstr=i;
					// Check if the symbol doesn't exist yet
					for(i=0; i<current_symbol && !(strcmp(str, symbols_name[i])==0); i++);
					if(i<current_symbol) {
						error = -202;
						continue;
					}
					symbols_name[current_symbol] = (char*)((int)mempool_1+mempool_pos_1);
					symbols_value[current_symbol] = sections[current_section]->data_pos;
					symbols_flags[current_symbol] = SYMFLAG_LABEL;
					symbols_section_id[current_symbol] = current_section;
					memcpy((char*)((int)mempool_1+mempool_pos_1), line, endstr);
					mempool_pos_1 += endstr+1; // don't forget the '\0' !
					current_symbol++;
				}
				continue;
			}

			// Is the line an assembler directive?
			if(line[0]=='.') {
				for(i=0; i<40 && !IS_SEP(line[i], ','); i++) str[i]=line[i];
				if (i>=40) {
					error = -103;
					continue;
				}
				str[i] = 0;

				char *tmpstr = (char*)((int)line + i);
				SKIP_BLANK(tmpstr);

				int numbersize = 0;
				if(strcmp(str, ".byte")==0) numbersize = 8;
				else if(strcmp(str, ".word")==0) numbersize = 16;
				else if(strcmp(str, ".long")==0) numbersize = 32;

				else if(strcmp(str, ".align")==0) {
					int alignsize;

					if(sections_number <= 0) {
						error = -106;
						continue;
					}
					
					if(tmpstr[0]=='1') continue; // ".align 1" does nothing...
					else if(tmpstr[0]=='2') alignsize = 2;
					else if(tmpstr[0]=='4') alignsize = 4;
					else {
						error = -104;
					}

					int aligndelta;
					if(parse_step <= COUNT_STEP) aligndelta = alignsize-(sections[current_section]->data_size % alignsize);
					else aligndelta = alignsize-(sections[current_section]->data_pos % alignsize);
					if(aligndelta == alignsize) aligndelta=0;

					if(parse_step==COUNT_STEP) {
						sections[current_section]->data_size += aligndelta;
						mempool_size_1 += aligndelta;
					}
					else if(parse_step==SYMBOLS_STEP || parse_step==FIND_ABS_STEP) sections[current_section]->data_pos += aligndelta;
					else if(parse_step==OPCODE_STEP) {
						unsigned char *tmpdata = (unsigned char*)((int)(sections[current_section]->data) + sections[current_section]->data_pos);
						for(i=0; i<aligndelta; i++) tmpdata[i]=0x00;
						sections[current_section]->data_pos += aligndelta;
					}

					continue;
				}

				else if(strcmp(str, ".section")==0) {
					if(tmpstr[0]!='"') {
						error = -107;
						continue;
					}
					for(i=0; i<40 && tmpstr[i+1]!='"' && tmpstr[i+1]!=0 && tmpstr[i+1]!='\n'; i++) str[i]=tmpstr[i+1];
						int endstr=i;
					if(i>=40 || tmpstr[i+1]!='"') {
						error = -107;
						continue;
					}
					str[i]=0;
					int namesize = i;

					// Check if the section doesn't exist yet
					for(i=0; i<sections_number && !(strcmp(sections[i]->name, str)==0); i++);
					
					if(i >= sections_number) {
						// New section
						if(parse_step != COUNT_STEP) {
							error = -104;
							continue;
						}
						if(sections_number >= MAX_SECTIONS) {
							error = -108;
							continue;
						}
						sections[sections_number] = calloc(sizeof(struct _SectionData)+namesize+1, sizeof(char));
						if(sections[sections_number] == NULL) {
							error = -2;
							continue;
						}
						char *nameptr = (char*)((int)(sections[sections_number])+sizeof(struct _SectionData));
						memcpy(nameptr, str, namesize+1);
						sections[sections_number]->name = nameptr;
						current_section = sections_number;
						sections_number++;
					}
					else {
						// A section with the same name already exist
						current_section = i;
					}
					continue;
				}

				else if(strcmp(str, ".global")==0) {
					if(parse_step == OPCODE_STEP) {

						line = tmpstr;
						for(i=0; i<40 && isalnum(line[i]) || line[i]=='_'; i++);
						if(i>=40 || !IS_SEP(line[i], ',')) {
							error = -110;
							continue;
						}

						memcpy(str, line, i);
						str[i]=0;
						for(i=0; i<symbols_number && !(strcmp(str, symbols_name[i])==0); i++);
						if(i<symbols_number) {
							if(symbols_flags[i] & SYMFLAG_EXTERN) {
								error = -204;
								continue;
							}
							symbols_flags[i] |= SYMFLAG_GLOBAL;
							continue;
						}
						else {
							error = -114;
							continue;
						}
					}
					else continue;
				}


				else if(strcmp(str, ".extern")==0) {
					line = tmpstr;
					for(i=0; i<40 && isalnum(line[i]) || line[i]=='_'; i++);
					if(i>=40 || !IS_SEP(line[i], ',')) {
						error = -110;
						continue;
					}
					// A label name can't start by a digit (0~9)
					if(isdigit(line[0])) {
						error = -100;
						continue;
					}

					if(parse_step == COUNT_STEP) {
						mempool_size_1 += i+1+SIZEOF_SYMBOLDATA_1;
						mempool_size_4 += SIZEOF_SYMBOLDATA_4;
						symbols_number++;
					}
					else if(parse_step == SYMBOLS_STEP) {
						int endstr = i;
						memcpy(str, line, endstr);
						str[endstr]=0;

						for(i=0; i<current_symbol && !(strcmp(str, symbols_name[i])==0); i++);
						if(i<current_symbol) {
							error = -202;
							continue;
						}
						else {
							symbols_name[current_symbol] = (char*)((int)mempool_1+mempool_pos_1);
							symbols_value[current_symbol] = 0;
							symbols_flags[current_symbol] = SYMFLAG_EXTERN;
							symbols_section_id[current_symbol] = 0xFF;
							memcpy((char*)((int)mempool_1+mempool_pos_1), line, endstr);
							mempool_pos_1 += endstr+1; // don't forget the '\0' !
							current_symbol++;
						}
					}
					continue;
				}

				else if(strcmp(str, ".equ")==0 || strcmp(str, ".set")==0) {
					// TODO for now, .equ and .set can't modifiy an existing symbol (only can created new one)
					// because of the memory pool system... So find a solution?
					line = tmpstr;
					for(i=0; i<40 && isalnum(line[i]) || line[i]=='_'; i++);
					if(i>=40 || !IS_SEP(line[i], ',')) {
						error = -110;
						continue;
					}
					// A label name can't start by a digit (0~9)
					if(isdigit(line[0])) {
						error = -100;
						continue;
					}

					if(parse_step == COUNT_STEP) {
						mempool_size_1 += i+1+SIZEOF_SYMBOLDATA_1;
						mempool_size_4 += SIZEOF_SYMBOLDATA_4;
						symbols_number++;
					}
					else if(parse_step == SYMBOLS_STEP || parse_step == OPCODE_STEP) {
						int endstr=i;

						// Get the new symbol's value :
						tmpline = (char*)((int)line+i);
						SKIP_BLANK(tmpline);
						if(tmpline[0] != ',') {
							error = -111;
							continue;
						}
						tmpline = (char*)((int)tmpline+1);
						SKIP_BLANK(tmpline);

						for(i=0; i<40 && !IS_SEP(tmpline[i], ','); i++) str[i]=tmpline[i];
						if (i>=40) {
							error = -105;
							continue;
						}
						str[i] = 0;

						tmpline = (char*)((int)tmpline + i);
						SKIP_BLANK(tmpline);
						if(tmpline[0]!='\n' && tmpline[0]!='\n' && tmpline[0]!=';' && tmpline[0]!='!') {
							error = -109;
							continue;
						}

						int value = 0;
						int stnret = stringToNumber(str, &value);
						if(stnret < 0) {
							if(stnret==-2) error = -112;
							else error = -113;
							continue;
						}

						// now, look for an existing symbol with the same name
						memcpy(str, line, endstr);
						str[endstr]=0;

						int maxnumber = (parse_step==SYMBOLS_STEP ? current_symbol : symbols_number);
						for(i=0; i<maxnumber && !(strcmp(str, symbols_name[i])==0); i++);
						if(i<maxnumber) {
							// For now, no overwriting value, in this case return error...
							//symbols_value[i] = value;
							if(parse_step == SYMBOLS_STEP) error = -202;
							continue;
						}
						else {
							if(parse_step == OPCODE_STEP) {
								error = -999;
								continue;
							}
							symbols_name[current_symbol] = (char*)((int)mempool_1+mempool_pos_1);
							symbols_value[current_symbol] = value;
							symbols_flags[current_symbol] = SYMFLAG_CONSTVAL;
							symbols_section_id[current_symbol] = 0xFF;
							memcpy((char*)((int)mempool_1+mempool_pos_1), line, endstr);
							mempool_pos_1 += endstr+1; // don't forget the '\0' !
							current_symbol++;
						}
					}
					continue;
				}

				// If the directive is .byte, .word or .long :
				if(numbersize > 0) {
					if(sections_number <= 0) {
						error = -106;
						continue;
					}

					if(parse_step==COUNT_STEP) {
						sections[current_section]->data_size += numbersize>>3;
						mempool_size_1 += numbersize>>3;
					}
					else if(parse_step==SYMBOLS_STEP) sections[current_section]->data_pos += numbersize>>3;
					else if(parse_step==OPCODE_STEP || parse_step==FIND_ABS_STEP) {
						int value;
						int value2;

						for(i=0; i<40 && !IS_SEP(tmpstr[i], ','); i++) str[i]=tmpstr[i];
						if (i>=40) {
							error = -103;
							continue;
						}
						str[i] = 0;

						tmpstr = (char*)((int)tmpstr + i);
						SKIP_BLANK(tmpstr);
						if(tmpstr[0]!='\n' && tmpstr[0]!='\n' && tmpstr[0]!=';' && tmpstr[0]!='!') {
							error = -104;
							continue;
						}

						int stnret = stringToNumber(str, &value);

						if(stnret < 0) {
							// Maybe it's a symbol address?
							for(i=0; i<symbols_number && !(strcmp(symbols_name[i], str)==0); i++);
							if(i >= symbols_number) {
								error = -104;
								continue;
							}

							if(numbersize == 32) { // Only for .long
								if(!(symbols_flags[i] & SYMFLAG_EXTERN)) {
									value = (int)symbols_value[i];
									if(symbols_flags[i] & SYMFLAG_LABEL) {
										// If the symbol is a non extern label, need to add this address to the 
										// positions of the section where a .long have to be "absolute extended"
										if(parse_step==OPCODE_STEP) sections[current_section]->abs_size++;
										else if(parse_step==FIND_ABS_STEP) {
											int abspos = sections[current_section]->abs_pos;
											sections[current_section]->abs_address[abspos] = sections[current_section]->data_pos;
											sections[current_section]->abs_section_id[abspos] = symbols_section_id[i];
											sections[current_section]->abs_pos++;
										}
									}
								}
								else {
									// If the symbol is external :
									if(parse_step==OPCODE_STEP) sections[current_section]->ext_size++;
									else if(parse_step==FIND_ABS_STEP) {
										int extpos = sections[current_section]->ext_pos;
										sections[current_section]->ext_address[extpos] = sections[current_section]->data_pos;
										sections[current_section]->ext_symbols_id[extpos] = i;
										sections[current_section]->ext_pos++;
									}
									value = 0;
								}
							}
							else {
								if(!(symbols_flags[i] & SYMFLAG_CONSTVAL) || (symbols_flags[i] & SYMFLAG_EXTERN)) {
									error = -203;
									continue;
								}
								value = (int)symbols_value[i];
							}
						}

						// if it's the FIND_ABS_STEP step, don't copy to data...
						if(parse_step == FIND_ABS_STEP) {
							sections[current_section]->data_pos += numbersize>>3;
							continue;
						}

						// TODO warning system
						/*else if(stnret == 1) {
							if(numbersize == 8) value2 = (char)value;
							else if(numbersize == 8) value2 = (short)value;
							else value2 = value;
						}
						else {
							if(numbersize == 8) value2 = (unsigned char)value;
							else if(numbersize == 8) value2 = (unsigned short)value;
							else value2 = value;
						}
						if(value2 != value) {
							// Put a warning : the number is too large for this usage
						}
						*/

						unsigned char *tmpdata = (unsigned char*)((int)(sections[current_section]->data) + sections[current_section]->data_pos);
						// byte
						if(numbersize == 8) {
							tmpdata[0] = (char)value;
							sections[current_section]->data_pos++;
						}
						//word (*big* endian)
						else if (numbersize == 16) {
							tmpdata[0] = (char)(value >> 8);
							tmpdata[1] = (char)value;
							sections[current_section]->data_pos += 2;
						}
						//long word (*big* endian)
						else {
							tmpdata[0] = (char)(value >> 24);
							tmpdata[1] = (char)(value >> 16);
							tmpdata[2] = (char)(value >> 8);
							tmpdata[3] = (char)value;
							sections[current_section]->data_pos += 4;
						}
					}
					continue;
				}


				error = -103;
				continue;
			}


			// Now the line is inevitably an opcode :
			if(sections_number <= 0) {
				error = -106;
				continue;
			}
			// Here, considerate all these lines are valid opcodes (2 bytes)
			// If there is any error, the OPCODE_STEP step will return an error code later.
			if(parse_step == SYMBOLS_STEP || parse_step == FIND_ABS_STEP) {
				sections[current_section]->data_pos += 2;
				continue;
			}	
			else if(parse_step == COUNT_STEP) {
				sections[current_section]->data_size += 2;
				mempool_size_1 += 2;
				continue;
			}
			else if(parse_step == OPCODE_STEP) {
				struct ParsedOpcode opcode;
				int opret;
				opret = parseOpcodeLine(line, &opcode);
				if(opret<0) {
					if(opret >= -5) error = -300 + opret;
					else error = -300;
					continue;
				}

				// transform arguments of type "Label" into a PC-displacement argument
				if(opcode.args[0].type == ARG_TYPE_LABEL) {
					for(i=0; i<symbols_number; i++) {
						if(strcmp((char*)(opcode.args[0].data), symbols_name[i])==0) break;
					}
					if((i >= symbols_number) || (symbols_section_id[i] != current_section)) {
						error = -200;
						continue;
					}
					int disp;
					if(tolower(opcode.mnemonic[0])=='m' && tolower(opcode.mnemonic[1])=='o' && tolower(opcode.mnemonic[2])=='v'
								&& opcode.mnemonic[3]=='.' && tolower(opcode.mnemonic[4])=='l' && opcode.mnemonic[5]==0 ) {
						disp = symbols_value[i] - (sections[current_section]->data_pos & 0xFFFFFFFC) -4;
					}
					else disp = symbols_value[i] - sections[current_section]->data_pos -4;
					// TODO check if the label isn't too far (here or not?)
					opcode.args[0].type = ARG_TYPE_DISP_PC;
					opcode.args[0].data = disp;
				} 

				unsigned short opvalue;
				opret = opcodeValue(&opcode, &opvalue);
				if(opret<0) {
					error = -320;
					continue;
				}
				sections[current_section]->data[sections[current_section]->data_pos] = (char)(opvalue >> 8);
				sections[current_section]->data[sections[current_section]->data_pos+1] = (char)opvalue;
				sections[current_section]->data_pos += 2;
				continue;
			}
			else continue;
	
			error=-101;
		}

		if(!error) if(sections_number > MAX_SECTIONS) error = -3;

		if(!error) {
			current_line = 0;
			current_symbol = 0;
			current_section = 0;
			if(parse_step == COUNT_STEP) {
				mempool_1 = calloc(mempool_size_1, sizeof(char));
				mempool_4 = calloc(mempool_size_4, sizeof(char));
				if((mempool_size_1!=0 && mempool_1==NULL) || (mempool_size_4!=0 && mempool_4==NULL)) {
					error = -2;
					break;
				}
				// Initialize array
				symbols_name = (char**)((int)mempool_4+mempool_pos_4);
				mempool_pos_4 += symbols_number * sizeof(char*);
				symbols_value = (unsigned int*)((int)mempool_4+mempool_pos_4);
				mempool_pos_4 += symbols_number * sizeof(unsigned int);
				symbols_flags = (unsigned char*)((int)mempool_1+mempool_pos_1);
				mempool_pos_1 += symbols_number * sizeof(unsigned char);
				symbols_section_id = (unsigned char*)((int)mempool_1+mempool_pos_1);
				mempool_pos_1 += symbols_number * sizeof(unsigned char);
				//sections = (struct _SectionData*)((int)mempool_4+mempool_pos_4);
				//mempool_pos_4 += sections_number * sizeof(struct _SectionData);
				for(i=0; i<sections_number; i++) {
					sections[i]->data = (unsigned char*)((int)mempool_1+mempool_pos_1);
					mempool_pos_1 += sections[i]->data_size * sizeof(unsigned char);
					//sections[i]->abs_address = (unsigned int*)((int)mempool_4+mempool_pos_4);
					//mempool_pos_4 += sections[i]->abs_size * sizeof(unsigned int);
				}
			}
			else if(parse_step == OPCODE_STEP) {
				int total_abs_size = 0;
				int total_ext_size = 0;
				for(i=0; i<sections_number; i++) {
					total_abs_size += sections[i]->abs_size;
					total_ext_size += sections[i]->ext_size;
				}
				mempool_absdata = calloc(total_abs_size, sizeof(unsigned char) + sizeof(unsigned int));
				mempool_extdata = calloc(total_ext_size, sizeof(int) + sizeof(unsigned int));
				if(((total_abs_size > 0) && (mempool_absdata==NULL)) || ((total_ext_size > 0) && (mempool_extdata==NULL))) {
					error = -2;
					break;
				}
				int abs_pos = 0;
				int ext_pos = 0;
				for(i=0; i<sections_number; i++) {
					sections[i]->abs_address = (unsigned int*)((int)mempool_absdata + abs_pos*4);
					sections[i]->abs_section_id = (unsigned char*)((int)mempool_absdata + abs_pos + total_abs_size*4);
					sections[i]->ext_address = (unsigned int*)((int)mempool_extdata + ext_pos*4);
					sections[i]->ext_symbols_id = (int*)((int)mempool_extdata + ext_pos*4 + total_ext_size*4);
					abs_pos += sections[i]->abs_size;
					ext_pos += sections[i]->ext_size;
				}
			}
			for(i=0; i<sections_number; i++) {
				sections[i]->data_pos = 0;
				sections[i]->abs_pos = 0;
				sections[i]->ext_pos = 0;
			}
			parse_step++;
			fseek(infile, 0, SEEK_SET);
		}
	}

	// File parsed, now write the object file!
	// Because of the fucking buggy Casio API, we have to write the full file in *one* call
	if(!error) {
		// Firstly compute the total size needed :
		int size = 0;
		int header_size = 7 + 4 + 4 + 4;
		size += header_size;
		int compiled_size = 4;
		for(i=0; i<sections_number; i++)
			compiled_size += strlen(sections[i]->name) + 1 + 4 + (sections[i]->abs_size*(4+1)) + 4 + sections[i]->data_size;
		size += compiled_size;
		int imported_size = 4;
		int imported_number = 0;
		for(i=0; i<sections_number; i++) {
			int j;
			imported_size += (1 + 4) * sections[i]->ext_size;  // occurence_section_id + occurence_position
			for(j=0; j<sections[i]->ext_size; j++) {
				int symid = sections[i]->ext_symbols_id[j];
				if((symbols_flags[symid] & SYMFLAG_EXTERN) && !(symbols_flags[symid] & SYMFLAG_EXT_USED)) {
					imported_number++;
					imported_size += strlen(symbols_name[symid]) + 1 + 4;  // name + name_size + occurence_number
					symbols_flags[symid] |= SYMFLAG_EXT_USED;
				}
			}
		}
		size += imported_size;
		int exported_size = 4;
		int exported_number = 0;
		for(i=0; i<symbols_number; i++) {
			if(symbols_flags[i] & SYMFLAG_GLOBAL) {
				exported_size += 1 + strlen(symbols_name[i]) + 1 + 4;
				exported_number++;
			}
		}
		size += exported_size;

		// then create and fill the buffer :
		char *buffer = calloc(size, sizeof(char));
		if(buffer == NULL) {
			error = -2;
		}
		else {
			int pos = 0;
			// Write header
			buffer[0]='M';		buffer[1]='O';		buffer[2]='F';		buffer[3]='-';
			buffer[4]=MOF_MAJOR_VERSION+'0';
			buffer[5]='.';
			buffer[6]=MOF_MINOR_VERSION+'0';
			WRITE_INTEGER(compiled_size, buffer, 7);
			WRITE_INTEGER(imported_size, buffer, 11);
			WRITE_INTEGER(exported_size, buffer, 15);
			pos += 19;

			// Write compiled data
			WRITE_INTEGER(sections_number, buffer, pos);
			pos += 4;
			for(i=0; i<sections_number; i++) {
				buffer[pos] = strlen(sections[i]->name);
				memcpy((char*)((int)buffer + pos+1), sections[i]->name, (unsigned char)(buffer[pos]));
				pos += (unsigned char)(buffer[pos]) + 1;
				WRITE_INTEGER(sections[i]->abs_size, buffer, pos);
				pos += 4;
				int j;
				for(j=0; j<sections[i]->abs_size; j++) {
					buffer[pos] = sections[i]->abs_section_id[j];
					WRITE_INTEGER(sections[i]->abs_address[j], buffer, pos+1);
					pos += 1 + 4;
				}
				WRITE_INTEGER(sections[i]->data_size, buffer, pos);
				memcpy((char*)((int)buffer + pos+4), sections[i]->data, sections[i]->data_size);
				pos += 4 + sections[i]->data_size;
			}

			// Write imported symbols (maked with .extern)
			WRITE_INTEGER(imported_number, buffer, pos);
			pos += 4;
			for(i=0; i<symbols_number; i++) symbols_flags[i] &= ~SYMFLAG_EXT_USED;  // Clear the EXT_USED flag
			for(i=0; i<sections_number; i++) {
				int j;
				for(j=0; j<sections[i]->ext_size; j++) {
					int symid = sections[i]->ext_symbols_id[j];
					unsigned char flags = symbols_flags[symid];
					if((flags & SYMFLAG_EXTERN) && !(flags & SYMFLAG_EXT_USED)) {
						buffer[pos] = strlen(symbols_name[symid]);
						memcpy((char*)((int)buffer + pos+1), symbols_name[symid], (unsigned char)(buffer[pos]));
						pos += (unsigned char)(buffer[pos]) + 1;
						int tmp_pos = pos;
						pos += 4; // the occurence number isn't know at this time
						int occurence_num = 0;
						int k;
						for(k=i; k<sections_number; k++) {
							int l;
							for(l=0; l<sections[k]->ext_size; l++) {
								if(sections[k]->ext_symbols_id[l] == symid) {
									occurence_num++;
									buffer[pos] = k;
									WRITE_INTEGER(sections[k]->ext_address[l], buffer, pos+1);
									pos += 1+4;
								}
							}
						}
						// Back-write the occurence number
						WRITE_INTEGER(occurence_num, buffer, tmp_pos);
						symbols_flags[symid] |= SYMFLAG_EXT_USED;
					}
				}
			}

			// Write exported symbols (marked with .global)
			WRITE_INTEGER(exported_number, buffer, pos);
			pos += 4;
			for(i=0; i<symbols_number; i++) {
				if(symbols_flags[i] & SYMFLAG_GLOBAL) {
					buffer[pos] = strlen(symbols_name[i]);
					memcpy((char*)((int)buffer + pos+1), symbols_name[i], (unsigned char)(buffer[pos]));
					pos += (unsigned char)(buffer[pos]) + 1;
					unsigned char section_id;
					if(symbols_flags[i] & SYMFLAG_CONSTVAL) section_id = 0xFF;
					else section_id = symbols_section_id[i];
					buffer[pos] = section_id;
					WRITE_INTEGER(symbols_value[i], buffer, pos+1);
					pos += 1+4;
				}
			}

#ifndef TESTING_PC
			// Buffer filled, write the real file!
			// Delete and/or create the out file and open it in write mode
			Bfile_DeleteFile(out);
			Bfile_CreateFile(out, size);
			int outputHandle = Bfile_OpenFile(out, _OPENMODE_WRITE);
			if(outputHandle < 0) return -4;
			if(Bfile_WriteFile(outputHandle, buffer, size) < 0) return -5;
			Bfile_CloseFile(outputHandle);
#else
			FILE *outfile = NULL;
			outfile = fopen(out, "wb+");
			if(outfile == NULL) return -4;
			fwrite(buffer, sizeof(unsigned char), size, outfile);
			fflush(outfile);
			fclose(outfile);
#endif
		}
		/*FILE *outfile = NULL;
		outfile = fopen(out, "wb+");
		if(outfile == NULL) return -4;
		fwrite(sections[0]->data, sizeof(unsigned char), sections[0]->data_size, outfile);*/
	}
//	else printf("Error line %d\n", line_number);

	// Now, free the mallocated pointers
	if(mempool_1 != NULL) free(mempool_1);
	if(mempool_4 != NULL) free(mempool_4);
	if(mempool_absdata != NULL) free(mempool_absdata);
	if(mempool_extdata != NULL) free(mempool_extdata);
	for(i=0; i<sections_number; i++) if(sections[i] != NULL) free(sections[i]);
	if(infile != NULL) fclose(infile);
	return error;
}



// This static array is requiered by parseOpcodeLine function to store a label argument
// (I considerate there is no opcode with 2 label argument)
static char g_pol_labelname[40];

int parseOpcodeLine(const char *line, struct ParsedOpcode *opstruct) {
	int i=0;
	char tmpstr[50];

	opstruct->args[0].type=ARG_TYPE_NOTHING;
	opstruct->args[1].type=ARG_TYPE_NOTHING;
	opstruct->args[0].data=0;
	opstruct->args[1].data=0;

	// Don't forget that an opcode may contain '.' or '/'
	for(i=0; (isalnum(opstruct->mnemonic[i]=tolower(line[i])) || line[i]=='/' || line[i]=='.') 
				&& (i<sizeof(opstruct->mnemonic)-1); i++);
	// If the next character isn't a space or a tab, it's an illegal character
	if(line[i]!=' ' && line[i]!='\t' && line[i]!='\n') return -1;
	opstruct->mnemonic[i]=0;

	line = (char*)((int)line + i);
	SKIP_BLANK(line);

	i=0;
	// Get the opcode arguments 
	while(line[0]!='\n' && line[0]!=0 && line[0]!='!' && line[0]!=';') {
		if(i>=2) return -2;

		// Don't forget that the && operator will not evaluate the right part if the left is false!

		// Direct Register :
		if(toupper(line[0])=='R' && isdigit(line[1]) && (IS_SEP(line[2],',') 
					|| (line[1]=='1' && line[2]>='0' && line[2]<='5' && IS_SEP(line[3],',')) ))
		{
			opstruct->args[i].type = ARG_TYPE_D_REG;
			int reg;
			if(isdigit(line[2])) reg = line[2]-'0' + 10;
			else reg = line[1]-'0';
			opstruct->args[i].data = reg;
			line = (char*)((int)line + (reg<10 ? 2 : 3));
		}

		// Indirect Register :
		else if(line[0]=='@' && toupper(line[1])=='R' && isdigit(line[2]) && (IS_SEP(line[3],',') 
					|| (line[2]=='1' && line[3]>='0' && line[3]<='5' && IS_SEP(line[4],',')) ))
		{
			opstruct->args[i].type = ARG_TYPE_I_REG;
			int reg;
			if(isdigit(line[3])) reg = line[3]-'0' + 10;
			else reg = line[2]-'0';
			opstruct->args[i].data = reg;
			line = (char*)((int)line + (reg<10 ? 3 : 4));
		}

		// Indirect Increment Register :
		else if(line[0]=='@' && toupper(line[1])=='R' && isdigit(line[2]) && ((line[3]=='+' && IS_SEP(line[4],','))
					|| (line[2]=='1' && line[3]>='0' && line[3]<='5' && line[4]=='+' && IS_SEP(line[5],',')) ))
		{
			opstruct->args[i].type = ARG_TYPE_INC_REG;
			int reg;
			if(isdigit(line[3])) reg = line[3]-'0' + 10;
			else reg = line[2]-'0';
			opstruct->args[i].data = reg;
			line = (char*)((int)line + (reg<10 ? 4 : 5));
		}

		// Indirect Decrement Register :
		else if(line[0]=='@' && line[1]=='-' && toupper(line[2])=='R' && isdigit(line[3]) && (IS_SEP(line[4],',')
					|| (line[3]=='1' && line[4]>='0' && line[4]<='5' && IS_SEP(line[5],',')) ))
		{
			opstruct->args[i].type = ARG_TYPE_DEC_REG;
			int reg;
			if(isdigit(line[4])) reg = line[4]-'0' + 10;
			else reg = line[3]-'0';
			opstruct->args[i].data = reg;
			line = (char*)((int)line + (reg<10 ? 4 : 5));
		}

		else if(line[0]=='@' && line[1]=='(' ) {
			if(toupper(line[2])=='R' && line[3]=='0' && line[4]==',') {
				// Indirect Indexed GBR :
				if(toupper(line[5])=='G' && toupper(line[6])=='B' && toupper(line[7])=='R' && line[8]==')' 
								&& IS_SEP(line[9], ',') )
				{
					opstruct->args[i].type = ARG_TYPE_INDEX_GBR;
					line = (char*)((int)line + 9);
				}

				// Indirect Indexed Register :
				else if(toupper(line[5])=='R' && isdigit(line[6]) && (line[7]==')' && IS_SEP(line[8],',') 
							|| (line[6]=='1' && line[7]>='0' && line[7]<='5' && line[8]==')' && IS_SEP(line[9],',')) ))
				{
					opstruct->args[i].type = ARG_TYPE_INDEX_REG;
					int reg;
					if(isdigit(line[7])) reg = line[7]-'0' + 10;
					else reg = line[6]-'0';
					opstruct->args[i].data = reg;
					line = (char*)((int)line + (reg<10 ? 8 : 9));
				}
				else return -3;
			}
			else {
				int j, ret, val;
				for(j=0; j<50 && !IS_SEP(line[j+2], ','); j++) tmpstr[j]=line[j+2];
				if(j>=50) return -3;
				tmpstr[j]=0;

				ret = stringToNumber(tmpstr, &val);
				if(ret == -2) return -4;
				else if(ret < 0) return -3;
				char *line2 = (char*)((int)line + 2+j);

				// Indirect PC Displacement
				if(line2[0]==',' && toupper(line2[1])=='P' && toupper(line2[2])=='C' && line2[3]==')' && IS_SEP(line2[4], ',')) {
					opstruct->args[i].type = ARG_TYPE_DISP_PC;
					opstruct->args[i].data = val;
					line = (char*)((int)line2 + 4);
				}

				// Indirect PC Displacement
				else if(line2[0]==',' && toupper(line2[1])=='G' && toupper(line2[2])=='B' && toupper(line2[3])=='R' 
							&& line2[4]==')' && IS_SEP(line2[5], ','))
				{
					opstruct->args[i].type = ARG_TYPE_DISP_GBR;
					opstruct->args[i].data = val;
					line = (char*)((int)line2 + 5);
				}

				// Indirect Register Displacement
				else if(line2[0]==',' && toupper(line2[1])=='R' && isdigit(line2[2]) && ((line2[3]==')' && IS_SEP(line2[4],',')) 
							|| (line2[2]=='1' && line2[3]>='0' && line2[3]<='5' && line2[4]==')' && IS_SEP(line2[5],',')) ))
				{
					opstruct->args[i].type = ARG_TYPE_DISP_REG;
					int reg;
					if(isdigit(line2[3])) reg = line2[3]-'0' + 10;
					else reg = line2[2]-'0';
					// WARNING : in this case (and ONLY this), data contain 2 informations : the 4 LSB
					// 			contain the register ID, and the 28 MSB are the disp value << 4.
					//			For now, there is no error/warning when the value overflow 
					opstruct->args[i].data = reg | (val<<4);
					line = (char*)((int)line2 + (reg<10 ? 4 : 5));
				}
				else return -3;
			}
		}

		// Maybe an Immediate Data
		else if(line[0]=='#') {
			int j, ret, val;
			for(j=0; j<50 && !IS_SEP(line[j+1], ','); j++) tmpstr[j]=line[j+1];
			if(j>=50) return -3;
			tmpstr[j]=0;

			ret = stringToNumber(tmpstr, &val);
			if(ret == -2) return -4;
			else if(ret < 0) return -3;
			opstruct->args[i].type = ARG_TYPE_IMMEDIATE;
			opstruct->args[i].data = val;

			line = (char*)((int)line + 1+j);
		}

		// Test the Specials Registers
		else {
			int j;
			// Don't need to copy more than 8 characters because it's more than the longer
			// control/system register name (R*_BANK)
			for(j=0; j<8 && !IS_SEP(line[j], ','); j++) tmpstr[j]=toupper(line[j]);
			int spereg = 0;
			if(j<8) {
				tmpstr[j]=0;
				if(strcmp(tmpstr, "GBR")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_GBR;
					line = (char*)((int)line + 3);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "SR")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_SR;
					line = (char*)((int)line + 2);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "VBR")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_VBR;
					line = (char*)((int)line + 3);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "SSR")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_SSR;
					line = (char*)((int)line + 3);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "SPC")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_SPC;
					line = (char*)((int)line + 3);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "R0_BANK")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_R0_BANK;
					line = (char*)((int)line + 7);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "R1_BANK")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_R1_BANK;
					line = (char*)((int)line + 7);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "R2_BANK")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_R2_BANK;
					line = (char*)((int)line + 7);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "R3_BANK")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_R3_BANK;
					line = (char*)((int)line + 7);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "R4_BANK")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_R4_BANK;
					line = (char*)((int)line + 7);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "R5_BANK")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_R5_BANK;
					line = (char*)((int)line + 7);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "R6_BANK")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_R6_BANK;
					line = (char*)((int)line + 7);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "R7_BANK")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_R7_BANK;
					line = (char*)((int)line + 7);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "MACH")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_MACH;
					line = (char*)((int)line + 4);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "MACL")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_MACL;
					line = (char*)((int)line + 4);
					spereg = 1;
				}
				else if(strcmp(tmpstr, "PR")==0) {
					opstruct->args[i].type = ARG_TYPE_SPECIAL_REG;
					opstruct->args[i].data = SREG_PR;
					line = (char*)((int)line + 2);
					spereg = 1;
				}
			}

			// The last possibilitie is that's a Label
			if(!spereg && (isalpha(line[0]) || line[0]=='_')) {
				int j;
				for(j=0; isalnum(line[j]) || line[j]=='_'; j++);
				// If the character that stop the loop is a separator, OK, it will be considerate as a label
				if(j>0 && j<40 && IS_SEP(line[j], ',')) {
					opstruct->args[i].type = ARG_TYPE_LABEL;
					int charnum = j;
					for(j=0; j<charnum; j++) g_pol_labelname[j]=line[j];
					g_pol_labelname[j]=0;
					opstruct->args[i].data = (int)g_pol_labelname;
					line = (char*)((int)line + charnum);
				}
				else return -2;
			}

			else if(!spereg) return -3;
		}

		SKIP_BLANK(line);
		if(line[0]==',') line = (char*)((int)line + 1);
		else if(line[0]!='\n' && line[0]!=0 && line[0]!='!' && line[0]!=';') return -5;
		SKIP_BLANK(line);

		i++;
	}

	return 0;
}



// Since this function doesn't use any array with opcode/arguments/values, the code should be pretty
// complicated to maintain and to debug, but I think it's the faster way
int opcodeValue(const struct ParsedOpcode *op, unsigned short *value) {
	int ret=0;
	
	*value=0;
	switch (op->args[0].type) {

	// If no arguments
	case ARG_TYPE_NOTHING:
		if(strcmp(op->mnemonic, "nop")==0) *value=OPC_NOP;
		else if(strcmp(op->mnemonic, "rts")==0) *value=OPC_RTS;
		else if(strcmp(op->mnemonic, "clrmac")==0) *value=OPC_CLRMAC;
		else if(strcmp(op->mnemonic, "clrs")==0) *value=OPC_CLRS;
		else if(strcmp(op->mnemonic, "clrt")==0) *value=OPC_CLRT;
		else if(strcmp(op->mnemonic, "div0u")==0) *value=OPC_DIV0U;
		else if(strcmp(op->mnemonic, "rte")==0) *value=OPC_RTE;
		else if(strcmp(op->mnemonic, "sets")==0) *value=OPC_SETS;
		else if(strcmp(op->mnemonic, "sett")==0) *value=OPC_SETT;
		else if(strcmp(op->mnemonic, "sleep")==0) *value=OPC_SLEEP;
		else if(strcmp(op->mnemonic, "ldtlb")==0) *value=OPC_LDTLB;
		else ret=-1;
		break;

	// If the first argument is a direct register (the most common case)
	case ARG_TYPE_D_REG:
		switch (op->args[1].type) {
		case ARG_TYPE_NOTHING:
			if(strcmp(op->mnemonic, "braf")==0) *value=OPC_BRAF(op->args[0].data);
			else if(strcmp(op->mnemonic, "bsrf")==0) *value=OPC_BSRF(op->args[0].data);
			else if(strcmp(op->mnemonic, "cmp/pl")==0) *value=OPC_CMP_PL(op->args[0].data);
			else if(strcmp(op->mnemonic, "cmp/pz")==0) *value=OPC_CMP_PZ(op->args[0].data);
			else if(strcmp(op->mnemonic, "dt")==0) *value=OPC_DT(op->args[0].data);
			else if(strcmp(op->mnemonic, "movt")==0) *value=OPC_MOVT(op->args[0].data);
			else if(strcmp(op->mnemonic, "rotcr")==0) *value=OPC_ROTCR(op->args[0].data);
			else if(strcmp(op->mnemonic, "rotcl")==0) *value=OPC_ROTCL(op->args[0].data);
			else if(strcmp(op->mnemonic, "rotr")==0) *value=OPC_ROTR(op->args[0].data);
			else if(strcmp(op->mnemonic, "rotl")==0) *value=OPC_ROTL(op->args[0].data);
			else if(strcmp(op->mnemonic, "shal")==0) *value=OPC_SHAL(op->args[0].data);
			else if(strcmp(op->mnemonic, "shar")==0) *value=OPC_SHAR(op->args[0].data);
			else if(strcmp(op->mnemonic, "shll")==0) *value=OPC_SHLL(op->args[0].data);
			else if(strcmp(op->mnemonic, "shll2")==0) *value=OPC_SHLL2(op->args[0].data);
			else if(strcmp(op->mnemonic, "shll8")==0) *value=OPC_SHLL8(op->args[0].data);
			else if(strcmp(op->mnemonic, "shll16")==0) *value=OPC_SHLL16(op->args[0].data);
			else if(strcmp(op->mnemonic, "shlr")==0) *value=OPC_SHLR(op->args[0].data);
			else if(strcmp(op->mnemonic, "shlr2")==0) *value=OPC_SHLR2(op->args[0].data);
			else if(strcmp(op->mnemonic, "shlr8")==0) *value=OPC_SHLR8(op->args[0].data);
			else if(strcmp(op->mnemonic, "shlr16")==0) *value=OPC_SHLR16(op->args[0].data);
			else ret=-1;
			break;
		case ARG_TYPE_D_REG:
			if(strcmp(op->mnemonic, "add")==0) *value=OPC_ADD(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "addc")==0) *value=OPC_ADDC(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "addv")==0) *value=OPC_ADDV(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "and")==0) *value=OPC_AND(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "cmp/eq")==0) *value=OPC_CMP_EQ(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "cmp/ge")==0) *value=OPC_CMP_GE(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "cmp/gt")==0) *value=OPC_CMP_GT(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "cmp/hi")==0) *value=OPC_CMP_HI(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "cmp/hs")==0) *value=OPC_CMP_HS(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "cmp/str")==0) *value=OPC_CMP_STR(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "div0s")==0) *value=OPC_DIV0S(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "div1")==0) *value=OPC_DIV1(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "dmuls.l")==0) *value=OPC_DMULSL(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "dmulu.s")==0) *value=OPC_DMULUL(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "exts.b")==0) *value=OPC_EXTSB(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "exts.w")==0) *value=OPC_EXTSW(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "extu.b")==0) *value=OPC_EXTUB(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "extu.w")==0) *value=OPC_EXTUW(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov")==0) *value=OPC_MOV_RM_RN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mul.l")==0) *value=OPC_MULL(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "muls.w")==0) *value=OPC_MULSW(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mulu.w")==0) *value=OPC_MULUW(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "neg")==0) *value=OPC_NEG(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "negc")==0) *value=OPC_NEGC(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "not")==0) *value=OPC_NOT(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "or")==0) *value=OPC_OR(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "shad")==0) *value=OPC_SHAD(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "shld")==0) *value=OPC_SHLD(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "sub")==0) *value=OPC_SUB(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "subc")==0) *value=OPC_SUBC(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "subv")==0) *value=OPC_SUBV(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "swap.b")==0) *value=OPC_SWAPB(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "swap.w")==0) *value=OPC_SWAPW(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "tst")==0) *value=OPC_TST(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "xor")==0) *value=OPC_XOR(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "xtrct")==0) *value=OPC_XTRCT(op->args[0].data, op->args[1].data);
			else ret=-1;
			break;
		case ARG_TYPE_I_REG:
			if(strcmp(op->mnemonic, "mov.b")==0) *value=OPC_MOVB_RM_ATRN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_RM_ATRN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_RM_ATRN(op->args[0].data, op->args[1].data);
			else ret=-1;
			break;
		case ARG_TYPE_SPECIAL_REG:
			if(strcmp(op->mnemonic, "ldc")==0) {
				switch(op->args[1].data) {
				case SREG_GBR:
					*value=OPC_LDC_GBR(op->args[0].data); break;
				case SREG_SR:
					*value=OPC_LDC_SR(op->args[0].data); break;
				case SREG_VBR:
					*value=OPC_LDC_VBR(op->args[0].data); break;
				case SREG_SSR:
					*value=OPC_LDC_SSR(op->args[0].data); break;
				case SREG_SPC:
					*value=OPC_LDC_SPC(op->args[0].data); break;
				case SREG_R0_BANK:
					*value=OPC_LDC_R0_BANK(op->args[0].data); break;
				case SREG_R1_BANK:
					*value=OPC_LDC_R1_BANK(op->args[0].data); break;
				case SREG_R2_BANK:
					*value=OPC_LDC_R0_BANK(op->args[0].data); break;
				case SREG_R3_BANK:
					*value=OPC_LDC_R3_BANK(op->args[0].data); break;
				case SREG_R4_BANK:
					*value=OPC_LDC_R4_BANK(op->args[0].data); break;
				case SREG_R5_BANK:
					*value=OPC_LDC_R5_BANK(op->args[0].data); break;
				case SREG_R6_BANK:
					*value=OPC_LDC_R6_BANK(op->args[0].data); break;
				case SREG_R7_BANK:
					*value=OPC_LDC_R7_BANK(op->args[0].data); break;
				default: ret=-1;
				}
			}
			else if(strcmp(op->mnemonic, "lds")==0) {
				switch(op->args[1].data) {
				case SREG_MACH:
					*value=OPC_LDS_MACH(op->args[0].data); break;
				case SREG_MACL:
					*value=OPC_LDS_MACL(op->args[0].data); break;
				case SREG_PR:
					*value=OPC_LDS_PR(op->args[0].data); break;
				default: ret=-1;
				}
			}
			else ret=-1;
			break;
		case ARG_TYPE_DISP_GBR:
			if(op->args[0].data == REG_R0) {
				if(strcmp(op->mnemonic, "mov.b")==0) *value=OPC_MOVB_R0_ATDISPGBR(op->args[1].data);
				else if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_R0_ATDISPGBR(op->args[1].data);
				else if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_R0_ATDISPGBR(op->args[1].data);
				else ret=-1;
			}
			else ret=-1;
			break;
		case ARG_TYPE_DISP_REG:
			//TODO check for negative displacement
			if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_RM_ATDISPRN(op->args[0].data, op->args[1].data >> 4, op->args[1].data & 0x0000000F);
			else if(op->args[0].data == REG_R0) {
				if(strcmp(op->mnemonic, "mov.b")==0) *value=OPC_MOVB_R0_ATDISPRN(op->args[1].data >> 4, op->args[1].data & 0x0000000F);
				else if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_R0_ATDISPRN(op->args[1].data >> 4, op->args[1].data & 0x0000000F);
				else ret=-1;
			}
			else ret=-1;
			break;
		case ARG_TYPE_INDEX_REG:
			if(strcmp(op->mnemonic, "mov.b")==0) *value=OPC_MOVB_RM_ATR0RN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_RM_ATR0RN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_RM_ATR0RN(op->args[0].data, op->args[1].data);
			else ret=-1;
			break;
		case ARG_TYPE_DEC_REG:
			if(strcmp(op->mnemonic, "mov.b")==0) *value=OPC_MOVB_RM_ATDECRN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_RM_ATDECRN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_RM_ATDECRN(op->args[0].data, op->args[1].data);
			else ret=-1;
			break;
		default: ret=-1;
		}
		break;

	// First argument is an immediate data
	case ARG_TYPE_IMMEDIATE:
		switch (op->args[1].type) {
		case ARG_TYPE_NOTHING:
			if(strcmp(op->mnemonic, "trapa")==0) *value=OPC_TRAPA(op->args[0].data);
			else ret=-1;
			break;
		case ARG_TYPE_D_REG:
			if(strcmp(op->mnemonic, "add")==0) *value=OPC_ADD_IMM(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov")==0) *value=OPC_MOV_IMM_RN(op->args[0].data, op->args[1].data);
			else if(op->args[1].data == REG_R0) {
				if(strcmp(op->mnemonic, "and")==0) *value=OPC_AND_IMM(op->args[0].data);
				else if(strcmp(op->mnemonic, "cmp/eq")==0) *value=OPC_CMP_EQ_IMM(op->args[0].data);
				else if(strcmp(op->mnemonic, "or")==0) *value=OPC_OR_IMM(op->args[0].data);
				else if(strcmp(op->mnemonic, "tst")==0) *value=OPC_TST_IMM(op->args[0].data);
				else if(strcmp(op->mnemonic, "xor")==0) *value=OPC_XOR_IMM(op->args[0].data);
				else ret=-1;
			}
			else ret=-1;
			break;
		case ARG_TYPE_INDEX_GBR:
			if(strcmp(op->mnemonic, "and.b")==0) *value=OPC_ANDB(op->args[0].data);
			else if(strcmp(op->mnemonic, "or.b")==0) *value=OPC_ORB(op->args[0].data);
			else if(strcmp(op->mnemonic, "tst.b")==0) *value=OPC_TSTB(op->args[0].data);
			else if(strcmp(op->mnemonic, "xor.b")==0) *value=OPC_XORB(op->args[0].data);
			else ret=-1;
			break;
		default: ret=-1;
		}
		break;

	// If the first argument is an indirect register
	case ARG_TYPE_I_REG:
		switch (op->args[1].type) {
		case ARG_TYPE_NOTHING:
			if(strcmp(op->mnemonic, "jmp")==0) *value=OPC_JMP(op->args[0].data);
			else if(strcmp(op->mnemonic, "jsr")==0) *value=OPC_JSR(op->args[0].data);
			else if(strcmp(op->mnemonic, "pref")==0) *value=OPC_PREF(op->args[0].data);
			else if(strcmp(op->mnemonic, "tas.b")==0) *value=OPC_TASB(op->args[0].data);
			else ret=-1;
			break;
		case ARG_TYPE_D_REG:
			if(strcmp(op->mnemonic, "mov.b")==0) *value=OPC_MOVB_ATRM_RN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_ATRM_RN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_ATRM_RN(op->args[0].data, op->args[1].data);
			else ret=-1;
			break;
		default: ret=-1;
		}
		break;

	// If the first argument is an indirect incremented register
	case ARG_TYPE_INC_REG:
		switch (op->args[1].type) {
		case ARG_TYPE_INC_REG:
			if(strcmp(op->mnemonic, "mac.l")==0) *value=OPC_MACL(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mac.w")==0) *value=OPC_MACW(op->args[0].data, op->args[1].data);
			else ret=-1;
			break;
		case ARG_TYPE_D_REG:
			if(strcmp(op->mnemonic, "mov.b")==0) *value=OPC_MOVB_ATRMINC_RN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_ATRMINC_RN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_ATRMINC_RN(op->args[0].data, op->args[1].data);
			else ret=-1;
			break;
		case ARG_TYPE_SPECIAL_REG:
			if(strcmp(op->mnemonic, "ldc.l")==0) {
				switch(op->args[1].data) {
				case SREG_GBR:
					*value=OPC_LDCL_GBR(op->args[0].data); break;
				case SREG_SR:
					*value=OPC_LDCL_SR(op->args[0].data); break;
				case SREG_VBR:
					*value=OPC_LDCL_VBR(op->args[0].data); break;
				case SREG_SSR:
					*value=OPC_LDCL_SSR(op->args[0].data); break;
				case SREG_SPC:
					*value=OPC_LDCL_SPC(op->args[0].data); break;
				case SREG_R0_BANK:
					*value=OPC_LDCL_R0_BANK(op->args[0].data); break;
				case SREG_R1_BANK:
					*value=OPC_LDCL_R1_BANK(op->args[0].data); break;
				case SREG_R2_BANK:
					*value=OPC_LDCL_R0_BANK(op->args[0].data); break;
				case SREG_R3_BANK:
					*value=OPC_LDCL_R3_BANK(op->args[0].data); break;
				case SREG_R4_BANK:
					*value=OPC_LDCL_R4_BANK(op->args[0].data); break;
				case SREG_R5_BANK:
					*value=OPC_LDCL_R5_BANK(op->args[0].data); break;
				case SREG_R6_BANK:
					*value=OPC_LDCL_R6_BANK(op->args[0].data); break;
				case SREG_R7_BANK:
					*value=OPC_LDCL_R7_BANK(op->args[0].data); break;
				default: ret=-1;
				}
			}
			else if(strcmp(op->mnemonic, "lds.l")==0) {
				switch(op->args[1].data) {
				case SREG_MACH:
					*value=OPC_LDSL_MACH(op->args[0].data); break;
				case SREG_MACL:
					*value=OPC_LDSL_MACL(op->args[0].data); break;
				case SREG_PR:
					*value=OPC_LDSL_PR(op->args[0].data); break;
				default: ret=-1;
				}
			}
			else ret=-1;
			break;
		default: ret=-1;
		}
		break;

	// If the first argument is a GBR displacement
	case ARG_TYPE_DISP_GBR:
		if((op->args[1].type == ARG_TYPE_D_REG) && (op->args[1].data == REG_R0)) {
			if(strcmp(op->mnemonic, "mov.b")==0) *value=OPC_MOVB_ATDISPGBR_R0(op->args[0].data);
			else if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_ATDISPGBR_R0(op->args[0].data);
			else if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_ATDISPGBR_R0(op->args[0].data);
			else ret=-1;
		}
		else ret=-1;
		break;

	// If the first argument is a register displacement
	case ARG_TYPE_DISP_REG:
		//TODO check for negative displacement
		if(op->args[1].type == ARG_TYPE_D_REG) {
			if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_ATDISPRM_RN(op->args[0].data >> 4, op->args[0].data & 0x0000000F, op->args[1].data);
			else if(op->args[1].data == REG_R0) {
				if(strcmp(op->mnemonic, "mov.b")==0) *value=OPC_MOVB_ATDISPRM_R0(op->args[0].data >> 4, op->args[0].data & 0x0000000F);
				else if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_ATDISPRM_R0(op->args[0].data >> 4, op->args[0].data & 0x0000000F);
				else ret=-1;
			}
			else ret=-1;
		}
		else ret=-1;
		break;

	// If the first argument is an indexed register
	case ARG_TYPE_INDEX_REG:
		if(op->args[1].type == ARG_TYPE_D_REG) {
			if(strcmp(op->mnemonic, "mov.b")==0) *value=OPC_MOVB_ATR0RM_RN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_ATR0RM_RN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_ATR0RM_RN(op->args[0].data, op->args[1].data);
			else ret=-1;
		}
		else ret=-1;
		break;

	// If the first argument is a PC displacement (or a label converted into displacement)
	case ARG_TYPE_DISP_PC:
		if(op->args[1].type == ARG_TYPE_NOTHING) {
			if(strcmp(op->mnemonic, "bf")==0) *value=OPC_BF(op->args[0].data);
			else if(strcmp(op->mnemonic, "bf/s")==0) *value=OPC_BF_S(op->args[0].data);
			else if(strcmp(op->mnemonic, "bt")==0) *value=OPC_BT(op->args[0].data);
			else if(strcmp(op->mnemonic, "bt/s")==0) *value=OPC_BT_S(op->args[0].data);
			else if(strcmp(op->mnemonic, "bra")==0) *value=OPC_BRA(op->args[0].data);
			else if(strcmp(op->mnemonic, "bsr")==0) *value=OPC_BSR(op->args[0].data);
			else ret=-1;
		}
		else if(op->args[1].type == ARG_TYPE_D_REG) {
			if(strcmp(op->mnemonic, "mov.w")==0) *value=OPC_MOVW_ATDISP_RN(op->args[0].data, op->args[1].data);
			else if(strcmp(op->mnemonic, "mov.l")==0) *value=OPC_MOVL_ATDISP_RN(op->args[0].data, op->args[1].data);
			else if((op->args[1].data == REG_R0) && (strcmp(op->mnemonic, "mova")==0))
				*value=OPC_MOVA(op->args[0].data);
			else ret=-1;
		}
		else ret=-1;
		break;

	// If the first argument is a special register
	case ARG_TYPE_SPECIAL_REG:
		if(op->args[1].type == ARG_TYPE_D_REG) {
			if(strcmp(op->mnemonic, "stc")==0) {
				switch(op->args[0].data) {
				case SREG_GBR:
					*value=OPC_STC_GBR(op->args[1].data); break;
				case SREG_SR:
					*value=OPC_STC_SR(op->args[1].data); break;
				case SREG_VBR:
					*value=OPC_STC_VBR(op->args[1].data); break;
				case SREG_SSR:
					*value=OPC_STC_SSR(op->args[1].data); break;
				case SREG_SPC:
					*value=OPC_STC_SPC(op->args[1].data); break;
				case SREG_R0_BANK:
					*value=OPC_STC_R0_BANK(op->args[1].data); break;
				case SREG_R1_BANK:
					*value=OPC_STC_R1_BANK(op->args[1].data); break;
				case SREG_R2_BANK:
					*value=OPC_STC_R0_BANK(op->args[1].data); break;
				case SREG_R3_BANK:
					*value=OPC_STC_R3_BANK(op->args[1].data); break;
				case SREG_R4_BANK:
					*value=OPC_STC_R4_BANK(op->args[1].data); break;
				case SREG_R5_BANK:
					*value=OPC_STC_R5_BANK(op->args[1].data); break;
				case SREG_R6_BANK:
					*value=OPC_STC_R6_BANK(op->args[1].data); break;
				case SREG_R7_BANK:
					*value=OPC_STC_R7_BANK(op->args[1].data); break;
				default: ret=-1;
				}
			}
			else if(strcmp(op->mnemonic, "sts")==0) {
				switch(op->args[0].data) {
				case SREG_MACH:
					*value=OPC_STS_MACH(op->args[1].data); break;
				case SREG_MACL:
					*value=OPC_STS_MACL(op->args[1].data); break;
				case SREG_PR:
					*value=OPC_STS_PR(op->args[1].data); break;
				default: ret=-1;
				}
			}
			else ret=-1;
		}
		else if(op->args[1].type == ARG_TYPE_DEC_REG) {
			if(strcmp(op->mnemonic, "stc.l")==0) {
				switch(op->args[0].data) {
				case SREG_GBR:
					*value=OPC_STCL_GBR(op->args[1].data); break;
				case SREG_SR:
					*value=OPC_STCL_SR(op->args[1].data); break;
				case SREG_VBR:
					*value=OPC_STCL_VBR(op->args[1].data); break;
				case SREG_SSR:
					*value=OPC_STCL_SSR(op->args[1].data); break;
				case SREG_SPC:
					*value=OPC_STCL_SPC(op->args[1].data); break;
				case SREG_R0_BANK:
					*value=OPC_STCL_R0_BANK(op->args[1].data); break;
				case SREG_R1_BANK:
					*value=OPC_STCL_R1_BANK(op->args[1].data); break;
				case SREG_R2_BANK:
					*value=OPC_STCL_R0_BANK(op->args[1].data); break;
				case SREG_R3_BANK:
					*value=OPC_STCL_R3_BANK(op->args[1].data); break;
				case SREG_R4_BANK:
					*value=OPC_STCL_R4_BANK(op->args[1].data); break;
				case SREG_R5_BANK:
					*value=OPC_STCL_R5_BANK(op->args[1].data); break;
				case SREG_R6_BANK:
					*value=OPC_STCL_R6_BANK(op->args[1].data); break;
				case SREG_R7_BANK:
					*value=OPC_STCL_R7_BANK(op->args[1].data); break;
				default: ret=-1;
				}
			}
			else if(strcmp(op->mnemonic, "sts.l")==0) {
				switch(op->args[0].data) {
				case SREG_MACH:
					*value=OPC_STSL_MACH(op->args[1].data); break;
				case SREG_MACL:
					*value=OPC_STSL_MACL(op->args[1].data); break;
				case SREG_PR:
					*value=OPC_STSL_PR(op->args[1].data); break;
				default: ret=-1;
				}
			}
			else ret=-1;
		}
		else ret=-1;
		break;

	}
	return ret;
}




int stringToNumber(const char *str, int *value) {
	int ret;
	int num=0;
	int i;

	// Hexa string
	if(str[0]=='0' && str[1]=='x') {
		ret=2;
		int overflow=0;
		for(i=0; (isdigit(str[2+i]) || (str[2+i]>='A' && str[2+i]<='F') || (str[2+i]>='a' && str[2+i]<='f')) && !overflow; i++) {
			if(num & 0xF0000000) overflow=1;
			num <<= 4;
			num += isdigit(str[2+i]) ? str[2+i]-'0' : toupper(str[2+i])-'A'+10;
		}
		if(i==0 || str[2+i]!=0) ret = -1;
		else if(overflow) ret = -2;
	}

	// Binary string
	else if(str[0]=='0' && str[1]=='b') {
		ret=2;
		int overflow=0;
		for(i=0; (str[2+i]=='0' || str[2+i]=='1') && !overflow; i++) {
			if(num & 0x80000000) overflow=1;
			num <<= 1;
			num += str[2+i]-'0';
		}
		if(i==0 || str[2+i]!=0) ret = -1;
		else if(overflow) ret = -2;
	}

	// Decimal string
	else if(isdigit(str[0]) || ((str[0]=='+' || str[0]=='-') && isdigit(str[1])) ) {
		ret=1;
		int overflow = 0;
		int start = (str[0]=='+' || str[0]=='-') ? 1 : 0;
		for(i=start; isdigit(str[i]) && !overflow; i++) {
			//TODO faster overflow detection...
			if((unsigned int)num > (0x7FFFFFFF/10)) overflow = 1;
			num *= 10;
			if((unsigned int)(num+(str[i]-'0')) > (0x7FFFFFFF)) overflow = 1;
			num += str[i]-'0';
		}
		if(i-start==0 || str[i]!=0) ret = -1;
		else if(overflow) ret = -2;
		else if(str[0]=='-') num = -num;
	}

	else ret = -1;

	if(ret >= 0) *value=num;

	return ret;
}


