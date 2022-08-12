#include "system.h"

#include <iostream>
#include <string>

#include "const.h"

int main(int argc, char **argv) {
  char *filename;
  SystemResult err;
	uint64_t initaddr = 0;

  if (argc < 2) {
    fprintf(stderr, "usage; %s <filename>\n", argv[0]);
  }
  filename = argv[1];

	if (argc == 3){
		initaddr = std::stoi(argv[2], nullptr, 16);
	}

  core::System system(filename, initaddr);

  err = system.Init();
  if (err != SystemResult::Success) {
    return 1;
  }

  system.execute_loop();
}
