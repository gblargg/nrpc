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
	eor <out_crc
	sta <out_crc
	rts
.else
	; 137
	
	sta <@temp
	eor <out_crc
	sta <out_crc	
@temp = <* + 1
	lda #0
	
	asl a
	jsr @five_bits
	ror a
	ora #$10
@five_bits:
	.repeat 4
		sta $4016
		bit $4017
		ror a
	.endrepeat
	sta $4016
	bit $4017
	rts
.endif
