; nes_list; exit

.include "command.inc"
.org $200

main:
	ldx #$77
	ldy #$77
@loop:
	lda $7777, y
	jsr serial_write
	iny
	bne @loop
	inc @loop+2
	inx
	bne @loop
	rts
