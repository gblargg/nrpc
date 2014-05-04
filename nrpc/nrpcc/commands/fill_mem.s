; nes_list; exit

.include "command.inc"

main:
	ldy #$77
	lda #$77
@loop:
	sta $7777, y
	iny
	bne @loop
	rts
