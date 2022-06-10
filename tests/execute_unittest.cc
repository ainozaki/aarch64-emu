#include <gtest/gtest.h>

#include "system.h"

TEST(Execute, ADD_SUB){
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
