; nes_list; exit

.include "command.inc"

main:
	lda #$81
	ldx #$4f
	sta $4015
	sta $4000
	stx $4001
	stx $4002
	sta $4003
	
	rts
