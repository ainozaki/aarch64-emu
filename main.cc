#include "system.h"

#include <iostream>

int main(int argc, char **argv) { 
	char *filename;
	
	if (argc < 2){
		fprintf(stderr, "usage; %s <filename>\n", argv[0]);
	}
	filename = argv[1];
	core::System system(filename);
	system.execute_loop();
}
