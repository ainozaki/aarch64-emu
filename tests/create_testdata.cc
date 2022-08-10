#include <cstdint>
#include <cstring>

#include <stdio.h>

int endflag = -1;

extern "C" uint32_t test_as(uint32_t);
extern "C" uint32_t adds_test_enter();
extern "C" uint32_t subs_test_enter(uint32_t);
extern "C" uint32_t b_test_enter();
extern "C" uint32_t ret_test_enter();
extern "C" uint32_t sum_test_enter(uint32_t);
extern "C" uint32_t fibonacci_test_enter(uint32_t);

void create_as_adds() {
  const char *filename = "tests/data/adds.s";
  FILE *f;

  f = fopen(filename, "r");
  if (!f) {
    perror("fopen");
    return;
  }
  adds_test_enter();

  fclose(f);
  printf("done\n");
}

void create_as_subs() {
  const char *filename = "tests/data/subs.s";
  FILE *f;

  f = fopen(filename, "w");
  if (!f) {
    perror("fopen");
    return;
  }
  const uint32_t tbl[] = {0, 1, 0x7ff, 0x800, 0x801, 0xfff};
  const size_t n = sizeof(tbl) / sizeof(tbl[0]);

  printf("creating %s... ", filename);
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < n; j++) {
      fprintf(f, "SUBS W1, W0, 0x%08x\n", tbl[j]);
    }
    subs_test_enter(tbl[i]);
  }
  fclose(f);
  printf("done\n");
}

void create_as(const char *asname) {
  if (!strcmp(asname, "adds")) {
    create_as_adds();
  } else if (!strcmp(asname, "subs")) {
    create_as_subs();
  } else if (!strcmp(asname, "b")) {
    b_test_enter();
  } else if (!strcmp(asname, "ret")) {
    ret_test_enter();
  } else if (!strcmp(asname, "fun_sum")) {
    sum_test_enter(10);
  } else if (!strcmp(asname, "fun_fibonacci")) {
    fibonacci_test_enter(15);
  } else {
    fprintf(stderr, "asname %s not match\n", asname);
  }
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s <as name>\n", argv[0]);
    return -1;
  }
  create_as(argv[1]);
}
