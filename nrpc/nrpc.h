/** NES remote procedure call recording interface \file */

#ifndef BLARGG_NRPC_H
#define BLARGG_NRPC_H

#ifdef __cplusplus
	extern "C" {
#endif

/** First parameter of most functions is a pointer to nrpc_t. */
typedef struct nrpc_t nrpc_t;

/** NES CPU or PPU address */
typedef int nrpc_addr_t;

/** Pointer to error string, or NULL if function was successful */
typedef const char* nrpc_err_t;


/**** Basics ****/

/** Returns pointer to a new recorder, or NULL if out of memory.
OR nrpc_flags below to set type of NES and serial method.
Currently only one recorder may exist at a time. */
nrpc_t* nrpc_new( int flags );
enum {
	nrpc_ntsc   = 0x00, /**< NTSC/Dendy NES */
	nrpc_pal    = 0x02, /**< PAL NES */
	nrpc_57600  = 0x00, /**< 57600 bps asynchronous serial */
	nrpc_115200 = 0x01, /**< 115200 bps asynchronous serial, less compatible */
	nrpc_sync   = 0x04, /**< Synchronous serial */
	nrpc_fast   = 0x01  /**< Faster synchronous serial, less compatible */
};

/** Deletes recorder, freeing memory. OK to pass NULL. */
void nrpc_delete( nrpc_t* );


/**** Writing ****/

/* Write functions transparently optimize long runs of same byte into fills. */

/** Writes byte in CPU address space */
void nrpc_write_byte( nrpc_t*, nrpc_addr_t, int value );

/** Writes buffer in CPU address space. */
void nrpc_write_mem( nrpc_t*, nrpc_addr_t, const unsigned char* in, int size );

/** Writes value bytes in CPU address space */
void nrpc_fill_mem( nrpc_t*, nrpc_addr_t, int value, int size );

/** Writes buffer to single address. */
void nrpc_write_port( nrpc_t*, nrpc_addr_t, const unsigned char* in, int size );

/** Writes value repeatedly to single address. */
void nrpc_fill_port( nrpc_t*, nrpc_addr_t, int value, int size );

/** Writes buffer in PPU address space */
void nrpc_write_ppu( nrpc_t*, nrpc_addr_t, const unsigned char* in, int size );

/** Writes value to bytes in PPU address space */
void nrpc_fill_ppu( nrpc_t*, nrpc_addr_t, int value, int size );


/**** User commands ****/

/** Command code is written to and executed from this address. */
#define nrpc_run_addr 0x200

/** Writes code to nrpc_run_addr, then JSR nrpc_run_addr. Command must return within
cyc cycles. */
void nrpc_command( nrpc_t*, const unsigned char* code, int size, int cycles );

/** Writes code to nrpc_run_addr, then JSR nrpc_run_addr, then sends data to command. */
void nrpc_upload( nrpc_t*, const unsigned char* code, int size,
		const unsigned char* data, int data_size );

/** Writes code to nrpc_run_addr, then JSR nrpc_run_addr, then makes appropriate
preparations to receive download_size bytes. You must do the actual reception. */
void nrpc_download( nrpc_t*, const unsigned char* code, int size, int download_size );

/** Calls routine already in memory at address */
void nrpc_jsr( nrpc_t*, nrpc_addr_t );

/** Writes size bytes of code to dest in NES memory, then JSRs to it at exec
Appends extra_size bytes from extra to routine's code. */
void nrpc_call_code( nrpc_t*, nrpc_addr_t dest, const unsigned char* in, int size,
		nrpc_addr_t exec, const unsigned char* extra, int extra_size );

/** Sends checksummed block of data to routine just called. The routine
should call read_block then receive all its bytes. Any block size is supported,
including greater than 64KB. */
void nrpc_send_block( nrpc_t*, const unsigned char* in, int size );

/** Delays allow routine to take more time before exiting back to loader. */

/** Delays by specified number of cycles before calling next routine. */
void nrpc_delay_cycles( nrpc_t*, int cycles );

/** Delays by specified number of serial bytes before calling next routine, so that it
can send that many bytes back to PC. */
void nrpc_delay_bytes( nrpc_t*, int bytes );


/**** Debugging ****/

/** Makes beep */
void nrpc_debug_beep( nrpc_t* );

/** Sends character back to PC */
void nrpc_debug_char( nrpc_t*, char );


/**** Recording access ****/

/** Saves recording to file, ready to send to NES */
nrpc_err_t nrpc_save_recording( nrpc_t*, const char path [] );

/** Returns pointer and size of recording data to send to NES. Does not clear it.
NULL if there was an error. */
const unsigned char* nrpc_recording( nrpc_t*, int* size_out );

/** Number of bytes at beginning of recording that must be sent at 57600 */
int nrpc_57600_count( const nrpc_t* );

/** Saves 57600 and 115200 portions of recording separately. If 115200 isn't
even enabled, writes zero-length file at path_57600. */
nrpc_err_t nrpc_save_split_recording( nrpc_t*, const char path_57600 [],
		const char path_115200 [] );

/** Clears recorded data */
void nrpc_clear_recording( nrpc_t* );

#ifdef __cplusplus
	}
#endif

#endif
