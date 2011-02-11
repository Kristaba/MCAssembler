#ifndef G1A_HEADER_H
#define G1A_HEADER_H

// From g1awrapper

struct G1A_Header {
	char magic[14];
	char checkbyte1;
	char stuff1;
	unsigned long inverted_filesize;
	char checkbyte2;
	char unknown[11];
	char name_start;
	char filename[8]; /* name used internally for settings etc? */
	char unknown_2[2];
	char estrip_count;
	char unknown_3[4];
	char version[10]; /* first 5 bytes shown in system menu */
	char unknown_4[2];
	char date[14];
	char unknown_5[2];
	char menu_icon[68];
	char estrip_1_data[80];
	char estrip_2_data[80];
	char estrip_3_data[80];
	char estrip_4_data[80];
	char unknown_6[4];
	char name[8];	/* shown in VERSION in the system menu. should be called 'title'? */
	char unknown_7[20];
	unsigned long filesize;
	char unknown_8[12];
};

#endif /* G1A_HEADER_H */
