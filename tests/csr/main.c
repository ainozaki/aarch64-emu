#include <stdio.h>
#include <stdint.h>

void test_csel();
int main(){
	uint64_t x1, x2, x3, x4, x5, x6, x7;
        test_csel();
	asm ("mov %0, x1" : "=r" (x1));
	asm ("mov %0, x2" : "=r" (x2));
	asm ("mov %0, x3" : "=r" (x3));
	asm ("mov %0, x4" : "=r" (x4));
	asm ("mov %0, x5" : "=r" (x5));
	asm ("mov %0, x6" : "=r" (x6));
	asm ("mov %0, x7" : "=r" (x7));
	printf("x1 = 0x%lx\n", x1);
	printf("x2 = 0x%lx\n", x2);
	printf("x3 = 0x%lx\n", x3);
	printf("x4 = 0x%lx\n", x4);
	printf("x5 = 0x%lx\n", x5);
	printf("x6 = 0x%lx\n", x6);
	printf("x7 = 0x%lx\n", x7);
        return 0;
}
