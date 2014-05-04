; Synchronous, fast (relies on open-bus)

	jsr serial_read_first

def_vectors

serial_read:
	bit $4017
serial_read_first:
:	lsr $4017
	bcc :-
serial_read_inl_:
	lda $4017
	asl a
	.repeat 6
		eor $4017
		rol a
	.endrepeat
	eor $4017
crc = <* + 1
	eor #0
	sta <crc
begin_block:
	rts

	; Receive codelet
	; 1 STA/JMP 2 addr 1 -len n data
:	sta <codelet + 3, x
first_codelet_byte:
	jsr serial_read
	inx
	bne :-
	tay
	jsr serial_read
	bne data_error
codelet = <codelet_
	lda <codelet
	bpl other_cmd
	clc
core_loop:
	bit $4017
	bit $4017
	.repeat 7
		eor $4017
		rol a
	.endrepeat
	eor $4017
codelet_:
	sta $7777, y		; STA $nnnn,Y / STA $nnnn / JMP $nnnn
	iny
	bne core_loop
	adc #0
	sta <crc
	jmp main			; BRA
	
other_cmd:
	cmp #$50
	bcs send
	dex					; S = $ff
	txs
	jsr codelet

main:
	ldx #-4
	bne first_codelet_byte	; BRA

send:
	rol a
	sta <send_loop
send_loop:
	lda (<codelet+1), y	; LDA (codelet),Y / LDA (codelet,X)
	jsr serial_write
	iny
	bne send_loop
	beq main			; BRA

def_error

.include "serial_write_sync.s"
