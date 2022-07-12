.global main
.global init_reg
.global test_as
init_reg:
	movz x1, #0
	movz x2, #0
	movz x3, #0
	movz x4, #0
	ret
call_forward:
	movz x4, #4
	ret
start_test:
	str  lr, [sp, #-16]! // push LR
	bl init_reg
test_as:
	bl call_forward
	bl call_back
	movz x1, #1
end_test:
	ldr  lr, [sp], #16    // pop LR
	ret
call_dummy:
	movz x2, #2
	ret
call_back:
	movz x3, #3
	ret

/*
.global init_reg
.global adds_as
.global subs_as
.global test_as
.global test2_as
.global test3_as
init_reg:
	movz x1, #0
	movz x2, #0
	movz x3, #0
adds_as:
	adds w1, w0, 0
	adds w1, w0, 1 
	adds w1, w0, 0x7ff
	adds w1, w0, 0x800
	adds w1, w0, 0x801
	adds w1, w0, 0xfff
	ret
subs_as:
 subs w1, w0, 0
 subs w1, w0, 1 
 subs w1, w0, 0x7ff
 subs w1, w0, 0x800
 subs w1, w0, 0x801
 subs w1, w0, 0xfff
 ret
b_as:
	movz x1, #1
	b test3_as
	ret
test2_as:
	movz x2, #2
	ret
test3_as:
	movz x3, #3
	ret
*/
