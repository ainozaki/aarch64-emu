#include "system.h"

#include <iostream>
#include <string>

#include "const.h"

int main(int argc, char **argv) {
  char *filename;
  uint64_t initaddr = 0;

  if (argc < 2) {
    printf("usage: %s <filename>\n", argv[0]);
    return 0;
  }
  filename = argv[1];

  if (argc == 3) {
    initaddr = std::stoi(argv[2], nullptr, 16);
  }

  System system(filename, initaddr);
  if (system.Init() != SysResult::Success) {
    return -1;
  }
  system.execute_loop();
}
