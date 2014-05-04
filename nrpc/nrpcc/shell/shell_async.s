; Asynchronous

	lda #1		; idle state to avoid first byte to PC being garbage
	sta $4016
	ldx #$ff	; init stack
	txs

serial_temp_ = <$f

def_vectors

serial_read:
	lda #SERIAL_MASK
@wait:
	bit $4017
	beq @wait
	pha			; 9 delay equivalent to JSR + JMP
	pla
	nop
	
.if SERIAL_FAST

serial_read_inl_:	; 20/18 (+3.5 for start bit loop)
rxd = <rxd_		; avoid ca65 "suspicious" warning
	lda <rxd
	.if !SERIAL_PAL
		nop
	.endif
	lsr $4017	; 15
crc = <* + 1
	eor #0
	sta <crc
	lda #$02
	rol a
	
	lsr $4017	; 16/14
	rol a
	bcc @first	; BRA
	
@loop:
	lsr $4017	; 15/14 (16/14 last time)
	rol a
	.if SERIAL_PAL
		bcs @end
		ora #0
		nop
	.else
		nop
		bcs @end
		bit <serial_temp_
	.endif
	
	lsr $4017	; 16/15
	rol a
	.if SERIAL_PAL
		ora #0
		nop
	@first:
	.else
		bit <serial_temp_
	@first:
		nop
	.endif
	bcc @loop	; BRA
	
@end:
	bit <serial_temp_
	
	lsr $4017
	rol a
	sta <rxd
	rts

rxd_:
	.byte 0

.else

serial_read_inl_:
rxd = <* + 1		; update CRC
	lda #0
crc = <* + 1		; avoid ca65 "suspicious" warning
	eor #0
	sta <crc
	
	lda #$02		; do 7 iterations until bit shifts out top
	clc
	bcc @first
@loop:
	nop
	and $4017		; read and mask bit
	cmp #1
@final = <@final_
	lda <@final
	rol a
	
	nop				; delay
	nop
	nop
	.if SERIAL_PAL
		ora #0
	.endif
@first:
	.if !SERIAL_PAL
		nop
		nop
	.endif
	
	sta <@final
	lda #SERIAL_MASK
	bcc @loop
	
	bit <@final
	and $4017		; final bit
	cmp #1
@final_ = <* + 1
	lda #0
	rol a
	sta <rxd
	rts

.endif

serial_sync:
	lda #SERIAL_MASK
:	bit $4017
	beq :-
:	bit $4017
	bne :-
	rts

begin_block:
	lda <rxd
	cmp <crc
	bne data_error
	jsr serial_sync
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
	tay
	jsr serial_read
	eor <crc
	bne data_error
	bit <codelet
	bpl jsr_codelet
:	jsr serial_read
codelet:
	sta $7777, y		; STA $nnnn,Y / STA $nnnn / JMP $nnnn
	iny
	bne :-
	
wait_block:
	ldx #-3
:	jsr serial_read
	eor #$ff
	sta <rxd			; preserve CRC
	bne first_codelet_byte
	beq :-				; BRA

jsr_codelet:
	dex					; S = $ff
	txs
	jsr codelet
main:
	jsr serial_sync
	beq wait_block		; BRA

.if SERIAL_FAST

serial_write:
	pha
	eor <out_crc
	sta <out_crc
	
	; Start bit
	asl a
	sta $4016			; 15
	nop
	sec
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
	clc
	pha
@first:
	pla
	
	sta $4016			; 16/14
	ror a
	bne @loop
	rts

.else

serial_write:
	pha
	eor <out_crc
	sta <out_crc
	
	; Start bit
	lda #0
	sta $4016
	pla
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
	
	bit $4017			; compatibility with sync serial
	lda <serial_temp_
	and #$01
	ror <serial_temp_
	sta $4016
	beq @done
	nop
	clc
	bcc @loop
	
@done:
	bit $4017			; compatibility with sync serial
	rts

.endif
