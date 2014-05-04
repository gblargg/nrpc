; nes_list; exit

.include "command.inc"
.org $200

main:
	lda #0
	sta $2000	; be sure addr inc is 1
	sta $2001	; rendering off
	bit $2002	; reset high/low toggle
	lda #$77
	sta $2006
	lda #$77
	sta $2006
	bit $2007	; fill buffer
	
	ldx #$77
	ldy #$77
@loop:
	lda $2007
	jsr serial_write
	iny
	bne @loop
	inx
	bne @loop
	rts
