/**
  * Contain the declarations of the assembler private functions (e.g. parser-related)
  */
#ifndef MCA_ASSEMBLER_PRIVATE_H
#define MCA_ASSEMBLER_PRIVATE_H

// Minimalist Object File format version used : 0.1
#define MOF_MAJOR_VERSION	0
#define MOF_MINOR_VERSION	1


#define ARG_TYPE_UNKNOW		-1
#define ARG_TYPE_NOTHING	 0
#define ARG_TYPE_D_REG		 1  // Rn
#define ARG_TYPE_I_REG	 	 2  // @Rn
#define ARG_TYPE_INC_REG	 3  // @Rn+
#define ARG_TYPE_DEC_REG	 4  // @-Rn
#define ARG_TYPE_IMMEDIATE	 5  // #imm
#define ARG_TYPE_LABEL		 6  // label, in this case argument_data is a char* NON null-terminated
#define ARG_TYPE_DISP_PC	 7  // @(disp,PC)
#define ARG_TYPE_DISP_REG	 8  // @(disp,Rn)
#define ARG_TYPE_INDEX_REG	 9  // @(R0,Rn)
#define ARG_TYPE_DISP_GBR	 10  // @(disp,GBR)
#define ARG_TYPE_INDEX_GBR	 11  // @(R0,GBR)
#define ARG_TYPE_SPECIAL_REG 12  // All the control/system register, as GBR, SR, etc...


// Definition of the specials registers :
#define SREG_GBR		0
#define SREG_SR			1
#define SREG_VBR		2
#define SREG_SSR		3
#define SREG_SPC		4
#define SREG_R0_BANK	5
#define SREG_R1_BANK	6
#define SREG_R2_BANK	7
#define SREG_R3_BANK	8
#define SREG_R4_BANK	9
#define SREG_R5_BANK	10
#define SREG_R6_BANK	11
#define SREG_R7_BANK	12
#define SREG_MACH		13
#define SREG_MACL		14
#define SREG_PR			15


struct OpcodeArgument {
	int type;
	int data; // pointer or int
};

struct ParsedOpcode {
	char mnemonic[10];
	struct OpcodeArgument args[2];
};


/*struct OpcodeFormat {
	const char *mnemonic;
	int base_value;
	unsigned char type1;
	unsigned char type2;
};*/


// Increment the string pointer until the first non-whitespace character, *exculding* \n and \0
// <pointer><lvalue> str
#define SKIP_BLANK(str) while((*str==' ' || *str=='\t') && (*str!='\n') && (*str!=0)) str++


// true is 'c' is a whitespace (' ' or '\t') or the given 'sep' char
// <char> c
// <char> sep
#define IS_SEP(c, sep) ((c)==(sep) || (c)==' ' || (c)=='\t' || (c)=='\n' || (c)==0)


// write integer in the buffer at position (pos to pos+3) in big-endian convention
// <int> integer
// <char*> buffer
// <int> pos
#define WRITE_INTEGER(integer, buffer, pos) (buffer)[(pos)] = (char)((integer)>>24); \
				(buffer)[(pos)+1] = (char)((integer)>>16); \
				(buffer)[(pos)+2] = (char)((integer)>>8); \
				(buffer)[(pos)+3] = (char)(integer)

/**
  * Get a line string (that start by a non-whitespace character) and return the corresponding
  * informations into the given ParsedOpcode structure.
  * Return 0 if success or an error code (negative value) else :
  * -1 : illegal character
  * -2 : more than 2 arguments
  * -3 : unknow argument addressing mode
  * -4 : a number can't be converted into integer (too large)
  * -5 : a comma is missing between two arguments
  */
int parseOpcodeLine(const char *line, struct ParsedOpcode *opstruct);


/**
  * Get a ParsedOpcode structure and return the corresponding binary opcode
  * If an opcode argument is a Label reference, this function can't solve it, so you
  * must solve it manualy before to call opcodeValue(), to transform it into an 'disp' PC value.
  * Return 0 if success, -1 if the opcode don't exist.
  */
int opcodeValue(const struct ParsedOpcode *op, unsigned short *value);


/**
  * Convert a string into a number, supported many 
  * Accept the following number formats :
  * Decimal : 		^[\+\-]?[0-9]+			examples : -5421 ; +75 ; 645
  * Hexadecimal :	^0x[0-9A-Fa-f]+			examples : 0x541 ; 0xA221 ; 0x5bc6f
  * Binary :		^0b[01]+				examples : 0b10010 ; 0b00011101
  *
  * In case of success, return 2 if the value must be unsigned (binary or hexa notation), 1 else (decimal number)
  * In case of error, a negative value is returned :
  * -1 : the string pattern doesn't match any supported format
  * -2 : the number is too large to be converted into an integer
  */
int stringToNumber(const char *str, int *value);


#endif
