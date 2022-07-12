.global b_test_enter
.global b_test
call_forward:
	movz x1, #1
b_test2:
	b call_back
call_dummy:
	movz x2, #2
	ret
b_test:
	bl init_reg
	b call_forward
	b call_back
	movz x3, #3
b_test_enter:
	str  lr, [sp, #-16]! // push LR
	bl init_reg
	b call_forward
	b call_back
	movz x3, #3
init_reg:
	movz x1, #0
	movz x2, #0
	movz x3, #0
	movz x4, #0
	movz x5, #0
	ret
call_back:
	movz x4, #4
end_test:
	movz x5, #1  // end flag
	ldr  lr, [sp], #16    // pop LR
	ret
