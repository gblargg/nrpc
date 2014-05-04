; Synchronous

:	lsr $4017
	bcc :-
	jsr serial_read_inl_

def_vectors

.macro serial_read_core
	bit $4017
:	lsr $4017
	bcc :-
@inl:
	lda $4017
	.repeat 7
		lsr $4017
		rol a
	.endrepeat
.endmacro

serial_read:
	serial_read_core
serial_read_inl_ = @inl
crc = <crc_ + 1
	eor <crc
	sta <crc
begin_block:
	rts

.macro serial_read_inl
	serial_read_core
crc_: ; extra label avoids "suspicious address expression" warning
	eor #0
	sta <crc
.endmacro

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
	bit <codelet
	bpl jsr_codelet
core_loop:
	serial_read_inl
codelet:
	sta $7777, y		; STA $nnnn,Y / STA $nnnn / JMP $nnnn
	iny
	bne core_loop
	beq main
	
jsr_codelet:
	dex					; S = $ff
	txs
	jsr codelet

main:
	ldx #-4
	bne first_codelet_byte	; BRA

def_error

.include "serial_write_sync.s"
