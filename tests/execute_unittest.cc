#include <gtest/gtest.h>

#include "system.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

TEST(DataProcessingImm, ADDS) {
	std::string qqq;
	uint64_t w0, ans, imm;
	uint32_t inst;
	char c;
	int cpsr[4];

  core::System sys("tests/data/adds.bin");
  sys.Init();

	std::ifstream f("tests/data/adds.txt");
	if (f.fail()){
		fprintf(stderr, "ifstream\n");
		return;
	}

	std::string s;
	while (getline(f, s)){
		printf("-----------------------\n");
		printf("[expected]\n");
		// imm
		std::istringstream ssimm(s);
		ssimm >> qqq >> std::hex >> imm;
		printf("imm = 0x%016lx\n", imm);
		
		// input
		getline(f,s);
		std::istringstream ssin(s);
		ssin >> qqq >> std::hex >> w0;
		printf("w0  = 0x%016lx\n", w0);
		
		// ans
		getline(f,s);
		std::istringstream ssans(s);
		ssans >> qqq >> std::hex >> ans;
		printf("ans = 0x%016lx\n", ans);

		// flag
		getline(f,s);
		std::istringstream ssflag(s);
		ssflag >> qqq;
		for (int i = 0; i < 4; i++){
			ssflag >> c;
			cpsr[i] = (c != '-');
		}
		printf("n=%d z=%d c=%d v=%d\n", cpsr[0], cpsr[1], cpsr[2], cpsr[3]);

		// execute
		printf("[actual]\n");
		inst = sys.fetch();
		sys.cpu().xregs[0] = w0;
		sys.decode_start(inst);
		sys.cpu().increment_pc();

		EXPECT_EQ(sys.cpu().xregs[1], ans);
		EXPECT_EQ(cpsr[0], sys.cpu().cpsr.N);
		EXPECT_EQ(cpsr[1], sys.cpu().cpsr.Z);
		EXPECT_EQ(cpsr[2], sys.cpu().cpsr.C);
		EXPECT_EQ(cpsr[3], sys.cpu().cpsr.V);
	}
}

TEST(DataProcessingImm, MoveWide) {
  core::System sys("");
  sys.Init();

  sys.decode_start(0x91404042); /* ADD X2, X2, 0x10, LSL #12 */
  EXPECT_EQ(0x10000, sys.cpu().xregs[2]);

  sys.decode_start(0x72800002); /* MOVK W2, #0 */
  EXPECT_EQ(0x10000, sys.cpu().xregs[2]);

  sys.decode_start(0x52800002); /* MOVZ W2, #0 */
  EXPECT_EQ(0, sys.cpu().xregs[2]);
}

TEST(DataProcessingImm, Logical) {
  core::System sys("");
  sys.Init();

  sys.decode_start(0xd2802201); /* MOV X1, #0x110 */
  EXPECT_EQ(0x110, sys.cpu().xregs[1]);

  sys.decode_start(0x92780023); /* AND X3, X1, #0x100 */
  EXPECT_EQ(0x100, sys.cpu().xregs[3]);

  sys.decode_start(0xb2780024); /* ORR X4, X1, #0x100 */
  EXPECT_EQ(0x110, sys.cpu().xregs[4]);

  sys.decode_start(0xd2780025); /* EOR X5, X1, #0x100 */
  EXPECT_EQ(0x010, sys.cpu().xregs[5]);

  sys.decode_start(0xf2400026); /* ANDS X6, X1, #0x1 */
  EXPECT_EQ(0x0, sys.cpu().xregs[6]);
  EXPECT_EQ(0, sys.cpu().cpsr.N);
  EXPECT_EQ(1, sys.cpu().cpsr.Z);
}

TEST(DataProcessingImm, Bitfield) {
  core::System sys("");
  sys.Init();

  sys.decode_start(0xd2801e22); /* MOV X2, #0x00f1 */
  sys.decode_start(0xd2820203); /* MOV X3, #0x1010 */
  EXPECT_EQ(0xf1, sys.cpu().xregs[2]);
  EXPECT_EQ(0x1010, sys.cpu().xregs[3]);

  sys.decode_start(0x93781c43); /* SBFI X3, X2, #8, #8 */
  EXPECT_EQ(0xfffffffffffff100, sys.cpu().xregs[3]);

  sys.decode_start(0xd2820203); /* MOV X3, #0x1010 */
  sys.decode_start(0xb3781c43); /* BFI X3, X2, #8, #8 */
  EXPECT_EQ(0xf110, sys.cpu().xregs[3]);

  sys.decode_start(0xd2820203); /* MOV X3, #0x1010 */
  sys.decode_start(0xd3781c43); /* BFI X3, X2, #8, #8 */
  EXPECT_EQ(0xf100, sys.cpu().xregs[3]);
}

TEST(Branch, BranchImm) {
  core::System sys("tests/test_branch.bin");
  sys.Init();
  sys.execute_loop();

  // B, BL
  EXPECT_EQ(0x1, sys.cpu().xregs[1]);
  EXPECT_NE(0x2, sys.cpu().xregs[2]);
  EXPECT_EQ(0x3, sys.cpu().xregs[3]);
  EXPECT_EQ(0x4, sys.cpu().xregs[4]);
  // CBZ, CBNZ
  EXPECT_NE(0x5, sys.cpu().xregs[5]);
  EXPECT_EQ(0x6, sys.cpu().xregs[6]);
}
