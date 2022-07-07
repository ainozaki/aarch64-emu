.global adds_as
adds_as:
	adds w1, w0, 0
	adds w1, w0, 1 
	adds w1, w0, 0x7ff
	adds w1, w0, 0x800
	adds w1, w0, 0x801
	adds w1, w0, 0xfff
	ret
