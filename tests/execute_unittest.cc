#include <gtest/gtest.h>

#include "arm.h"

TEST(Execute, ADD_SUB){
	Cpu cpu;
  cpu.execute(0x91004001); /* ADD X1, X0, #0x10 */
	EXPECT_EQ(0x10, cpu.xregs_[1]);
  cpu.execute(0xd1000421); /* SUB X1, X0, #0x1 */
	EXPECT_EQ(0x0f, cpu.xregs_[1]);
}

TEST(Execute, ADDS_SUBS){
	Cpu cpu;
  cpu.execute(0xf1004021); /* SUBS X1, X0, #0x10 */
	EXPECT_EQ(-0x10, cpu.xregs_[1]);
	EXPECT_EQ(1, cpu.cpsr_.N);
	EXPECT_EQ(0, cpu.cpsr_.Z);
  
	cpu.execute(0xb1004021); /* ADDS X1, X1, #0x10 */
	EXPECT_EQ(0, cpu.xregs_[1]);
	EXPECT_EQ(0, cpu.cpsr_.N);
	EXPECT_EQ(1, cpu.cpsr_.Z);
}
