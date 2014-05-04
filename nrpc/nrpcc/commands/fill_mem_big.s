; nes_list; exit

.include "command.inc"
.org $200

main:
	ldx #$77
	ldy #$77
	lda #$77
@loop:
	sta $7777, y
	iny
	bne @loop
	inc @loop+2
	inx
	bne @loop
	rts
