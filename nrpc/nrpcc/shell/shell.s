; nrpc shell boot program to run from zero-page

.ifndef SERIAL_SYNC
	SERIAL_SYNC = 0
.endif
.ifndef SERIAL_FAST
	SERIAL_FAST = 0
.endif
SERIAL_MASK = $01 ; for clarity; only $01 supported
BLOCK_SIG = $d3

; Copyright (C) 2014 Shay Green <gblargg@gmail.com>
; Copying and distribution of this file, with or without modification, are
; permitted in any medium without royalty provided the copyright notice and
; this notice are preserved. This file is offered as-is, without any warranty.

	.res 4, 0
	ldx #$ff
	txs

out_crc = <$ff

.macro debug_dump_mem addr, count
debug_dump_used = 1
	ldy #0
:	lda addr, y
	jsr serial_write
	iny
	cpy #count
	bne :-
:	beq :-
.endmacro

.macro def_vectors
	.align $10, $ea
	jmp main
	jmp error
	jmp serial_write
	jmp begin_block
serial_read_inl:
	bne serial_read_inl_	; BRA
.endmacro

.macro def_error
data_error:
	lda #'1'

error:
	jsr serial_write
	
	; Make ugly sound that clearly indicates error
	lda #$3a
	sta $4015
	ldx #-16
:	sta $4000 - <-16, x
	inx
	bne :-
	
	; Colored screen as well
	lda #$21
	sta $2001
	
:	bne :-
.endmacro

.if SERIAL_SYNC
	.if SERIAL_FAST
		.include "shell_sync_fast.s"
	.else
		.include "shell_sync.s"
	.endif
.else
	.include "shell_async.s"
.endif

	.res 1 ; space for out_crc
