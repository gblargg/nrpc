// NES remote procedure calls core

#ifndef BLARGG_NRPCC_H
#define BLARGG_NRPCC_H

#ifdef __cplusplus
	extern "C" {
#endif

//// Setup

// User-defined function called to output each byte to NES. Write to file/serial port/etc.
typedef void (*nrpcc_out_f)( unsigned char data );

// Initializes library and sets mode, and uploads shell to bootloader running on NES
void nrpcc_init( int flags, nrpcc_out_f hook );
enum {
	nrpcc_ntsc   = 0,
	nrpcc_pal    = 0x02, // PAL instead of NTSC/Dendy timing
	nrpcc_fast   = 0x01, // faster serial (85% faster async, 28% faster sync)
	nrpcc_sync   = 0x04  // synchronous serial
};
enum {
	nrpcc_baud      =  57600, // 8N1
	nrpcc_baud_fast = 115200  // 8N2 (2 stop bits)
};

// Sets controller open-bus value. Normally 0x40. PowerPak needs 0xe0. Only used by
// synchronous fast serial.
void nrpcc_set_openbus( int open_bus );


//// Memory

// Writes/fills to size bytes starting at addr
void nrpcc_write_byte( int addr, unsigned char value );
void nrpcc_write_mem( int addr, const unsigned char in [], int size );
void nrpcc_fill_mem( int addr, unsigned char fill, int size );

// Writes/fills all bytes to I/O port
void nrpcc_write_port( int addr, const unsigned char in [], int size );
void nrpcc_fill_port( int addr, unsigned char fill, int size );

// Disables rendering and sets PPU address
void nrpcc_set_ppuaddr( int addr );

// Writes/fills to size bytes starting at addr in PPU space
void nrpcc_write_ppu( int addr, const unsigned char in [], int size );
void nrpcc_fill_ppu( int addr, unsigned char fill, int size );


//// User commands

// Calls nrpcc_run_code() then nrpcc_delay_cyc()
void nrpcc_command( const unsigned char code [], int size, int cyc );

// Calls nrpcc_run_code() then nrpcc_send_data()
void nrpcc_upload( const unsigned char code [], int code_size,
		const unsigned char data [], int data_size );

// Calls nrpcc_run_code() then nrpcc_delay_bytes()
void nrpcc_download( const unsigned char code [], int code_size, int download_size );

// JSR addr
void nrpcc_jsr( int addr );

// Uploads data to nrpcc_run_addr then JSRs to nrpcc_run_addr
enum { nrpcc_run_addr = 0x200 };
void nrpcc_run_code( const unsigned char in [], int size );

// Sends data to running code
void nrpcc_send_block( const unsigned char in [], int size );

// Allows code to take up to n cycles before next operation
void nrpcc_delay_cycles( int n );

// Allows code to send n bytes back to PC
void nrpcc_delay_bytes( int n );


//// Debugging

// Writes character to PC
void nrpcc_debug_char( char c );

// Makes beep
void nrpcc_debug_beep( void );

// Stops shell in infinite loop
void nrpcc_debug_stop( void );

#ifdef __cplusplus
	}
#endif

#endif
