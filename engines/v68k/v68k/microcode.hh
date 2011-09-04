/*
	microcode.hh
	------------
*/

#ifndef V68K_MICROCODE_HH
#define V68K_MICROCODE_HH

// C99
#include <stdint.h>


namespace v68k
{
	
	struct processor_state;
	
	
	const int max_params = 4;
	
	
	typedef void (*microcode)( processor_state&, uint32_t* );
	
	
	#pragma mark -
	#pragma mark Line 0
	
	void microcode_ORI_to_CCR( processor_state& state, uint32_t* params );
	
	void microcode_ORI_to_SR( processor_state& state, uint32_t* params );
	
	void microcode_MOVEP_to  ( processor_state& state, uint32_t* params );
	void microcode_MOVEP_from( processor_state& state, uint32_t* params );
	
	void microcode_ANDI_to_CCR( processor_state& state, uint32_t* params );
	
	void microcode_ANDI_to_SR( processor_state& state, uint32_t* params );
	
	void microcode_EORI_to_CCR( processor_state& state, uint32_t* params );
	
	void microcode_EORI_to_SR( processor_state& state, uint32_t* params );
	
	void microcode_MOVES( processor_state& state, uint32_t* params );
	
	#pragma mark -
	#pragma mark Lines 1-3
	
	void microcode_MOVE( processor_state& state, uint32_t* params );
	
	#pragma mark -
	#pragma mark Line 4
	
	void microcode_LEA( processor_state& state, uint32_t* params );
	
	void microcode_MOVE_from_CCR( processor_state& state, uint32_t* params );
	
	void microcode_MOVE_from_SR( processor_state& state, uint32_t* params );
	
	void microcode_MOVE_to_CCR( processor_state& state, uint32_t* params );
	
	void microcode_MOVE_to_SR( processor_state& state, uint32_t* params );
	
	void microcode_BKPT( processor_state& state, uint32_t* params );
	
	void microcode_PEA( processor_state& state, uint32_t* params );
	
	void microcode_EXT_W( processor_state& state, uint32_t* params );
	void microcode_EXT_L( processor_state& state, uint32_t* params );
	
	void microcode_EXTB( processor_state& state, uint32_t* params );
	
	void microcode_MOVEM_to  ( processor_state& state, uint32_t* params );
	void microcode_MOVEM_from( processor_state& state, uint32_t* params );
	
	void microcode_TRAP( processor_state& state, uint32_t* params );
	
	void microcode_LINK( processor_state& state, uint32_t* params );
	
	void microcode_UNLK( processor_state& state, uint32_t* params );
	
	void microcode_MOVE_to_USP  ( processor_state& state, uint32_t* params );
	void microcode_MOVE_from_USP( processor_state& state, uint32_t* params );
	
	void microcode_NOP( processor_state& state, uint32_t* params );
	
	void microcode_STOP( processor_state& state, uint32_t* params );
	
	void microcode_RTE( processor_state& state, uint32_t* params );
	
	void microcode_RTD( processor_state& state, uint32_t* params );
	
	void microcode_RTS( processor_state& state, uint32_t* params );
	
	void microcode_TRAPV( processor_state& state, uint32_t* params );
	
	void microcode_RTR( processor_state& state, uint32_t* params );
	
	void microcode_JSR( processor_state& state, uint32_t* params );
	
	void microcode_MOVEC( processor_state& state, uint32_t* params );
	
	void microcode_JMP( processor_state& state, uint32_t* params );
	
	#pragma mark -
	#pragma mark Line 6
	
	void microcode_BRA( processor_state& state, uint32_t* params );
	void microcode_BSR( processor_state& state, uint32_t* params );
	void microcode_Bcc( processor_state& state, uint32_t* params );
	
	#pragma mark -
	#pragma mark Line 8
	
	void microcode_OR( processor_state& state, uint32_t* params );
	
	#pragma mark -
	#pragma mark Line 9
	
	void microcode_SUB( processor_state& state, uint32_t* params );
	
	void microcode_SUBA( processor_state& state, uint32_t* params );
	
	#pragma mark -
	#pragma mark Line B
	
	void microcode_CMP( processor_state& state, uint32_t* params );
	
	void microcode_EOR( processor_state& state, uint32_t* params );
	
	#pragma mark -
	#pragma mark Line C
	
	void microcode_AND( processor_state& state, uint32_t* params );
	
	void microcode_EXG( processor_state& state, uint32_t* params );
	
	#pragma mark -
	#pragma mark Line D
	
	void microcode_ADD( processor_state& state, uint32_t* params );
	
	void microcode_ADDA( processor_state& state, uint32_t* params );
	
}

#endif

