serial_write:
.ifdef debug_dump_used ; saves ~50 bytes
	pha
	asl a
	sta $4016
	bit $4017
	ror a
	sec
:	sta $4016
	bit $4017
	ror a
	clc
	bne :-
	pla
.else
	asl a
	sta $4016
	bit $4017
	ror a
	sec
	
	.repeat 8
		sta $4016
		bit $4017
		ror a
	.endrepeat
	
	sta $4016
	bit $4017
	ror a
.endif
	eor <out_crc
	sta <out_crc
	rts
