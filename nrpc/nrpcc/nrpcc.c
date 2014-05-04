// Utilities implemented on top of core nrpcc functionality

#include "nrpcc.h"

#include <assert.h>

/* Copyright (C) 2014 Shay Green <gblargg@gmail.com>
Copying and distribution of this file, with or without modification, are
permitted in any medium without royalty provided the copyright notice and
this notice are preserved. This file is offered as-is, without any warranty.*/

enum { nrpcc_op_max_size = 256 };
typedef unsigned char byte;

void nrpcc_run_code( const byte in [], int size )
{
	nrpcc_write_mem( nrpcc_run_addr, in, size );
	nrpcc_jsr( nrpcc_run_addr );
}

void nrpcc_command( const byte code [], int size, int cyc )
{
	nrpcc_run_code( code, size );
	nrpcc_delay_cycles( cyc );
}

void nrpcc_upload( const byte code [], int code_size, const byte data [], int data_size )
{
	nrpcc_run_code( code, code_size );
	nrpcc_send_block( data, data_size );
}

void nrpcc_download( const byte code [], int code_size, int download_size )
{
	nrpcc_run_code( code, code_size );
	nrpcc_delay_bytes( download_size );
}

void nrpcc_debug_char( char c )
{
	const byte code [] = {
		0xa9,c,    		// lda #$77
		0x4c,0x16,0x00,	// jmp $0016
	};
	nrpcc_download( code, sizeof code, 1 );
}

void nrpcc_debug_beep( void )
{
	const unsigned char code [] = {
		0xa9,0x81,    	// lda #$81
		0xa2,0x4f,    	// ldx #$4f
		0x8d,0x15,0x40,	// sta $4015
		0x8d,0x00,0x40,	// sta $4000
		0x8e,0x01,0x40,	// stx $4001
		0x8e,0x02,0x40,	// stx $4002
		0x8d,0x03,0x40,	// sta $4003
		0x60,        	// rts
	};
	nrpcc_command( code, sizeof code, 24 );
}

void nrpcc_debug_stop( void )
{
	const unsigned char code [] = {
		0x4c,0x00,0x02, // jmp $200
	};
	nrpcc_command( code, sizeof code, 0 );
}

// Breaks operation into nrpcc_op_max_size-byte chunks
static void nrpcc_write_( int addr, int inc_addr, const byte in [], int remain,
		void (*func)( int addr, const byte in [], int size ) )
{
	assert( remain >= 0 );
	
	while ( remain )
	{
		int n = nrpcc_op_max_size;
		if ( n > remain )
			n = remain;
		
		func( addr, in, n );
		in += n;
		if ( inc_addr )
			addr += n;
		
		remain -= n;
	}
}

void nrpcc_write_mem( int addr, const byte in [], int size )
{
	void nrpcc_op_write_mem( int addr, const unsigned char in [], int size );
	nrpcc_write_( addr, 1, in, size, nrpcc_op_write_mem );
}

void nrpcc_write_port( int addr, const byte in [], int size )
{
	void nrpcc_op_write_port( int addr, const unsigned char in [], int size );
	nrpcc_write_( addr, 0, in, size, nrpcc_op_write_port );
}

// Breaks operation into nrpcc_op_max_size-byte chunks
static void nrpcc_read_( int addr, int inc_addr, int remain,
		void (*func)( int addr, int size ) )
{
	assert( remain >= 0 );
	
	while ( remain )
	{
		int n = nrpcc_op_max_size;
		if ( n > remain )
			n = remain;
		
		func( addr, n );
		if ( inc_addr )
			addr += n;
		
		remain -= n;
	}
}

void nrpcc_read_mem( int addr, int size )
{
	void nrpcc_op_read_mem( int addr, int size );
	nrpcc_read_( addr, 1, size, nrpcc_op_read_mem );
}

void nrpcc_read_port( int addr, int size )
{
	void nrpcc_op_read_port( int addr, int size );
	nrpcc_read_( addr, 0, size, nrpcc_op_read_port );
}

void nrpcc_fill_mem( int addr, byte fill, int size )
{
	assert( size >= 0 );
	if ( size <= 0 )
		return;
	
	int y = -size >> 0 & 0xff;
	int x = -size >> 8 & 0xff;
	addr -= y;
	
	if ( size <= 256 )
	{
		const byte code [] = {
			0xa0, y,    		// ldy #$ea
			0xa9, fill,    		// lda #$ea
			0x99, addr, addr>>8,// sta $eaea, y
			0xc8,        		// iny
			0xd0,0xfa,    		// bne $0004
			0x60,        		// rts
		};
		nrpcc_command( code, sizeof code, size*10 );
	}
	else
	{
		const unsigned char code [] = {
			0xa2, x,    		// ldx #$77
			0xa0, y,    		// ldy #$77
			0xa9, fill,    		// lda #$77
			0x99, addr, addr>>8,// sta $7777,y
			0xc8,        		// iny
			0xd0,0xfa,    		// bne $0006
			0xee,0x08,0x02,		// inc $0208
			0xe8,        		// inx
			0xd0,0xf4,    		// bne $0006
			0x60,        		// rts
		};
		nrpcc_command( code, sizeof code, size*10 + (256-x)*10 );
	}
}

void nrpcc_fill_port( int addr, byte fill, int size )
{
	assert( size >= 0 );
	if ( size <= 0 )
		return;
	
	int y = -size >> 0 & 0xff;
	int x = -size >> 8 & 0xff;
	
	if ( size <= 256 )
	{
		const byte code [] = {
			0xa0, y,    		// ldy #$ea
			0xa9, fill,   	 	// lda #$ea
			0x8d, addr, addr>>8,// sta $eaea
			0xc8,        		// iny
			0xd0,0xfa,    		// bne $0004
			0x60,        		// rts
		};
		nrpcc_command( code, sizeof code, size*9 );
	}
	else
	{
		const unsigned char code [] = {
			0xa2, x,    		// ldx #$77
			0xa0, y,    		// ldy #$77
			0xa9, fill,    		// lda #$77
			0x8d, addr, addr>>8,// sta $7777
			0xc8,        		// iny
			0xd0,0xfa,    		// bne $0006
			0xe8,        		// inx
			0xd0,0xf7,    		// bne $0006
			0x60,        		// rts
		};
		nrpcc_command( code, sizeof code, size*9 + (256-x)*10 );
	}
}

void nrpcc_write_byte( int addr, unsigned char value )
{
	nrpcc_write_port( addr, &value, 1 );
}

void nrpcc_set_ppuaddr( int addr )
{
	const unsigned char code [] = {
		0xa9,0x00,    	// lda #$00
		0x8d,0x00,0x20,	// sta $2000
		0x8d,0x01,0x20,	// sta $2001
		0x2c,0x02,0x20,	// bit $2002
		0xa9,addr>>8,   // lda #$77
		0x8d,0x06,0x20,	// sta $2006
		0xa9,addr&0xff, // lda #$77
		0x8d,0x06,0x20,	// sta $2006
		0x60,        	// rts
	};
	nrpcc_command( code, sizeof code, 28 );
}

void nrpcc_write_ppu( int addr, const byte in [], int size )
{
	nrpcc_set_ppuaddr( addr );
	nrpcc_write_port( 0x2007, in, size );
}

void nrpcc_fill_ppu( int addr, byte fill, int size )
{
	nrpcc_set_ppuaddr( addr );
	nrpcc_fill_port( 0x2007, fill, size );
}

void nrpcc_read_ppu( int addr, int size )
{
	int y = -size >> 0 & 0xff;
	int x = -size >> 8 & 0xff;
	
	const unsigned char code [] = {
		0xa9,0x00,    	// lda #$00
		0x8d,0x00,0x20,	// sta $2000
		0x8d,0x01,0x20,	// sta $2001
		0x2c,0x02,0x20,	// bit $2002
		0xa9, addr>>8,  // lda #$77
		0x8d,0x06,0x20,	// sta $2006
		0xa9, addr,    	// lda #$77
		0x8d,0x06,0x20,	// sta $2006
		0x2c,0x07,0x20,	// bit $2007
		0xa2, x,    	// ldx #$77
		0xa0, y,    	// ldy #$77
		0xad,0x07,0x20,	// lda $2007
		0x20,0x16,0x00,	// jsr $0016
		0xc8,        	// iny
		0xd0,0xf7,    	// bne $001c
		0xe8,        	// inx
		0xd0,0xf4,    	// bne $001c
		0x60,        	// rts
	};
	nrpcc_download( code, sizeof code, size );
}

void nrpcc_reset_crc( void )
{
	nrpcc_write_byte( 0x00ff, 0 );
}

void nrpcc_read_crc( void )
{
	nrpcc_read_mem( 0x00ff, 1 );
}
