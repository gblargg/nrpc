// Stable interface to nrpcc

#include "nrpc.h"

#include "nrpcc/nrpcc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Copyright (C) 2014 Shay Green <gblargg@gmail.com>
Copying and distribution of this file, with or without modification, are
permitted in any medium without royalty provided the copyright notice and
this notice are preserved. This file is offered as-is, without any warranty.*/

enum { mem_size = 0x10000 };

//// Recording

struct nrpc_t { int nothing; };

static nrpc_t* nrpc;
static int flags;
static int nrpc_57600_count_;
static int recording_size;
static unsigned char* recording;

static void clear_recording( void )
{
	nrpc_57600_count_ = 0;
	recording_size = 0;
	free( recording );
	recording = 0;
}

static int rounded( int n )
{
	assert( n >= 0 );
	int const chunk = 24 * 1024;
	return (n + chunk-1)/chunk * chunk;
}

static void out_hook( unsigned char data )
{
	if ( recording_size < 0 )
		return;
	
	int current = rounded( recording_size );
	int needed  = rounded( recording_size + 1 );
	if ( needed > current )
	{
		void* p = realloc( recording, needed );
		if ( !p )
		{
			clear_recording();
			recording_size = -1;
			return;
		}
		recording = (unsigned char*) p;
	}
	
	recording [recording_size++] = data;
}

void nrpc_clear_recording( nrpc_t* m )
{
	clear_recording();
}

const unsigned char* nrpc_recording( nrpc_t* m, int* size_out )
{
	if ( recording_size < 0 )
		return 0;
	
	*size_out = recording_size;
	static unsigned char dummy;
	return recording ? recording : &dummy; // never return NULL since it signals error
}

int nrpc_57600_count( const nrpc_t* m )
{
	if ( flags & (nrpc_115200 | nrpc_sync) )
		return nrpc_57600_count_;
	
	return recording_size;
}

nrpc_t* nrpc_new( int new_flags )
{
	assert( !nrpc );
	static nrpc_t nrpc_;
	nrpc = &nrpc_;
	
	clear_recording();
	flags = new_flags;
	nrpcc_init( flags, out_hook );
	if ( flags & (nrpc_fast | nrpc_sync) )
		nrpc_57600_count_ = 256;
	nrpc_set_openbus( nrpc, 0x40 );
	
	return nrpc;
}

void nrpc_delete( nrpc_t* m )
{
	nrpc = 0;
	clear_recording();
}

void nrpc_call_code( nrpc_t* m, nrpc_addr_t dest, const unsigned char* in, int in_size,
		nrpc_addr_t exec, const unsigned char* extra, int extra_size )
{
	int size = in_size + extra_size;
	assert( 0x200 <= dest && dest + size <= mem_size );
	assert( dest <= exec && exec < dest + size );
	assert( 1 <= size && size <= 256 );
	
	unsigned char buf [256];
	memcpy( buf, in, in_size );
	memcpy( buf + in_size, extra, extra_size );
	nrpcc_write_mem( dest, buf, size );
	nrpcc_jsr( exec );
}

//// Simple wrappers

void nrpc_set_openbus( nrpc_t* m, int open_bus )
{
	nrpcc_set_openbus( open_bus );
}

void nrpc_command( nrpc_t* m, const unsigned char* in, int size, int cycles )
{
	nrpcc_command( in, size, cycles );
}

void nrpc_upload( nrpc_t* m, const unsigned char* code, int code_size,
		const unsigned char* data, int data_size )
{
	nrpcc_upload( code, code_size, data, data_size );
}

void nrpc_download( nrpc_t* m, const unsigned char* in, int size, int data_size )
{
	nrpcc_download( in, size, data_size );
}

void nrpc_write_byte( nrpc_t* m, nrpc_addr_t addr, int value )
{
	nrpcc_write_byte( addr, value );
}

void nrpc_fill_mem( nrpc_t* m, nrpc_addr_t addr, int value, int size )
{
	nrpcc_fill_mem( addr, value, size );
}

void nrpc_fill_port( nrpc_t* m, nrpc_addr_t addr, int value, int size )
{
	nrpcc_fill_port( addr, value, size );
}

void nrpc_fill_ppu( nrpc_t* m, nrpc_addr_t addr, int value, int size )
{
	nrpcc_fill_ppu( addr, value, size );
}

void nrpc_jsr( nrpc_t* m, nrpc_addr_t addr )
{
	nrpcc_jsr( addr );
}

void nrpc_send_block( nrpc_t* m, const unsigned char* in, int size )
{
	nrpcc_send_block( in, size );
}

void nrpc_delay_cycles( nrpc_t* m, int cycles )
{
	nrpcc_delay_cycles( cycles );
}

void nrpc_delay_bytes( nrpc_t* m, int bytes )
{
	nrpcc_delay_bytes( bytes );
}

void nrpc_debug_beep( nrpc_t* m )
{
	nrpcc_debug_beep();
}

void nrpc_debug_char( nrpc_t* m, char c )
{
	nrpcc_debug_char( c );
}

void nrpc_read_ppu( nrpc_t* m, nrpc_addr_t addr, int size )
{
	nrpcc_read_ppu( addr, size );
}

void nrpc_read_mem( nrpc_t* m, nrpc_addr_t addr, int size )
{
	nrpcc_read_mem( addr, size );
}

void nrpc_read_port( nrpc_t* m, nrpc_addr_t addr, int size )
{
	nrpcc_read_port( addr, size );
}

void nrpc_reset_crc( nrpc_t* m )
{
	nrpcc_reset_crc();
}

void nrpc_read_crc( nrpc_t* m )
{
	nrpcc_read_crc();
}

int nrpc_calc_crc( nrpc_t* m, const unsigned char* in, int count, int old_crc )
{
	return nrpcc_calc_crc( in, count, old_crc );
}


//// Optimized writing

// Breaks array into runs of the same byte, and runs that need to be copied.
// Returns length of next run to be copied, or negative length if it's a run
// of a byte repeated threshold or more times.
static int next_run( const unsigned char* in, int size )
{
	int threshold = 27;
	//extern int nrpc_test; threshold = nrpc_test;
	int i = 0;
	while ( i < size )
	{
		int start = i;
		while ( i < size && in [i] == in [start] )
			i++;
		
		if ( i - start >= threshold )
			return start ? start : -i;
	}
	
	return size;
}

void nrpc_write_mem( nrpc_t* m, nrpc_addr_t addr, const unsigned char* in, int size )
{
	//nrpcc_write_mem( addr, in, size ); return; // bypass optimization
	
	assert( 0 <= addr && addr <= mem_size );
	assert( 0 <= size && size <= mem_size - addr );
	
	// Avoid optimizing writes to lower part of memory where fill code will overwrite it
	int noopt = 0x220 - addr;
	if ( noopt > 0 )
	{
		// Do optimized FIRST, since it could overwrite earlier memory
		int remain = size - noopt;
		if ( remain > 0 )
			nrpc_write_mem( m, addr + noopt, in + noopt, remain );
		
		nrpcc_write_mem( addr, in, noopt );
		return;
	}
	
	while ( size > 0 )
	{
		int n = next_run( in, size );
		if ( n < 0 )
			nrpcc_fill_mem( addr, *in, (n = -n) );
		else
			nrpcc_write_mem( addr, in, n );
		
		addr += n;
		in   += n;
		size -= n;
	}
}

void nrpc_write_port( nrpc_t* m, nrpc_addr_t addr, const unsigned char* in, int size )
{
	//nrpcc_write_port( addr, in, size ); return; // bypass optimization
	
	assert( 0 <= addr && addr <= mem_size );
	assert( 0 <= size );
	
	while ( size > 0 )
	{
		int n = next_run( in, size );
		if ( n < 0 )
			nrpcc_fill_port( addr, *in, (n = -n) );
		else
			nrpcc_write_port( addr, in, n );
		
		in   += n;
		size -= n;
	}
}

void nrpc_write_ppu( nrpc_t* m, nrpc_addr_t addr, const unsigned char* in, int size )
{
	//nrpcc_write_ppu( addr, in, size ); return; // bypass optimization
	
	// only need to set ppuaddr once
	nrpcc_set_ppuaddr( addr );
	nrpcc_write_port( 0x2007, in, size );
}


//// Recording saving

static nrpc_err_t save_file( const char path [], const void* in, int size )
{
	FILE* out = fopen( path, "wb" );
	if ( out == NULL )
		return "couldn't create file";
	
	fwrite( in, 1, size, out );
	if ( fclose( out ) )
		return "couldn't write file";
	
	return NULL;
}

nrpc_err_t nrpc_save_split_recording( nrpc_t* o, const char path_57600 [],
		const char* path_115200 )
{
	nrpc_err_t err;
	const unsigned char* data;
	int remain = 0;
	int size;
	
	data = nrpc_recording( o, &size );
	if ( data == NULL )
		return "out of memory";
	
	if ( path_115200 )
	{
		remain = size;
		size = nrpc_57600_count( o );
		remain -= size;
	}
	
	err = save_file( path_57600, data, size );
	
	if ( !err && path_115200 )
		err = save_file( path_115200, data + size, remain );
	
	return err;
}

nrpc_err_t nrpc_save_recording( nrpc_t* o, const char path [] )
{
	return nrpc_save_split_recording( o, path, NULL );
}
