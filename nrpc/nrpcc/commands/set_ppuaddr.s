; nes_list; exit

.include "command.inc"

main:
	lda #0
	sta $2000	; be sure addr inc is 1
	sta $2001	; rendering off
	bit $2002	; reset high/low toggle
	lda #$77
	sta $2006
	lda #$77
	sta $2006
	rts
