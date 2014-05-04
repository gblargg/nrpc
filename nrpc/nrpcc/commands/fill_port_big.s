; nes_list; exit

.include "command.inc"
.org $200

main:
	ldx #$77
	ldy #$77
	lda #$77
@loop:
	sta $7777
	iny
	bne @loop
	inx
	bne @loop
	rts
