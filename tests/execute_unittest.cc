#include <gtest/gtest.h>

#include "system.h"

TEST(DataProcessingImm, ADD_SUB){
	core::System sys;

  sys.decode_start(0x91004000); /* ADD X0, X0, #0x10 */
	EXPECT_EQ(0x10, sys.cpu().xregs[0]);
  sys.decode_start(0xd1000400); /* SUB X0, X0, #0x1 */
	EXPECT_EQ(0x0f, sys.cpu().xregs[0]);

  sys.decode_start(0xf1004021); /* SUBS X1, X0, #0x10 */
	EXPECT_EQ(-0x10, sys.cpu().xregs[1]);
	EXPECT_EQ(1, sys.cpu().cpsr.N);
	EXPECT_EQ(0, sys.cpu().cpsr.Z);

	sys.decode_start(0xb1004021); // ADDS X1, X1, #0x10 //
	EXPECT_EQ(0, sys.cpu().xregs[1]);
	EXPECT_EQ(0, sys.cpu().cpsr.N);
	EXPECT_EQ(1, sys.cpu().cpsr.Z);
}

TEST(DataProcessingImm, MoveWide){
	core::System sys;

  sys.decode_start(0x91404042); /* ADD X2, X2, 0x10, LSL #12 */
	EXPECT_EQ(0x10000, sys.cpu().xregs[2]);
  
	sys.decode_start(0x72800002); /* MOVK W2, #0 */
	EXPECT_EQ(0x10000, sys.cpu().xregs[2]);

  sys.decode_start(0x52800002); /* MOVZ W2, #0 */
	EXPECT_EQ(0, sys.cpu().xregs[2]);
}

TEST(DataProcessingImm, Logical){
	core::System sys;

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
