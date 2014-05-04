; nes_list; exit

.include "command.inc"

main:
	ldy #$77
	lda #$77
:	sta $7777
	iny
	bne :-
	rts
