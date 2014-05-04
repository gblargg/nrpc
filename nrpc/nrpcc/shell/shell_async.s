; Asynchronous

	lda #1
	sta $4016
	nop

serial_temp_ = <$e
rxd = <$f

def_vectors

.macro serial_read_inl
	.local @wait
	lda #SERIAL_MASK
@wait:
	bit $4017
	beq @wait
	jsr serial_read_inl
	lsr $4017
	rol a
	sta <rxd
.endmacro

serial_read:
	serial_read_inl
	rts

.if SERIAL_FAST

serial_read_inl_:	; 20/18
	lda <rxd
	.if !SERIAL_PAL
		nop
	.endif
	lsr $4017	; 15
crc = <* + 1
	eor #$ea
	sta <crc
	lda #$04
	rol a
	
	lsr $4017	; 16/14
	rol a
	bcc @first	; BRA
	
@bit:
	lsr $4017	; 15/14
	rol a
	.if SERIAL_PAL
		bit <serial_temp_
	.else
		nop
		nop
	.endif
	bit <serial_temp_
	
	lsr $4017	; 16/15
	rol a
	.if SERIAL_PAL
		nop
		bcs @end
	@first:
	.else
		bit <serial_temp_
	@first:
		bcs @end
	.endif
	bcc @bit	; BRA
	
@end:
	nop
	
	lsr $4017	; 16
	rol a
	.if !SERIAL_PAL
		nop
	.endif
	rts

.else

serial_read_inl_:
	lda <rxd
crc = <* + 1
	eor #$ea
	sta a:crc
	lda #$02
	clc
	bcc :+
@bit:
	inc <serial_temp_
	lsr $4017
	nop
	nop
	nop
	nop
	rol a
	
	.if SERIAL_PAL
		ora #0
		bit <serial_temp_
	:
	.else
		bit <serial_temp_
	:	nop
		nop
	.endif
	
	bcc @bit
	rts

.endif

.macro serial_sync
	lda #SERIAL_MASK
:	bit $4017
	beq :-
:	bit $4017
	bne :-
.endmacro

begin_block:
	lda <rxd
	cmp <crc
	bne data_error
	serial_sync
:	jsr serial_read
	eor #$ff
	sta <rxd	; preserve CRC
	beq :-
	cmp #BLOCK_SIG ^ $ff
	beq serial_read

def_error

	; Receive codelet
	; 1 STA/JMP 2 addr 1 len n data
first_codelet_byte:
:	sta <codelet+3, x
	jsr serial_read
	inx
	bne :-
	tax
	jsr serial_read
	eor <crc
	bne data_error
	bit <codelet
	bpl jsr_codelet
:	jsr serial_read
codelet:
	sta $7777, x		; STA $xxxx,x / STA $xxxx / JMP $xxxx
	inx
	bne :-
	
wait_block:
	ldx #-3
:	jsr serial_read
	eor #$ff
	sta <rxd	; preserve CRC
	bne first_codelet_byte
	beq :-

jsr_codelet:
	txs					; S = $ff
	jsr codelet
main:
	serial_sync
	jmp wait_block

.if SERIAL_FAST

serial_write:
	; Start bit
	asl a
	sta $4016			; 15
	ror a
	sec
	sta a:@saved_a
	bcs @first
	
	; 8 data bits + stop bit
@loop:
	.if SERIAL_PAL
		inc <serial_temp_
	.else
		pha
		pla
	.endif

	sta $4016			; 15
	lsr a
	pha
	pla
	clc
	
@first:
	sta $4016			; 16/14
	ror a
	bne @loop
@saved_a = <* + 1
	lda #0
	rts

.else

serial_write:
	sta <@saved_a
	
	; Start bit
	lda #0
	sta $4016
	lda a:@saved_a
	sec
	sta <serial_temp_
	
	; 8 data bits + stop bit
@loop:
	.if SERIAL_PAL
		ora #0
	.else
		nop
		nop
	.endif
	
	bit $4017
	lda <serial_temp_
	and #$01
	ror <serial_temp_
	sta $4016
	beq @done
	nop
	clc
	jmp @loop
	
@done:
	bit $4017
@saved_a = <* + 1
	lda #0
	rts

.endif
