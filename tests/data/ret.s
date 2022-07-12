.global ret_test_enter
.global ret_test
call_forward:
	movz x4, #4
	ret
ret_test:
	bl init_reg
	bl call_forward
	bl call_back
	movz x1, #1
	ret
ret_test_enter:
	str  lr, [sp, #-16]! // push LR
	bl init_reg
	bl call_forward
	bl call_back
	movz x1, #1
	ldr  lr, [sp], #16    // pop LR
	ret
init_reg:
	movz x1, #0
	movz x2, #0
	movz x3, #0
	movz x4, #0
	ret
call_dummy:
	movz x2, #2
	ret
call_back:
	movz x3, #3
	ret
