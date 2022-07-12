.global b_test_enter
b_test_enter:
	str  lr, [sp, #-16]! // push LR
	bl init_reg
	bl test_b
	ldr  lr, [sp], #16    // pop LR
	ret
init_reg:
	movz x1, #0
	movz x2, #0
	movz x3, #0
test_b:
	movz x1, #1
	b test3_as
	ret
test2_as:
	movz x2, #2
	ret
test3_as:
	movz x3, #3
	ret
