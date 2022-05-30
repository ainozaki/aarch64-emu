#include <gtest/gtest.h>

#include "arm.h"

TEST(Execute, ADD_SUB){
	Cpu cpu;
  cpu.execute(0x91004001); /* ADD X1, X0, #0x10 */
	EXPECT_EQ(0x10, cpu.xregs[1]);
  cpu.execute(0xd1000421); /* SUB X1, X0, #0x1 */
	EXPECT_EQ(0x0f, cpu.xregs[1]);
}

TEST(Execute, ADDS_SUBS){
	Cpu cpu;
  cpu.execute(0xf1004021); /* SUBS X1, X0, #0x10 */
	EXPECT_EQ(-0x10, cpu.xregs[1]);
	EXPECT_EQ(1, cpu.cpsr.N);
	EXPECT_EQ(0, cpu.cpsr.Z);
  
	cpu.execute(0xb1004021); /* ADDS X1, X1, #0x10 */
	EXPECT_EQ(0, cpu.xregs[1]);
	EXPECT_EQ(0, cpu.cpsr.N);
	EXPECT_EQ(1, cpu.cpsr.Z);
}

TEST(Execute, AND_EOR){
	Cpu cpu;
  cpu.execute(0x91004400); /* ADD X0, X0, #0x11 */
	EXPECT_EQ(0x11, cpu.xregs[0]);
  
	cpu.execute(0x927c0001); /* AND X1, X0, #0x10 */
	EXPECT_EQ(0x11, cpu.xregs[0]);
	EXPECT_EQ(0x10, cpu.xregs[1]);

	cpu.execute(0xb27c0002); /* ORR X2, X0, #0x10 */
	EXPECT_EQ(0x11, cpu.xregs[0]);
	EXPECT_EQ(0x11, cpu.xregs[2]);
}
