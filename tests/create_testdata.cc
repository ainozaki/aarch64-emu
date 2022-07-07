#include <cstdint>

#include <stdio.h>

extern "C" uint32_t adds_as(uint32_t);

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

	printf("creating %s...", filename);
	for (size_t i = 0; i < regn; i++){
		for (size_t j = 0; j < immn; j++){
			fprintf(f, "ADDS W1, W0, 0x%08x\n", immtbl[j]);
		}
		adds_as(regtbl[i]);
	}
	printf("done\n");
}

int main(){
	create_as_adds();
}
