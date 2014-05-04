; Exits back to shell. Equivalent to returning.
exit				= $10 + 3*0

; Reports error by sending byte to host, changing screen color, making an ugly sound,
; and stopping shell.
; In: A = error byte, preferably a character from '0' to '9'
error				= $10 + 3*1

; Writes byte to host
; Preserved: A, X, Y
serial_write		= $10 + 3*2

; Begins receiving block of data from host
; Preserved: X, Y
begin_block			= $10 + 3*3

; If even more time is needed, use two stop bits for serial data. This adds
; 29 cycles to the maximum below without affecting the minimums.
; If using synchronous serial, the timing requirements below can be ignored.

; Reads next byte in block
; Out: A = byte
; Preserved: X, Y
; Timing: 8-14 cycles of code between JSR serial_read
serial_read			= $10 + 3*4 + 2

; $00-$09 are free for use and not modified by the shell
; $0a-$17f are reserved for the shell
; $180-$7ff can be used in any way
; $200-$2ff is where routine's code is uploaded

; On entry, A, X, and Y have garbage, and S is in the range $f0-$ff
