#include <cstdint>
#include <cstring>

#include <stdio.h>

extern "C" uint32_t adds_as(uint32_t);
extern "C" uint32_t test_as(uint32_t);

void create_as_adds(){
	const char *filename = "tests/data/adds.s";
	FILE *f;

	f = fopen(filename, "w");
	if (!f){
		perror("fopen");
		return;
	}
	const uint32_t regtbl[] = {0, 1, 0x7fffffff, 0x80000000, 0x80000001, 0xffffffff};
	const uint32_t immtbl[] = {0, 1, 0x7ff, 0x800, 0x801, 0xfff};
	const size_t regn = sizeof(regtbl) / sizeof(regtbl[0]);
	const size_t immn = sizeof(immtbl) / sizeof(immtbl[0]);

	printf("creating %s... ", filename);
	for (size_t i = 0; i < regn; i++){
		for (size_t j = 0; j < immn; j++){
			fprintf(f, "ADDS W1, W0, 0x%08x\n", immtbl[j]);
		}
		test_as(regtbl[i]);
	}
	printf("done\n");
}

void create_as_subs(){
	const char *filename = "tests/data/subs.s";
	FILE *f;

	f = fopen(filename, "w");
	if (!f){
		perror("fopen");
		return;
	}
	const uint32_t tbl[] = {0, 1, 0x7ff, 0x800, 0x801, 0xfff};
	const size_t n = sizeof(tbl) / sizeof(tbl[0]);

	printf("creating %s... ", filename);
	for (size_t i = 0; i < n; i++){
		for (size_t j = 0; j < n; j++){
			fprintf(f, "SUBS W1, W0, 0x%08x\n", tbl[j]);
		}
		test_as(tbl[i]);
	}
	printf("done\n");
}

void create_as(const char *asname){
	if (!strcmp(asname, "adds")){
		create_as_adds();
	}else if (!strcmp(asname, "subs")){
		create_as_subs();
	}else {
		fprintf(stderr, "asname %s not match\n", asname);
	}
};

int main(int argc, char *argv[]){
	if (argc < 2){
		fprintf(stderr, "usage: %s <as name>\n", argv[0]);
		return -1;
	}
	create_as(argv[1]);
}
