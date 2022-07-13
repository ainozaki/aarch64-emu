.global adds_test_enter
.global adds_test
adds_test:
	bl init_reg
	bl test_adds
	movz x5, #1
	ret
adds_test_enter:
	str  lr, [sp, #-16]! // push LR
	bl init_reg
	bl test_adds
	movz x5, #1
	ldr  lr, [sp], #16    // pop LR
	ret
init_reg:
	mov x0, 0
	mov x1, 0
	mov x5, 0
	ret
test_adds:
	mov x0, 0 
	ADDS W1, W0, 0x00000000
	ADDS W1, W0, 0x00000001
	ADDS W1, W0, 0x000007ff
  ADDS W1, W0, 0x00000800
  ADDS W1, W0, 0x00000801
  ADDS W1, W0, 0x00000fff
	mov x0, 1 
  ADDS W1, W0, 0x00000000
  ADDS W1, W0, 0x00000001
  ADDS W1, W0, 0x000007ff
  ADDS W1, W0, 0x00000800
  ADDS W1, W0, 0x00000801
  ADDS W1, W0, 0x00000fff
	mov x0, 0x7fffffff 
  ADDS W1, W0, 0x00000000
  ADDS W1, W0, 0x00000001
  ADDS W1, W0, 0x000007ff
  ADDS W1, W0, 0x00000800
  ADDS W1, W0, 0x00000801
  ADDS W1, W0, 0x00000fff
	mov x0, 0x80000000
  ADDS W1, W0, 0x00000000
  ADDS W1, W0, 0x00000001
  ADDS W1, W0, 0x000007ff
  ADDS W1, W0, 0x00000800
  ADDS W1, W0, 0x00000801
  ADDS W1, W0, 0x00000fff
	movz x0, 0x0001
	movk x0, 0x8000, LSL#16
  ADDS W1, W0, 0x00000000
  ADDS W1, W0, 0x00000001
  ADDS W1, W0, 0x000007ff
  ADDS W1, W0, 0x00000800
  ADDS W1, W0, 0x00000801
  ADDS W1, W0, 0x00000fff
	mov x0, 0xffffffff
  ADDS W1, W0, 0x00000000
  ADDS W1, W0, 0x00000001
  ADDS W1, W0, 0x000007ff
  ADDS W1, W0, 0x00000800
  ADDS W1, W0, 0x00000801
  ADDS W1, W0, 0x00000fff
	ret
