// Core of nrpcc; everything else is layered on this

#include "nrpcc.h"
#include "nrpcc_shell.h"

#include <string.h>
#include <assert.h>

/* Copyright (C) 2014 Shay Green <gblargg@gmail.com>
Copying and distribution of this file, with or without modification, are
permitted in any medium without royalty provided the copyright notice and
this notice are preserved. This file is offered as-is, without any warranty.*/

typedef unsigned char byte;
enum { block_signature = 0xd3 }; // least-common byte and reading in middle can't yield same value
enum { nrpcc_op_max_size = 256 };

static int flags;
static nrpcc_out_f hook;
static int out_crc;
static int sync_fast_open_bus = 0x40;
static int sync_fast_xor = 0x7f;

static int is_async( void ) { return !(flags & nrpcc_sync); }

static void write_out( int in )
{
	// reverse bits
	int out = in;
	int i;
	for ( i = 0; i < 8; i++ )
		out = (out >> 1) | (in << i & 0x80);
	
	hook( out ^ 0xff );
}

void nrpcc_set_openbus( int open_bus )
{
	sync_fast_open_bus = open_bus;
	sync_fast_xor = 0;
	int i;
	for ( i = 0; i < 8; i++ )
		if ( open_bus >> i & 1 )
			sync_fast_xor ^= 0x3f8 << i;
	
	sync_fast_xor ^= sync_fast_xor >> 9;
	sync_fast_xor &= 0x1ff;
	
}

static int sync_fast_encode( int in, int crc )
{
	int out = crc ^ sync_fast_xor;
	out = (out << 7 & 0x180) | (out >> 2 & 0x7f);
	
	int t = (in ^ out ^ (out >> 8)) & 1;
	in -= sync_fast_open_bus + t + (out >> 8 & 1);
	out = ((out ^ in) & 0xfe) | t;
	
	return out;
}

static void write_out_crc( int in )
{
	// One-time action just after shell has been started
	if ( out_crc < 0 )
	{
		out_crc = 0;
		
		if ( flags & nrpcc_sync )
			write_out_crc( 0 );
	}
	
	if ( is_async() )
	{
		write_out( in );
		out_crc ^= in;
	}
	else if ( flags & nrpcc_fast )
	{
		write_out( sync_fast_encode( in, out_crc ) );
		out_crc = in;
	}
	else
	{
		write_out( out_crc ^ in );
		out_crc = in;
	}
}

static void write_delay( int n )
{
	while ( n-- )
		write_out( 0xff );
}

void nrpcc_delay_bytes( int n )
{
	assert( n >= 0 );
	
	if ( !(flags & nrpcc_sync) )
		n += n / 5; // 10% margin for error
	
	write_delay( n );
}

void nrpcc_delay_cycles( int cyc )
{
	assert( cyc >= 0 );
	
	if ( !(flags & nrpcc_sync) )
	{
		enum { // Assume 5% higher rate
			slow = nrpcc_baud      * 105 / 100 / 10,
			fast = nrpcc_baud_fast * 105 / 100 / 11 // one extra stop bit
		};
		
		int clock = (flags & nrpcc_pal) ? 1662607 : 1789772;
		int bps = (flags & nrpcc_fast) ? fast : slow;
		int cyc_per_byte = clock / bps;
		write_delay( (cyc + (cyc_per_byte - 100)) / cyc_per_byte );
	}
}

static void do_op( byte op, int addr, const unsigned char in [], int size, int indexed )
{
	if ( size == 0 )
		return;
	
	assert( 1 <= size && size <= nrpcc_op_max_size );
	
	if ( is_async() )
		op ^= 0xff;
	
	int x = 256 - size;
	if ( indexed )
		addr -= x;
	byte h [4] = { op, addr & 0xff, addr >> 8, x };
	
	int i;
	for ( i = 0; i < (int) sizeof h; i++ )
		write_out_crc( h [i] );
	
	if ( is_async() )
	{
		out_crc ^= 0xff;
		write_out_crc( out_crc );
	}
	else
	{
		write_out_crc( 0 );
	}
	
	if ( in )
	{
		for ( i = 0; i < size; i++ )
			write_out_crc( in [i] );
	}
}

void nrpcc_jsr( int addr )
{
	// JMP (size=1 passes $ff to load into X for TXS before JSR)
	do_op( 0x4c, addr, 0, 1, 0 );
	if ( is_async() )
		write_delay( 1 );
}

static void nrpcc_op_write_mem( int addr, const unsigned char in [], int size )
{
	do_op( 0x9d, addr, in, size, 1 ); // STA $xxxx,X
}

static void nrpcc_op_write_port( int addr, const unsigned char in [], int size )
{
	do_op( 0x8d, addr, in, size, 0 ); // STA $xxxx
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
	nrpcc_write_( addr, 1, in, size, nrpcc_op_write_mem );
}

void nrpcc_write_port( int addr, const byte in [], int size )
{
	nrpcc_write_( addr, 0, in, size, nrpcc_op_write_port );
}

static byte calc_crc( const byte in [], int size )
{
	byte crc = 0;
	while ( size-- )
		crc ^= *in++;
	return crc;
}

void nrpcc_send_block( const byte in [], int size )
{
	assert( size >= 0 );
	
	if ( is_async() )
	{
		out_crc ^= 0xff;
		write_out_crc( block_signature );
		write_out_crc( out_crc ^ calc_crc( in, size ) );
	}
	
	while ( size-- )
		write_out_crc( *in++ );
	
	if ( is_async() )
		write_delay( 1 );
}

/* Converts 256-byte block of user code (beginning at offset 4)
into 256-byte program block ready to send to boot loader */
enum { boot_block_size = 256 };
enum { boot_block_offset = 4 };
static void make_boot_block( byte block [boot_block_size] )
{
	int i, crc = 0;
	for ( i = boot_block_size - 1; i >= boot_block_offset - 1; i-- )
	{
		crc += 0x100 - 0x99;
		crc = (crc << 7 & 0x80) | (crc >> 1 & 0x7F);
		crc ^= block [i];
	}
	block [0]  = 0xdc;
	block [1]  = 0x4b;
	block [2]  = 0xd2;
	block [3] ^= crc ^ 0xcb;
}

void nrpcc_init( int new_flags, nrpcc_out_f new_hook )
{
	hook = new_hook;
	flags = new_flags;
	int index = flags;
	if ( flags & nrpcc_sync )
		index &= ~nrpcc_pal;
	
	byte shell [256];
	memcpy( shell, &nrpcc_shell [index * 256], sizeof shell );
	make_boot_block( shell );
	int i;
	for ( i = 0; i < boot_block_size; i++ )
		hook( shell [i] );
	
	nrpcc_delay_cycles( 8700 );
	
	out_crc = -1;
}
