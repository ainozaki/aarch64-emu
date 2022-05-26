#include <gtest/gtest.h>

#include "arm.h"

TEST(Execute, ADDSUB){
	Cpu cpu;
	cpu.xregs_[0] = 0;
	cpu.xregs_[1] = 0;
  cpu.execute(0x91004001); /* ADD X1, X0, #0x10*/
	EXPECT_EQ(0x10, cpu.xregs_[1]);
  cpu.execute(0xd1000421); /* SUB X1, X0, #0x1 */
	EXPECT_EQ(0x0f, cpu.xregs_[1]);
}
