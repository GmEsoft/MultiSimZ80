#ifndef MODEL1_MEMORY_H
#define MODEL1_MEMORY_H

// TRS-80 MODEL I MEMORY MANAGER

#include "Memory_I.h"
#include "Memory32_I.h"
#include "InOut_I.h"
#include "Latch.h"
#include "CPU.h"

class Model1Memory :
	public Memory_I
{
public:
	Model1Memory
		( Memory32_I &_rom
		, Memory32_I &_ram
		, Memory_I &_do
#if USE_TRS80_KI
		, Memory_I &_ki
#endif
		//, Memory_I &_pr
		, InOut_I &_fdc			// Floppy Disk Controller
		, Latch_I &_floppies	// Floppy Unit/Side/Density selector
		, CPU &_cpu
		)
		: rom_( _rom )
		, ram_( _ram )
		, do_( _do )
#if USE_TRS80_KI
		, ki_( _ki )
#endif
		, /*pr_( _pr )
		, */fdc_( _fdc )
		, floppies_( _floppies )
		, cpu_( _cpu )
	{
	}

	~Model1Memory(void)
	{
	}

	// write byte
	virtual uchar write( ushort addr, uchar data );

	// read byte
	virtual uchar read( ushort addr );

	Latch			model1IntLatch_;	// ..-.. - M1 Int Latch
	Latch			model1IntMask_;		// ..-.. - M1 Int Mask

private:
	Memory32_I		&rom_;
	Memory32_I		&ram_;
	Memory_I		&do_;
#if USE_TRS80_KI
	Memory_I		&ki_;
#endif
	//Memory_I		&pr_;
	InOut_I			&fdc_;		// Floppy Disk Controller
	Latch_I			&floppies_;	// Floppy Unit/Side/Density selector
	CPU				&cpu_;
};

#endif

