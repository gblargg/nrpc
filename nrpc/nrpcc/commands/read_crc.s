; nes_list; exit

.include "command.inc"
.org $200

crc_out = <$ff

main:
	lda <crc_out
	jmp serial_write
