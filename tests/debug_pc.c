#include "../assembler.h"
#include "../linker.h"
#include <stdio.h>


int main (int args, char **argv) {
	int ret;
	char* files[2];

	files[0] = "out2.obj";
	files[1] = "out.obj";

	ret = asm2objfile("in.txt", "out.obj");
	if(ret<0) printf("asm2objfile 1 error : %d\n", ret);
	else {
		ret = asm2objfile("in2.txt", "out2.obj");
		if(ret<0) printf("asm2objfile 2 error : %d\n", ret);
		else {
			ret = linkobjfiles(2, files, "mcatest.g1a", FORMAT_G1A);
			printf("linkobjfiles : %d\n", ret);
		}
	}

	return 0;
}
