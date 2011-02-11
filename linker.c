#include "linker.h"
#include "linker_private.h"
// for writing G1A files :
#include "g1a_utils.h"

#ifndef TESTING_PC
#include <fxstdio.h>
#else
#include <stdio.h>
#endif

#include <stdlib.h>
#include <string.h>


// TODO find a good way to process error in parsing

// TODO don't forget to start all section at a .align 4 address!!!

// TODO command line argument system (to set output type and options)

int linkobjfiles(int filesnum, FONTCHARACTER **files, FONTCHARACTER *out, int outformat) {

	// Static mapping for now
	struct _MemoryMap memories_map[] = {
		{"rom", 0x00300200, 0x80000}, // 512kiB
		{"ram", 0x08100000, 0x8000}   //  32kiB
	};
	int memories_num = 2;
	struct _SectionMap sections_map[] = {
		{".pretext",	0, 0x00},
		{".text",		0, 0x00},
		{".rodata",		0, 0x00},
		{".bss",		1, GEN_BEGIN_SYMBOL | GEN_END_SYMBOL | DONT_GEN_SECTION},
		{".data",		1, GEN_BEGIN_SYMBOL | GEN_END_SYMBOL | DATA_TYPE_SECTION},
		{".comment",	-1, REMOVE_SECTION}
	};
	int sections_num = 6;

	int i, j;
	int error = 0;
	char str[40];
	char buffer[256];
	int tmp;

	// intrinsic symbols
	int intrinsic_number = 0;
	char **intrinsic_name = NULL;
	unsigned int *intrinsic_value = NULL;

	// for the dirty data-type sections system
	unsigned int rom_data_begin = 0;

	unsigned int bin_size = 0;
	unsigned char *bin_buffer = NULL;

	FILE *file = NULL;

	struct _FileContent *files_content = NULL;
	
	// Memory pool system
	int mempool_pos_1 = 0;
	int mempool_size_1 = 0;
	void *mempool_1 = NULL;  // memory pool for '.align 1' data (as char)
	int mempool_pos_4 = 0;
	int mempool_size_4 = 0;
	void *mempool_4 = NULL;  // memory pool for '.align 4' data (as int or pointer)


	// Compute the mempool size
	for(i=0; i<filesnum && !error; i++) {
		for(j=0; (j<40) && ((str[j]=(char)files[i][j]) != 0); j++);
		file = fopen(str, "r");
		if(file == NULL) { ERROR_LOOP(-1); }

		mempool_size_4 += sizeof(struct _FileContent);

		if(fread(buffer, sizeof(char), 7+4+4+4, file) != 7+4+4+4) { ERROR_LOOP(-2); }
		if(buffer[0]=='M' && buffer[1]=='O' && buffer[2]=='F' && buffer[3]=='-' && buffer[5]=='.') {
			// Check if MOF version is supported
			if( !(buffer[4]=='0' && buffer[6]=='1') ) { ERROR_LOOP(-4); }
		}
		else  { ERROR_LOOP(-3); }

		if(fread(buffer, sizeof(char), 4, file) != 4) { ERROR_LOOP(-2); }
		int number;
		READ_INTEGER(buffer, 0, number);
		// For each section :
		for(j=0; j<number && !error; j++) {
			mempool_size_4 += sizeof(char*) + sizeof(unsigned int) + sizeof(int)
						+ sizeof(unsigned int) + sizeof(unsigned int);  // for sections_org and sections_file_pos
			mempool_size_1 += sizeof(unsigned char); // for sections_global_id
			if((tmp = fgetc(file)) == EOF) { ERROR_LOOP(-2); }
			mempool_size_1 += (unsigned char)tmp + 1; // name length + '\0'
			
			fseek(file, (unsigned char)tmp, SEEK_CUR);
			// section-relative address number
			if(fread(buffer, sizeof(char), 4, file) != 4) { ERROR_LOOP(-2); }
			READ_INTEGER(buffer, 0, tmp);
			fseek(file, tmp*(4+1), SEEK_CUR);

			// data size
			if(fread(buffer, sizeof(char), 4, file) != 4) { ERROR_LOOP(-2); }
			READ_INTEGER(buffer, 0, tmp);
			fseek(file, tmp, SEEK_CUR);
		}
		if(error) continue;

		if(fread(buffer, sizeof(char), 4, file) != 4) { ERROR_LOOP(-2); }
		READ_INTEGER(buffer, 0, number);
		// For each imported symbol
		for(j=0; j<number && !error; j++) {
			if((tmp = fgetc(file)) == EOF) { ERROR_LOOP(-2); }			
			fseek(file, (unsigned char)tmp, SEEK_CUR);

			// occurences number
			if(fread(buffer, sizeof(char), 4, file) != 4) { ERROR_LOOP(-2); }
			READ_INTEGER(buffer, 0, tmp);
			fseek(file, tmp*(4+1), SEEK_CUR);
		}
		if(error) continue;

		if(fread(buffer, sizeof(char), 4, file) != 4) { ERROR_LOOP(-2); }
		READ_INTEGER(buffer, 0, number);
		// For each exported symbol
		for(j=0; j<number && !error; j++) {
			mempool_size_4 += sizeof(char*) + sizeof(unsigned int);
			mempool_size_1 += sizeof(unsigned char);
			if((tmp = fgetc(file)) == EOF) { ERROR_LOOP(-2); }
			mempool_size_1 += (unsigned char)tmp + 1; // name length + '\0'
			fseek(file, (unsigned char)tmp + 1+4, SEEK_CUR);
		}
		if(error) continue;
		fclose(file);
		file = NULL;
	}
	if(error) {
		if(file != NULL) fclose(file);
		return error;
	}

	// Intrinsic symbols size
	for(i=0; i<sections_num && !error; i++) {
		int flags = sections_map[i].flags;
		if(flags & GEN_BEGIN_SYMBOL) {
			intrinsic_number++;
			mempool_size_4 += sizeof(unsigned int) + sizeof(char*);
			mempool_size_1 += strlen(sections_map[i].name) + 6+1;
		}
		if(flags & GEN_END_SYMBOL) {
			intrinsic_number++;
			mempool_size_4 += sizeof(unsigned int) + sizeof(char*);
			mempool_size_1 += strlen(sections_map[i].name) + 4+1;
		}
		if(flags & GEN_SIZE_SYMBOL) {
			intrinsic_number++;
			mempool_size_4 += sizeof(unsigned int) + sizeof(char*);
			mempool_size_1 += strlen(sections_map[i].name) + 5+1;
		}
		if(flags & DATA_TYPE_SECTION) {
			intrinsic_number++;
			mempool_size_4 += sizeof(unsigned int) + sizeof(char*);
			mempool_size_1 += strlen(sections_map[i].name) +10+1;
		}
	}


	// Allocate memory pool
	mempool_1 = calloc(mempool_size_1, sizeof(char));
	mempool_4 = calloc(mempool_size_4, sizeof(char));
	if((mempool_size_1!=0 && mempool_1==NULL) || (mempool_size_4!=0 && mempool_4==NULL)) {
		if(file != NULL) fclose(file);
		if(mempool_1 != NULL) free(mempool_1);
		if(mempool_4 != NULL) free(mempool_4);
		return -5;
	}
	files_content = mempool_4;
	mempool_pos_4 += filesnum * sizeof(struct _FileContent);
	intrinsic_name = (char**)((int)mempool_4 + mempool_pos_4);
	mempool_pos_4 += intrinsic_number * sizeof(char*);
	intrinsic_value = (unsigned int*)((int)mempool_4 + mempool_pos_4);
	mempool_pos_4 += intrinsic_number * sizeof(unsigned int);



	// Load the most importants parts of the object files
	// Assume the files are safe (to avoid some redundancies tests), but maybe change this later?
	for(i=0; i<filesnum && !error; i++) {
		for(j=0; (j<40) && ((str[j]=(char)files[i][j]) != 0); j++);
		file = fopen(str, "r");
		if(file == NULL) { ERROR_LOOP(-1); }

		fread(buffer, sizeof(char), 7+4+4+4, file);
		files_content[i].format_version = (buffer[4]-'0')*10 + buffer[6]-'0';

		fread(buffer, sizeof(char), 4, file);
		int number;
		READ_INTEGER(buffer, 0, number);
		files_content[i].sections_number = number;
		files_content[i].sections_name = (char**)((int)mempool_4+mempool_pos_4);
		mempool_pos_4 += sizeof(char*) * number;
		files_content[i].sections_size = (unsigned int*)((int)mempool_4+mempool_pos_4);
		mempool_pos_4 += sizeof(unsigned int) * number;
		files_content[i].sections_abs_number = (int*)((int)mempool_4+mempool_pos_4);
		mempool_pos_4 += sizeof(int) * number;

		// the three followings will not filled immediately
		files_content[i].sections_org = (unsigned int*)((int)mempool_4+mempool_pos_4);
		mempool_pos_4 += sizeof(unsigned int) * number;
		files_content[i].sections_file_pos = (unsigned int*)((int)mempool_4+mempool_pos_4);
		mempool_pos_4 += sizeof(unsigned int) * number;
		files_content[i].sections_global_id = (unsigned char*)((int)mempool_1+mempool_pos_1);
		mempool_pos_1 += sizeof(unsigned char) * number;

		// For each section :
		for(j=0; j<number && !error; j++) {
			// copy the section name
			tmp = fgetc(file);
			fread(buffer, sizeof(char), (unsigned char)tmp, file);
			memcpy((char*)((int)mempool_1+mempool_pos_1), buffer, (unsigned char)tmp);
			files_content[i].sections_name[j] = (char*)((int)mempool_1+mempool_pos_1);
			files_content[i].sections_name[j][(unsigned char)tmp] = 0;
			mempool_pos_1 += (unsigned char)tmp+1;

			// copy section-relative address
			fread(buffer, sizeof(char), 4, file);
			int absnum;
			READ_INTEGER(buffer, 0, absnum);
			files_content[i].sections_abs_number[j] = absnum;
			//files_content[i].abs_address[j] = (unsigned int*)((int)mempool_4+mempool_pos_4);
			//mempool_pos_4 += absnum*sizeof(unsigned int);
			//files_content[i].abs_address_section[j] = (unsigned char*)((int)mempool_1+mempool_pos_1);
			//mempool_pos_1 += absnum*sizeof(unsigned char);
			//int k;
			//for(k=0; k<absnum; k++) {
				//fread(buffer, sizeof(char), 5, file);
				//files_content[i].abs_address_section[j][k]=buffer[0];
				//READ_INTEGER(buffer, 1, tmp);
				//files_content[i].abs_address_section[j][k]=tmp;
			//}
			fseek(file, absnum*(4+1), SEEK_CUR);
			
			// copy data size and skip the content
			fread(buffer, sizeof(char), 4, file);
			READ_INTEGER(buffer, 0, tmp);
			files_content[i].sections_size[j] = tmp;
			fseek(file, tmp, SEEK_CUR);
		}
		if(error) continue;

		fread(buffer, sizeof(char), 4, file);
		READ_INTEGER(buffer, 0, number);
		files_content[i].imported_number = number;

		// For each imported symbol
		for(j=0; j<number && !error; j++) {
			// copy the symbol name
			tmp = fgetc(file);
			fseek(file, (unsigned char)tmp, SEEK_CUR);
			//fread(buffer, sizeof(char), (unsigned char)tmp, file);
			//memcpy((char*)((int)mempool_1+mempool_pos_1), buffer, (unsigned char)tmp);
			//files_content[i].imported_name[j] = (char*)((int)mempool_1+mempool_pos_1);
			//files_content[i].imported_name[j][(unsigned char)tmp] = 0;
			//mempool_pos_1 += (unsigned char)tmp+1;

			// copy each occurence data
			fread(buffer, sizeof(char), 4, file);
			int occurences;
			READ_INTEGER(buffer, 0, occurences);
			//files_content[i].imported_occurences_number[j] = occurences;
			//files_content[i].imported_occurences_address[j] = (unsigned int*)((int)mempool_4+mempool_pos_4);
			//mempool_pos_4 += occurences*sizeof(unsigned int);
			//files_content[i].imported_occurences_section[j] = (unsigned char*)((int)mempool_1+mempool_pos_1);
			//mempool_pos_1 += occurences*sizeof(unsigned char);
			//int k;
			//for(k=0; k<occurences; k++) {
				//fread(buffer, sizeof(char), 5, file);
				//files_content[i].imported_occurences_section[j][k]=buffer[0];
				//READ_INTEGER(buffer, 1, tmp);
				//files_content[i].imported_occurences_address[j][k]=tmp;
			//}
			fseek(file, occurences*(4+1), SEEK_CUR);
		}
		if(error) continue;

		fread(buffer, sizeof(char), 4, file);
		READ_INTEGER(buffer, 0, number);
		files_content[i].exported_number = number;
		files_content[i].exported_name = (char**)((int)mempool_4+mempool_pos_4);
		mempool_pos_4 += sizeof(char*) * number;
		files_content[i].exported_value = (int*)((int)mempool_4+mempool_pos_4);
		mempool_pos_4 += sizeof(int) * number;
		files_content[i].exported_section = (unsigned char*)((int)mempool_1+mempool_pos_1);
		mempool_pos_1 += sizeof(unsigned char) * number;

		// For each exported symbol
		for(j=0; j<number && !error; j++) {
			// copy the symbol name
			tmp = fgetc(file);
			fread(buffer, sizeof(char), (unsigned char)tmp, file);
			memcpy((char*)((int)mempool_1+mempool_pos_1), buffer, (unsigned char)tmp);
			files_content[i].exported_name[j] = (char*)((int)mempool_1+mempool_pos_1);
			files_content[i].exported_name[j][(unsigned char)tmp] = 0;
			mempool_pos_1 += (unsigned char)tmp+1;

			fread(buffer, sizeof(char), 5, file);
			files_content[i].exported_section[j]=buffer[0];
			READ_INTEGER(buffer, 1, tmp);
			files_content[i].exported_value[j]=tmp;
		}
		if(error) continue;
		fclose(file);
		file = NULL;
	}
	if(error) {
		if(file != NULL) fclose(file);
		if(mempool_1 != NULL) free(mempool_1);
		if(mempool_4 != NULL) free(mempool_4);
		return error;
	}


	// Initialize intrinsic symbols
	// The names order should follow a predictable order, so it's the same method than
	// that is used to define the origin of each sections...
	for(i=0; i<memories_num && !error; i++) {
		int pos=0;
		int k;
		for(k=0; k<sections_num; k++) {
			if(!(sections_map[k].flags & REMOVE_SECTION) && sections_map[k].memory_id == i) {
				int flags = sections_map[k].flags;
				if(flags & GEN_BEGIN_SYMBOL) {
					for(j=0; sections_map[k].name[j] != 0; j++) {
						((char*)((int)mempool_1+mempool_pos_1))[j]
								= (isalnum(sections_map[k].name[j]) ? sections_map[k].name[j] : '_');
					}
					memcpy((char*)((int)mempool_1+mempool_pos_1+j), "_begin", 6);
					((char*)((int)mempool_1+mempool_pos_1+j))[6] = 0;
					intrinsic_name[pos] = (char*)((int)mempool_1+mempool_pos_1);
					pos++;
					mempool_pos_1 += j+7;
				}
				if(flags & GEN_END_SYMBOL) {
					for(j=0; sections_map[k].name[j] != 0; j++) {
						((char*)((int)mempool_1+mempool_pos_1))[j]
								= (isalnum(sections_map[k].name[j]) ? sections_map[k].name[j] : '_');
					}
					memcpy((char*)((int)mempool_1+mempool_pos_1+j), "_end", 4);
					((char*)((int)mempool_1+mempool_pos_1+j))[4] = 0;
					intrinsic_name[pos] = (char*)((int)mempool_1+mempool_pos_1);
					pos++;
					mempool_pos_1 += j+5;
				}
				if(flags & GEN_SIZE_SYMBOL) {
					for(j=0; sections_map[k].name[j] != 0; j++) {
						((char*)((int)mempool_1+mempool_pos_1))[j]
								= (isalnum(sections_map[k].name[j]) ? sections_map[k].name[j] : '_');
					}
					memcpy((char*)((int)mempool_1+mempool_pos_1+j), "_size", 5);
					((char*)((int)mempool_1+mempool_pos_1+j))[5] = 0;
					intrinsic_name[pos] = (char*)((int)mempool_1+mempool_pos_1);
					pos++;
					mempool_pos_1 += j+6;
				}
			}
		}
	}
	{
		int inverted_pos = intrinsic_number-1;
		for(i=0; i<sections_num && !error; i++) {
			if(sections_map[i].flags & DATA_TYPE_SECTION) {
				// Place them at the end using inverted displacement
				for(j=0; sections_map[i].name[j] != 0; j++) {
					((char*)((int)mempool_1+mempool_pos_1))[j]
							= (isalnum(sections_map[i].name[j]) ? sections_map[i].name[j] : '_');
				}
				memcpy((char*)((int)mempool_1+mempool_pos_1+j), "_rom_begin", 10);
				((char*)((int)mempool_1+mempool_pos_1+j))[10] = 0;
				intrinsic_name[inverted_pos] = (char*)((int)mempool_1+mempool_pos_1);
				inverted_pos--;
				mempool_pos_1 += j+11;
			}
		}
	}


	// Check if there are 2 symbols with the same name
	for(i=0; i<filesnum && !error; i++) {
		for(j=0; j<files_content[i].exported_number && !error; j++) {
			if(i>=filesnum && j>=files_content[i].exported_number) continue;
			int k;
			for(k=(j+1<files_content[i].exported_number ? i : i+1); k<filesnum && !error; k++) {
				int l;
				for(l=((k==i) ? j+1 : 0); l<files_content[k].exported_number && !error; l++) {
					if(strcmp(files_content[i].exported_name[j], files_content[k].exported_name[l]) == 0) {
						ERROR_LOOP(-100);
					}
				}
			}
		}
	}


	// Identify the files' sections using the _SectionMap array and init sections_file_pos to 0xFFFFFFFF
	for(i=0; i<filesnum && !error; i++) {
		for(j=0; j<files_content[i].sections_number && !error; j++) {
			int k;
			files_content[i].sections_file_pos[j] = 0xFFFFFFFF;
			for(k=0; k<sections_num; k++) {
				if(strcmp(files_content[i].sections_name[j], sections_map[k].name) == 0) {
					files_content[i].sections_global_id[j] = (unsigned char)k;
					break;
				}
			}
			if(k >= sections_num) { ERROR_LOOP(-101); }
		}
	}

	// Define the origin (.org) of each section and for each file through the defined mapping
	// of sections and memories. Not optimized but I think it's not a critical section.
	bin_size = 0;
	tmp = 0; // intrinsic symbol position
	for(i=0; i<memories_num && !error; i++) {
		unsigned int memory_pos = memories_map[i].begin_address;
		for(j=0; j<sections_num && !error; j++) {
			if(!(sections_map[j].flags & REMOVE_SECTION) && sections_map[j].memory_id == i) {
				unsigned int section_begin = memory_pos;
				if(sections_map[j].flags & GEN_BEGIN_SYMBOL) {
					intrinsic_value[tmp] = memory_pos;
					tmp++;
				}
				int k;
				for(k=0; k<filesnum && !error; k++) {
					int l;
					for(l=0; l<files_content[k].sections_number; l++) {
						if(files_content[k].sections_global_id[l] == j) {
							// Be sure the section start in a .align 4 address :
							if(memory_pos % 4 != 0) memory_pos += 4-(memory_pos%4);
							files_content[k].sections_org[l] = memory_pos;
							memory_pos += files_content[k].sections_size[l];
							if(!(sections_map[j].flags & DONT_GEN_SECTION)) {
								// Compute the final binary size and set the sections position in the file
								// TODO compute the sections_file_pos without forget DATA_TYPE_SECTION system
								files_content[k].sections_file_pos[l] = bin_size;
								bin_size += files_content[k].sections_size[l];
							}
						}
					}
				}
				if(sections_map[j].flags & GEN_END_SYMBOL) {
					intrinsic_value[tmp] = memory_pos;
					tmp++;
				}
				if(sections_map[j].flags & GEN_SIZE_SYMBOL) {
					intrinsic_value[tmp] = memory_pos-section_begin;
					tmp++;
				}
			}
			// temporary :
			else if(sections_map[j].memory_id>=sections_num) {
				ERROR_LOOP(-102);
			}
		}
		if(memory_pos > memories_map[i].begin_address + memories_map[i].size) {
			ERROR_LOOP(-103);
		}
		// TODO find a better way to process data-type sections
		if(i == 0 /* first memory assumed to be the ROM */) rom_data_begin = memory_pos;
	}
	tmp = intrinsic_number-1;
	for(i=0; i<sections_num && !error; i++) {
		if(sections_map[i].flags & DATA_TYPE_SECTION) {
			// TODO improve this!!!
			intrinsic_value[tmp] = rom_data_begin;
			tmp--;
		}
	}

	if(!error) {
		bin_buffer = calloc(bin_size, sizeof(unsigned char));
		if(bin_size!=0 && bin_buffer==NULL) {
			error = -5;
		}
	}

	// Copy data and replace the symbols (externals (intrinsic or not) and anonymous)
	tmp = 0;
	for(i=0; i<filesnum && !error; i++) {
		for(j=0; (j<40) && ((str[j]=(char)files[i][j]) != 0); j++);
		file = fopen(str, "r");
		if(file == NULL) { ERROR_LOOP(-1); }

		fseek(file, 7+4+4+4 + 4, SEEK_CUR); // Header + 4

		for(j=0; j<files_content[i].sections_number && !error; j++) {
			unsigned int abs_pos;
			tmp = fgetc(file);
			fseek(file, (unsigned char)tmp + 4, SEEK_CUR);
			abs_pos = ftell(file);
			fseek(file, files_content[i].sections_abs_number[j]*(4+1) + 4, SEEK_CUR);
			if(files_content[i].sections_file_pos[j] == 0xFFFFFFFF) {
				fseek(file, files_content[i].sections_size[j], SEEK_CUR);
			}
			else {
				fread((char*)((int)bin_buffer+files_content[i].sections_file_pos[j]), sizeof(char), 
					files_content[i].sections_size[j], file);
			}

			// Replace the section-relative address (anonymous local labels)
			fseek(file, abs_pos, SEEK_SET);
			int k;
			tmp = files_content[i].sections_file_pos[j];
			int abs_num = files_content[i].sections_abs_number[j];
			for(k=0; k<abs_num && !error; k++) {
				unsigned int address, value;
				fread(buffer, sizeof(char), 5, file);
				READ_INTEGER(buffer, 1, address);
				READ_INTEGER(bin_buffer, tmp+address, value);
				if((unsigned char)(buffer[0]) >= files_content[i].sections_number) {
					ERROR_LOOP(-6);
				}
				value += files_content[i].sections_org[(unsigned char)(buffer[0])];
				WRITE_INTEGER(value, bin_buffer, tmp+address);
			}
			fseek(file, 4 + files_content[i].sections_size[j], SEEK_CUR);
		}

		fseek(file, 4 , SEEK_CUR);
		// replace imported symbols (.global and intrinsic)
		for(j=0; j<files_content[i].imported_number && !error; j++) {
			unsigned int value, address;
			int k, found, occurences;
			tmp = fgetc(file);
			fread(buffer, sizeof(char), (unsigned char)tmp, file);
			buffer[(unsigned char)tmp] = 0;
			// find the symbol's value in globals symbols
			found=0;
			for(k=0; k<filesnum && !found; k++) {
				int l;
				for(l=0; l<files_content[k].exported_number && !found; l++) {
					if(strcmp(buffer, files_content[k].exported_name[l]) == 0) {
						found = 1;
						value = files_content[k].exported_value[l];
						if(files_content[k].exported_section[l] != 0xFF) {
							value += files_content[k].sections_org[files_content[k].exported_section[l]];
						}
					}
				}
			}
			// if it's not a global symbol, maybe it's an intrinsic symbol
			if(!found) {
				for(k=0; k<intrinsic_number && !found; k++) {
					if(strcmp(buffer, intrinsic_name[k]) == 0) {
						found = 1;
						value = intrinsic_value[k];
					}
				}
			}
			// if not found stop linking immediately
			if(!found) {
				ERROR_LOOP(-104);
			}

			fread(buffer, sizeof(char), 4, file);
			READ_INTEGER(buffer, 0, occurences);
			for(k=0; k<occurences && !error; k++) {
				fread(buffer, sizeof(char), 5, file);
				READ_INTEGER(buffer, 1, address);
				if((unsigned char)(buffer[0]) >= files_content[i].sections_number) {
					ERROR_LOOP(-6);
				}
				address += files_content[i].sections_file_pos[(unsigned char)(buffer[0])];
				WRITE_INTEGER(value, bin_buffer, address);
			}			
		}
		fclose(file);
		file = NULL;
	}
	if(error) {
		if(mempool_1 != NULL) free(mempool_1);
		if(mempool_4 != NULL) free(mempool_4);
		if(bin_buffer != NULL) free(bin_buffer);
		return error;
	}

	// Buffer filled, write the file :
	if(outformat == FORMAT_G1A) {
		// use default arguments :
		for(i=0; i<40 && ((str[i]=(char)out[i]) != 0) && str[i]!='.'; i++);
		if(i<40 && str[i]=='.') str[i] = 0;
		tmp = writeG1AFromBuffer(bin_buffer, bin_size, out, defaultIconG1A(), str);
		if(tmp<0) {
			if(tmp==-1) error = -7;
			else error = -8;
		}
	}
	else {
		#ifdef TESTING_PC
		file = fopen("test.bin", "w+");
		if(file != NULL) {
			fwrite(bin_buffer, sizeof(char), bin_size, file);
			fclose(file);
			file = NULL;
		}
		#endif
	}

	// Free the memory and close the files
	if(file != NULL) fclose(file);
	if(mempool_1 != NULL) free(mempool_1);
	if(mempool_4 != NULL) free(mempool_4);
	if(bin_buffer != NULL) free(bin_buffer);
	return error;
}

