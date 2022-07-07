.global test_as
.global subs_as
test_as:
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
