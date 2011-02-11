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
#include "linker.h"
#include "g1a_utils.h"


unsigned char icon[68] = {0x00, 0x00, 0x00, 0x04, 0x13, 0x03, 0x24, 0x34, 0x2A, 0x84, 0x54, 0x44, 0x2A, 0xB4, 0x74, 0x44, 0x12, 0x83, 0x57, 0x34, 0x00, 0x00, 0x00, 0x04, 0x80, 0x06, 0x34, 0x44, 0xC0, 0x09, 0x46, 0xC4, 0xE0, 0x0F, 0x25, 0x44, 0xF0, 0x09, 0x14, 0x44, 0xF8, 0x09, 0x64, 0x44, 0xDC, 0x00, 0x00, 0x04, 0x8E, 0x00, 0x00, 0xFC, 0xAF, 0x00, 0x00, 0xFC, 0xAF, 0x80, 0x00, 0xFC, 0x8F, 0xC0, 0x00, 0xFC, 0xDF, 0xE0, 0x00, 0xFC};

inline void intToString (int n, char* string);

int main(int args, char** argv) {
	//asm2objfile("test.asm", "test.out");
	//linkobjfiles("test.out", "test.bin");
	FONTCHARACTER in[] = {'\\','\\','f','l','s','0','\\','C','R','T','0','.','s',0};
	FONTCHARACTER in2[] = {'\\','\\','f','l','s','0','\\','M','A','I','N','.','s',0};
	FONTCHARACTER bin[] = {'\\','\\','f','l','s','0','\\','C','R','T','0','.','o','b','j',0};
	FONTCHARACTER bin2[] = {'\\','\\','f','l','s','0','\\','M','A','I','N','.','o','b','j',0};
	FONTCHARACTER out[] = {'\\','\\','f','l','s','0','\\','O','U','T','.','g','1','a',0};

	FONTCHARACTER *obj_files[2];
	obj_files[0] = bin;
	obj_files[1] = bin2;

	char *state = "asm2objfile 1";

	int ret = asm2objfile(in, bin);
	if(ret >= 0) {
		ret = asm2objfile(in2, bin2);
		state = "asm2objfile 2";
	}
	if(ret >= 0) {
		ret = linkobjfiles(2, obj_files, out, FORMAT_G1A);
		state = "linkobjfiles";
	}
	if(ret < 0) {
		char error[20];
		intToString(ret, error);
		Bdisp_AllClr_DDVRAM();
		locate(1,1);
		Print("Error:");
		Print(error);
		locate(1,3);
		Print("State:");
		locate(3,4);
		Print(state);
		int key;
		GetKey(&key);
	}
	else {
		Bdisp_AllClr_DDVRAM();
		locate(1,1);
		Print("Sucess!");
		int key;
		GetKey(&key);
	}
}

inline void intToString (int n, char* string) {
	int  i;
	int  cpt;
	int start = 0;
  	
	if (n<0) {
		start=1;
		string[0] = '-';
		n *= -1;
	}
	for (i = 1, cpt = 1; n / i >= 10; i *= 10, cpt++);
	for (cpt = start; i; cpt++, i /= 10) string[cpt] = (n / i) % 10 + '0';
	string[cpt] = '\0';
}
