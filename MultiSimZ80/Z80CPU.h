#ifndef Z80CPU_H
#define Z80CPU_H

// Z80 CPU WITH KEIL + GME EXTENSIONS

#include "CPU.h"

class Z80CPU;

typedef void (Z80CPU::*SimOp)();

struct instr 
{ 
	int mnemon, opn1, opn2;
	SimOp simop;
    int arg1, arg2; 
};

union Z80REGS
{ 
	struct X 
	{ 
		unsigned short	bc ,
                        de ,
                        hl ,
                        af ,
                        sp ,
                        ix ,
                        iy ,
                        bc1,
                        de1,
                        hl1,
                        af1,
                        ir ;
    } x ;
    struct H 
	{ 
		unsigned char	c, b,
                        e, d,
                        l, h,
                        f, a,
                        spl, sph ,
                        ixl, ixh,
                        iyl, iyh,
                        c1, b1,
                        e1, d1,
                        l1, h1,
                        f1, a1,
                        i, r,
                        im;
	} h ;
};

struct Z80FLAGS {int c:1,n:1,p:1,x:1,h:1,y:1,z:1,s:1;} ;
//	F = SZYHXPNC
//		10101000 = A8

class Z80CPU :
	public CPU
{
public:
	Z80CPU()
	{
		init();
	}

	// Trigger IRQ interrupt
	virtual void trigIRQ( char irq );

	// Get IRQ status
	virtual char getIRQ( void );

	// Trigger NMI interrupt
	virtual void trigNMI( void );

	// Get NMI status
	virtual char getNMI( void );

	// Init internal pointers
	void init();

	// CPU Reset
	virtual void reset();

	// Set Break On Halt
	void setBreakOnHalt( int _breakOnHalt )
	{
		breakOnHalt = _breakOnHalt;
	}

	// immediate data addr
	uchar dataaddr()
	{
		return fetch();
	}

	// immediate bit addr
	uchar bitaddr()
	{
		return fetch();
	}

	// immediate short (rel) addr
	ushort saddr()
	{
		schar d = fetch();
		return pc_ + d;
	}

	// fetch signed offset
	schar sfetch()
	{
		return fetch();
	}

	uint laddr()
	{
		uint x = fetch ();
		return x | ( fetch () << 8 );
	}

	// Instruction Result
	uchar resINC_byte (uchar res);
	uchar resDEC_byte (uchar res);
	uchar resOR_byte (uchar res);
	uchar resAND_byte (uchar res);
	uchar resSUB_byte (schar res, schar c, schar h, schar v);
	uchar resADD_byte (schar res, schar c, schar h, schar v);
	uchar resROT_byte (uchar res, uchar c);
	uchar resROTA (uchar res, uchar c);
	uchar resLDI (uchar res, uint bc);
	uchar resCPI (uchar res, uint bc, uchar h);
	uchar resRXD_byte (uchar res);
	void incR();
	void selectHL();
	void selectIX();
	void selectIY();

	// Execute 1 Statement
	virtual void sim();

	~Z80CPU();

	// 8 BIT LOAD GROUP ///////////////////////////////////////////////////////////

	void simLD_R_R();
	void simLDS_A_R();
	void simLD_R_b();
	void simLD_R_ATRX();
	void simLD_ATRX_R();
	void simLD_ATRX_b();
	void simLD_R_ATw();
	void simLD_ATw_R();

	// 16 BIT LOAD GROUP //////////////////////////////////////////////////////////

	void simLD_RX_w();
	void simLD_RX_ATw();
	void simLD_ATw_RX();
	void simLD_RX_RX();
	void simPUSH_RX();
	void simPOP_RX();

	// EXCHANGE, BLOCK XFER AND SEARCH GROUP //////////////////////////////////////

	void simEX_RX_RX();
	void simEX_AF_AFP();
	void simEXX();
	void simEX_ATRX_RX();
	void simLDI();
	void simLDIR();
	void simLDD();
	void simLDDR();
	void simCPI();
	void simCPIR();
	void simCPD();
	void simCPDR();

	// 8 BIT ARITH AND LOGICAL GROUP //////////////////////////////////////////////

	void simADD_R_R();
	void simADD_R_b();
	void simADD_R_ATRX();
	void simADC_R_R();
	void simADC_R_b();
	void simADC_R_ATRX();
	void simSUB_R();
	void simSUB_b();
	void simSUB_ATRX();
	void simSBC_R_R();
	void simSBC_R_b();
	void simSBC_R_ATRX();
	void simAND_R();
	void simAND_b();
	void simAND_ATRX();
	void simOR_R();
	void simOR_b();
	void simOR_ATRX();
	void simXOR_R();
	void simXOR_b();
	void simXOR_ATRX();
	void simCP_R();
	void simCP_b();
	void simCP_ATRX();
	void simINC_R();
	void simINC_ATRX();
	void simDEC_R();
	void simDEC_ATRX();

	// GENERAL PURPOSE ARITHMETIC AND CPU CONTROL GROUP ///////////////////////////

	void simDAA();
	void simCPL();
	void simNEG();
	void simCCF();
	void simSCF();
	void simNOP();
	void simHALT();
	void simDI();
	void simEI();
	void simIM();

	// 16 BIT ARITHMETIC GROUP ////////////////////////////////////////////////////

	void simADD_RX_RX();
	void simADC_RX_RX();
	void simSBC_RX_RX();
	void simINC_RX();
	void simDEC_RX();

	// ROTATE AND SHIFT GROUP /////////////////////////////////////////////////////

	void simRLCA();
	void simRLA();
	void simRRCA();
	void simRRA();
	void simRLC_R();
	void simRLC_ATRX();
	void simRL_R();
	void simRL_ATRX();
	void simRRC_R();
	void simRRC_ATRX();
	void simRR_R();
	void simRR_ATRX();
	void simSLA_R();
	void simSLA_ATRX();
	void simSRA_R();
	void simSRA_ATRX();
	void simSLL_R();
	void simSLL_ATRX();
	void simSRL_R();
	void simSRL_ATRX();
	void simRLD();
	void simRRD();

	// BIT SET RESET AND TEST GROUP ///////////////////////////////////////////////

	void simBIT_n_R();
	void simBIT_n_ATRX();
	void simSET_n_R();
	void simSET_n_ATRX();
	void simRES_n_R();
	void simRES_n_ATRX();

	// JUMP GROUP /////////////////////////////////////////////////////////////////

	void simJP();
	void simJP_Z();
	void simJP_NZ();
	void simJP_C();
	void simJP_NC();
	void simJP_PE();
	void simJP_PO();
	void simJP_P();
	void simJP_M();
	void simJR();
	void simJR_Z();
	void simJR_NZ();
	void simJR_C();
	void simJR_NC();
	void simJP_ATRX();
	void simDJNZ();

	// CALL AND RETURN GROUP //////////////////////////////////////////////////////

	void simCALL();
	void simCALL_Z();
	void simCALL_NZ();
	void simCALL_C();
	void simCALL_NC();
	void simCALL_PE();
	void simCALL_PO();
	void simCALL_P();
	void simCALL_M();
	void simRET();
	void simRET_Z();
	void simRET_NZ();
	void simRET_C();
	void simRET_NC();
	void simRET_PE();
	void simRET_PO();
	void simRET_P();
	void simRET_M();
	void simRETI();
	void simRETN();
	void simRST();

	// INPUT AND OUTPUT GROUP /////////////////////////////////////////////////////

	void simIN_R_ATb();
	void simIN_R_ATR();
	void simINI();
	void simINIR();
	void simIND();
	void simOUT_ATb_R();
	void simOUT_ATR_R();
	void simOUTI();
	void simOTIR();
	void simOUTD();

	// MULTI-BYTE OPCODES /////////////////////////////////////////////////////////

	void simCB();
	void simDD();
	void simED();
	void simFD();

	// EXTENDED SIMULATOR INSTRUCTIONS - DAVID KEIL ///////////////////////////////

	// David Keil's extended instructions

	void simDIV16();
	void simDIV32();
	void simIDIV32();
	void simSSPD();
	void simGSPD();
	void simOPEN();
	void simCLOSE();
	void simREAD();
	void simWRITE();
	void simSEEK();
	void simERROR();
	void simFINDFIRST();
	void simFINDNEXT();
	void simLOAD();
	void simCHDIR();
	void simGETDIR();
	void simPATHOPEN();
	void simCLOSEALL();

	// EXTENDED SIMULATOR INSTRUCTIONS - SIMZ8032 /////////////////////////////////

	void simBREAK();
	void simEXIT();
	void simCSINB();
	void simCSOUTC();
	void simCSEND();
	void simKBINC();
	void simKBWAIT();
	void simPUTCH();
	void simEIBRKON();
	void simEIBRKOFF();
private:
	long			tstates;
	ushort			opcode;
	uchar			iff1, iff2, irq, nmi;
	union Z80REGS	Z80 ;
	ushort			*Z80x[16];
	uchar			*Z80h[32];

	//  bit-mapping of PSW register
	struct Z80FLAGS *flags;

	uchar			useix, useiy;
	schar			offset;
	int				breakOnEI;
	int				breakOnHalt;
	int				Z80SimSpeedFlags;
	char			Z80KeyPressed;
public:
	static struct instr	instr[];
};

#endif
