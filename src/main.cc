#include "system.h"

#include <iostream>

#include "const.h"

int main(int argc, char **argv) { 
	char *filename;
	SystemResult err;
	
	if (argc < 2){
		fprintf(stderr, "usage; %s <filename>\n", argv[0]);
	}
	filename = argv[1];
	core::System system(filename);
	
	err = system.Init();
	if (err != SystemResult::Success){
		return 1;
	}
	
	system.execute_loop();
}
