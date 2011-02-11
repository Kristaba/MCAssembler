#include "g1a_utils.h"
#include "g1a_header.h"

#ifndef TESTING_PC
#include <fxstdio.h>
#else
#include <stdio.h>
#endif

#include <string.h>

static unsigned char default_icon[68] = {0x00, 0x00, 0x00, 0x04, 0x13, 0x03, 0x24, 0x34, 0x2A, 0x84, 0x54, 0x44, 0x2A, 0xB4, 0x74, 0x44, 0x12, 0x83, 0x57, 0x34, 0x00, 0x00, 0x00, 0x04, 0x80, 0x06, 0x34, 0x44, 0xC0, 0x09, 0x46, 0xC4, 0xE0, 0x0F, 0x25, 0x44, 0xF0, 0x09, 0x14, 0x44, 0xF8, 0x09, 0x64, 0x44, 0xDC, 0x00, 0x00, 0x04, 0x8E, 0x00, 0x00, 0xFC, 0xAF, 0x00, 0x00, 0xFC, 0xAF, 0x80, 0x00, 0xFC, 0x8F, 0xC0, 0x00, 0xFC, 0xDF, 0xE0, 0x00, 0xFC};


int writeG1AFromFile(const FONTCHARACTER *bin, const FONTCHARACTER *outputname, const unsigned char *icon, const char *name) {

#ifndef TESTING_PC
	struct G1A_Header header;
	int i;
	int binaryHandle, outputHandle;

	// Open the binary file and return an error if the file can't be opened
	binaryHandle = Bfile_OpenFile(bin, _OPENMODE_READ);
	if(binaryHandle < 0) return -1; 	
	
	fillG1AHeader(&header, icon, name, Bfile_GetFileSize(binaryHandle));

	// Delete and/or create the G1A file and open it in write mode
	Bfile_DeleteFile(outputname);
	Bfile_CreateFile(outputname, Bfile_GetFileSize(binaryHandle) + 0x200);
	outputHandle = Bfile_OpenFile(outputname, _OPENMODE_WRITE);
	if(outputHandle < 0) return -3;

	// Write the header
	if(Bfile_WriteFile(outputHandle, (void*)(&header), 0x200) < 0) return -4;
	// Copy data
#warning Using Bfile_ReadFile and Bfile_WriteFile in a loop is not safe.
	//@TODO do the copy more safe ;)
	char readbuffer[16384];  // High size because the Bfile_*** fail after some call O_o
	int toRead, read;
	for(toRead = Bfile_GetFileSize(binaryHandle); toRead>0; toRead -= 16384) {
		if((read = Bfile_ReadFile(binaryHandle, readbuffer, 16384, -1)) < 0) return -2;
		if (Bfile_WriteFile(outputHandle, readbuffer, read) < 0) return -4;
	}

	Bfile_CloseFile(binaryHandle);
	Bfile_CloseFile(outputHandle);
	return 0;

#else
	return -1;
#endif
}



int writeG1AFromBuffer(unsigned char *bin, unsigned int binsize, const FONTCHARACTER *outputname, const unsigned char *icon, const char *name) {
	struct G1A_Header header;
	int outputHandle;
	
	fillG1AHeader(&header, icon, name, binsize);

#ifndef TESTING_PC
	// Delete and/or create the G1A file and open it in write mode
	Bfile_DeleteFile(outputname);
	Bfile_CreateFile(outputname, binsize + 0x200);
	outputHandle = Bfile_OpenFile(outputname, _OPENMODE_WRITE);
	if(outputHandle < 0) return -1;

	// Write the header and copy data
	if(Bfile_WriteFile(outputHandle, (void*)(&header), 0x200) < 0) return -2;
	if(Bfile_WriteFile(outputHandle, bin, binsize) < 0) return -2;

	Bfile_CloseFile(outputHandle);

#else
	FILE *outfile = NULL;
	outfile = fopen(outputname, "wb+");
	if(outfile == NULL) return -1;
	fwrite((void*)(&header), sizeof(char), 0x200, outfile);
	fwrite(bin, sizeof(char), binsize, outfile);
	fflush(outfile);
	fclose(outfile);
#endif

	return 0;
}




void fillG1AHeader(struct G1A_Header *header, const unsigned char *icon, const char *name, unsigned int binsize) {
	int i, tmp;

	/* read data into the struct */
	memset(header, 0, sizeof(struct G1A_Header)); /* initialize */
	memcpy(header->magic,"\xAA\xAC\xBD\xAF\x90\x88\x9A\x8D\x0C\xFF\xEF\xFF\xEF\xFF", sizeof(header->magic));
	header->stuff1 = 0xFE;
	#ifndef TESTING_PC
	header->inverted_filesize = ~(binsize + 0x200);		/* NOT'ed */
	#else
	tmp = ~(binsize + 0x200);
	header->inverted_filesize = (tmp<<24) + ((tmp&0x0000FF00)<<8) + ((tmp&0x00FF0000)>>8) + (tmp>>24);
	#endif
	
	/* write the checkbytes */
	char lowbyte = (char)(header->inverted_filesize & 0x000000FF);	/* last (LSB) byte in the long */
	header->checkbyte1 = lowbyte - 0x41;
	header->checkbyte2 = lowbyte - 0xB8;
	
	header->name_start = '@';
	// Copy both header->filename and header->name here
	int copy = 1;
	for(i=0; i<sizeof(header->filename); i++)
		if(copy) if((header->filename[i]=header->name[i]=name[i]) == 0) copy=1;
	header->estrip_count = 0;
	memcpy(header->version, "Open!.0000", 10);

	// Write a static date because the calculator's RTC isn't trusted
	memcpy(header->date, "1981.0511.1200", sizeof(header->date));

	// The icon bitmap's array size must be 68!
	memcpy(header->menu_icon, icon, sizeof(header->menu_icon));
	#ifndef TESTING_PC
	header->filesize = (binsize + 0x200); /* include size of header */
	#else
	tmp = binsize + 0x200;
	header->filesize = (tmp<<24) + ((tmp&0x0000FF00)<<8) + ((tmp&0x00FF0000)>>8) + (tmp>>24);
	#endif
}



const unsigned char *defaultIconG1A() {
	return default_icon;
}
