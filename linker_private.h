#ifndef MCA_LINKER_PRIVATE_H
#define MCA_LINKER_PRIVATE_H


#define ERROR_LOOP(errornum) error = (errornum); \
								continue

// For memories mapping
struct _MemoryMap {
	char *name;
	unsigned int begin_address;
	unsigned int size;
};


// For sections mapping
struct _SectionMap {
	char *name;
	int memory_id;   // N° in the _MemoryMap array to use
	//int priority;    // The higer it's, the less this section's priority is
	int flags;
};

// _SectionMap flags :
// NB: <section_name> is the name of the section with every non-alphanum char replaced by '_'
#define GEN_BEGIN_SYMBOL	0x01	// Generate <section_name>_begin symbol
#define GEN_END_SYMBOL		0x02    // Generate <section_name>_end symbol
#define GEN_SIZE_SYMBOL		0x04    // Generate <section_name>_size symbol
#define DONT_GEN_SECTION	0x08    // Process section's symbols but dont put it in the final file
#define REMOVE_SECTION		0x10	// Don't process the section at all (act as if the section doesn't exist)
#define	DATA_TYPE_SECTION	0x20	// Dirty flag for .data section...
									//   generate a <section_name>_rom_begin symbol


// Representation of a MOF file content
struct _FileContent {
	int format_version;
	int sections_number;
	char **sections_name;
	unsigned int *sections_size;
	int *sections_abs_number;
//	unsigned int **abs_address;
//	unsigned char **abs_address_section;
	int imported_number;
//	char **imported_name;
//	int *imported_occurences_number;
//	unsigned char **imported_occurences_section;
//	unsigned int **imported_occurences_address;
	int exported_number;
	char **exported_name;
	unsigned char *exported_section;
	int *exported_value;
	// The followings fields aren't filled when the file is reading, but are here for convenience :
	unsigned int *sections_org; // .org of each section
	unsigned int *sections_file_pos; // position in the output file of each section
	unsigned char *sections_global_id; // corresponding section ID in the _SectionMap array
};


// read an integer (32 bits) from buffer at position (pos to pos+3) in big-endian convention
// <char*> buffer
// <int> pos
// <int> integer
#define READ_INTEGER(buffer, pos, integer) integer = ((buffer)[(pos)]<<24) + (((unsigned char*)(buffer))[(pos)+1]<<16) + (((unsigned char*)(buffer))[(pos)+2]<<8) + ((unsigned char*)(buffer))[(pos)+3]

// write integer in the buffer at position (pos to pos+3) in big-endian convention
// <int> integer
// <char*> buffer
// <int> pos
#define WRITE_INTEGER(integer, buffer, pos) (buffer)[(pos)] = (char)((integer)>>24); \
				(buffer)[(pos)+1] = (char)((integer)>>16); \
				(buffer)[(pos)+2] = (char)((integer)>>8); \
				(buffer)[(pos)+3] = (char)(integer)

#endif //MCA_LINKER_PRIVATE_H
