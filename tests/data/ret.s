init_reg:
	movz x1, #0
	movz x2, #0
	movz x3, #0
test_as:
	b call_ok
	movz x1, #1
call_dummy:
	movz x2, #2
	ret
call_ok:
	movz x3, #3
	ret
