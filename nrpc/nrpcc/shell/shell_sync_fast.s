; Synchronous, fast (relies on open-bus)

:	lsr $4017
	bcc :-
	jsr serial_read_inl_

def_vectors

.macro serial_read_core load_crc
	bit $4017
	.if SERIAL_NOWAIT
		bit $4017
	.else
	:	lsr $4017
		bcc :-
	.endif
@inl:
	load_crc
	eor $4017
	asl a
	.repeat 6
		eor $4017
		rol a
	.endrepeat
	adc $4017
.endmacro

serial_read:
crc = <serial_read_inl_ + 1
	serial_read_core {lda #0}
	sta <crc
serial_read_inl_ = @inl
begin_block:
	rts

.macro serial_read_inl
	serial_read_core {}
.endmacro

	; Receive codelet
	; 1 STA/JMP 2 addr 1 -len n data
:	sta <codelet + 3, x
first_codelet_byte:
	jsr serial_read
	inx
	bne :-
	tax
	jsr serial_read
	bne data_error
	bit <codelet
	bpl jsr_codelet
core_loop:
	serial_read_inl
codelet:
	sta $7777, x		; STA $xxxx,x / STA $xxxx / JMP $xxxx
	inx
	bne core_loop
	sta <crc
	beq main
	
jsr_codelet:
	txs					; S = $ff
	jsr codelet

main:
	ldx #-4
	bne first_codelet_byte	; BRA

def_error

.include "serial_write_sync.s"
