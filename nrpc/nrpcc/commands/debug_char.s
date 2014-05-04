; nes_list; exit

.include "command.inc"

	lda #$77
	jmp serial_write
