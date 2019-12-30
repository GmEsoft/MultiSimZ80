#include "Z80CPU.h"
#include "FileTools.h"

#include <assert.h>
#include <cstdlib>
#include <string.h>

#define simStop(msg)
#define trace(msg)
#define setSimSpeed() // temp !!
#define Z80Wait() // temp !!
#define perror( error ) // temp !!
#define DO_updateDisplay() // temp !!

#ifndef O_BINARY
#define O_BINARY 0
#endif

File_I *readfile( Console_I* console, FileSystem_I *fileSystem, const char *prompt, const char *mode, const char *ext )
{
    char	buffer[43];
    int     x,y;
    File_I    *file = NULL;

	console->puts( prompt );
    buffer[0] = 40;
	console->gets( buffer );
	FileTools::addDefaultExt( buffer+2, ext );
    //console->puts( buffer+2 );

    if ( buffer[1] > 0 )
    {
		file = fileSystem ? fileSystem->open( buffer+2, mode ) : 0;
    }

    if ( !file )
    {
        console->puts( buffer+2 );
        console->puts( ": " );
        if ( !fileSystem )
            console->puts( "no fileSystem" );
        else
            console->puts( fileSystem->strerror( fileSystem->errno_() ) );
    }

    return file;
}


#define USE_DOS 1
#define USE_DIR 0
#define USE_HARD 0
#define USE_LOAD 0
#define MAXPATH 260

//  Enumerated constants for instructions, also array subscripts
enum {  NOP=0, LD, INC, DEC, ADD, SUB, ADC, SBC, AND, OR, XOR, RLCA,
        RRCA, RLA, RRA, EX, EXX, DJNZ, JR, JP, CALL, RET, RST, CPL, NEG, SCF, CCF,
        CP, IN, OUT, PUSH, POP, HALT, DI, EI, DAA, RLD, RRD,
        RLC, RRC, RL, RR, SLA, SRA, SLL, SRL, BIT, RES, SET,
        LDI, LDD, LDIR, LDDR, CPI, CPIR, CPD, CPDR,
        INI, INIR, IND, INDR, OUTI, OTIR, OUTD, OTDR, IM, RETI, RETN,
        BREAK, EXIT, CSINB, CSOUTC, CSEND, KBINC, KBWAIT, PUTCH, EIBRKON, EIBRKOFF,
        DIV16, DIV32, IDIV32,
        OPEN, CLOSE, READ, WRITE, SEEK,
        GSPD, SSPD, ERROR, FINDFIRST, FINDNEXT, LOAD,
        CHDIR, GETDIR, PATHOPEN, CLOSEALL,
        DEFB};

//  Enumerated constants for operands, also array subscripts
enum {R=1, RX, BYTE, WORD, OFFSET, ATR, ATRX, AFP,
      Z, C, NZ, NC, PE, PO, P, M, ATBYTE, ATWORD, DIRECT};


enum {simA=1, simB, simC, simD, simE, simH, simL, simI, simR, simBC, simDE, simHL, simAF, simSP, simIR};



Z80CPU::~Z80CPU()
{
}

// Get nth opcode (1st or 2nd)
__inline int getopn(int opcode, int pos) 
{ 
	return pos==1 ? Z80CPU::instr[opcode].opn1 : Z80CPU::instr[opcode].opn2 ; 
}

// Get nth argument (1st or 2nd)
__inline int getarg(int opcode, int pos) 
{ 
	return pos==1? Z80CPU::instr[opcode].arg1 : Z80CPU::instr[opcode].arg2 ; 
}

// Init internal pointers
void Z80CPU::init()
{
	flags = (struct Z80FLAGS*)&Z80.h.f;
	Z80h[simA] = &Z80.h.a;
	Z80h[simB] = &Z80.h.b;
	Z80h[simC] = &Z80.h.c;
	Z80h[simD] = &Z80.h.d;
	Z80h[simE] = &Z80.h.e;
	Z80h[simH] = &Z80.h.h;
	Z80h[simL] = &Z80.h.l;
	Z80h[simI] = &Z80.h.i;
	Z80h[simR] = &Z80.h.r;
	Z80x[simAF] = &Z80.x.af;
	Z80x[simBC] = &Z80.x.bc;
	Z80x[simDE] = &Z80.x.de;
	Z80x[simHL] = &Z80.x.hl;
	Z80x[simSP] = &Z80.x.sp;
	Z80x[simIR] = &Z80.x.ir;
	breakOnEI = 0;
	breakOnHalt = 1;
	Z80SimSpeedFlags = 0;
	Z80KeyPressed = 0;
}


// CPU Reset
void Z80CPU::reset()
{
	pc_ = 0;
	Z80.x.sp = 0xFFFF;
	useix = useiy = 0;
	iff1 = iff2 = Z80.h.im = 0;
	irq = nmi = 0;
	Z80.x.af = Z80.x.af1 = Z80.x.bc = Z80.x.bc1 = Z80.x.de = Z80.x.de1 = Z80.x.hl = Z80.x.hl1 = Z80.x.ir = Z80.x.ix = Z80.x.iy = 0xFFFF;
	breakOnEI = 0;
	tstates = 0;
}

// Trigger IRQ interrupt
void Z80CPU::trigIRQ( char myirq )
{
	irq = myirq;
}

// Get IRQ status
char Z80CPU::getIRQ( void )
{
	return irq;
}

// Trigger NMI interrupt
void Z80CPU::trigNMI( void )
{
	nmi = 3;
}

// Get NMI status
char Z80CPU::getNMI( void )
{
	return nmi;
}

// Get Parity of ACC (1=even, 0=odd)
uchar parity(uchar res)
{
	res ^= res >> 4;
	res ^= res >> 2;
	res ^= res >> 1;
	return (~res) & 1;
}

// Instruction Results
__inline uchar Z80CPU::resINC_byte (uchar res)
{
    flags->p = res==0x80 ? 1 : 0;
    flags->n = 0;
    flags->z = res==0 ? 1 : 0;
    flags->h = (res&0x0F)==0 ? 1 : 0;
    Z80.h.f &= ~0xA8;
    Z80.h.f |= res & 0xA8;
    return res;
}

uchar Z80CPU::resDEC_byte (uchar res)
{
    flags->p = res==0x7F ? 1 : 0;
    flags->n = 1;
    flags->z = res==0 ? 1 : 0;
    flags->h = (res&0x0F)==0x0F ? 1 : 0;
    Z80.h.f &= ~0xA8;
    Z80.h.f |= res & 0xA8;
    return res;
}

uchar Z80CPU::resOR_byte (uchar res)
{
    flags->c = flags->n = 0;
    flags->p = parity(res);
    flags->z = res==0 ? 1 : 0;
    flags->h = 0;
    Z80.h.f &= ~0xA8;
    Z80.h.f |= res & 0xA8;
    return res;
}

uchar Z80CPU::resAND_byte (uchar res)
{
    flags->c = flags->n = 0;
    flags->p = parity(res);
    flags->z = res==0 ? 1 : 0;
    flags->h = 1;
    Z80.h.f &= ~0xA8;
    Z80.h.f |= res & 0xA8;
    return res;
}

/*				-2		-1		0		+1
		-		110		111		000		001
-2		110		000		001		010(CV)	111(CV)
-1		111		11(C)	00		01(C)	10(CV)
0		000		10		11		00		01
1		001		01(V)	10		11(C)	00
*/
uchar Z80CPU::resSUB_byte (schar res, schar c, schar h, schar v)
{
    flags->c = c ? 1 : 0;
    flags->p = v ? 1 : 0;
    flags->h = h ? 1 : 0;
    flags->z = res==0 ? 1 : 0;
    flags->n = 1;
    Z80.h.f &= ~0xA8;
    Z80.h.f |= res & 0xA8;
    return res;
}

/*				-2		-1		0		+1
		+		10		11		00		01
-2		10		00(CV)	01(CV)	10		11
-1		11		01(CV)	10(CV)	11		00(C)
0		00		10		11		00		01
1		01		11		00(C)	01		10
*/
uchar Z80CPU::resADD_byte (schar res, schar c, schar h, schar v)
{
    flags->c = c ? 1 : 0;
    flags->p = v ? 1 : 0; //??
    flags->h = h ? 1 : 0;
    flags->z = res==0 ? 1 : 0;
    flags->n = 0;
    Z80.h.f &= ~0xA8;
    Z80.h.f |= res & 0xA8;
    return res;
}

uchar Z80CPU::resROT_byte (uchar res, uchar c)
{
    Z80.h.f = res;
    flags->c = c;
    flags->p = parity(res);
    flags->z = res==0 ? 1 : 0;
    flags->n = 0;
    flags->h = 0;
    return res;
}

uchar Z80CPU::resROTA (uchar res, uchar c)
{
    Z80.h.f = (Z80.h.f & 0xC4) | (res & 0x28);
    flags->c = c;
    //flags->p = parity(res);
    flags->n = 0;
    flags->h = 0;
    return res;
}

uchar Z80CPU::resLDI (uchar res, uint bc)
{
	ushort x = res + Z80.h.a;
	flags->y = ( x >> 1 ) & 1;
	flags->x = ( x >> 3 ) & 1;
    flags->p = bc==0 ? 0 : 1;
    flags->n = 0;
    flags->h = 0;
    return res;
}

uchar Z80CPU::resCPI (uchar res, uint bc, uchar h)
{
	ushort x = res - h;
	flags->y = ( x >> 1 ) & 1;
	flags->x = ( x >> 3 ) & 1;
	flags->s = ( res >> 7 ) & 1;
    flags->z = res==0 ? 1 : 0;
    flags->p = bc==0 ? 0 : 1;
    flags->n = 1;
    flags->h = h;
    return res;
}

uchar Z80CPU::resRXD_byte (uchar res)
{
    flags->p = parity(res);
    flags->z = res==0 ? 1 : 0;
    flags->n = 0;
    flags->h = 0;
    Z80.h.f &= ~0xA8;
    Z80.h.f |= res & 0xA8;
    return res;
}

void Z80CPU::incR()
{
	ushort x = Z80.h.r & 0x80;
	++Z80.h.r;
	Z80.h.r &= 0x7F;
	Z80.h.r |= x;
}

// Init internal pointers for HL
void Z80CPU::selectHL()
{
	Z80h[simH] = &Z80.h.h;
	Z80h[simL] = &Z80.h.l;
	Z80x[simHL] = &Z80.x.hl;
}

// Init internal pointers for IX
void Z80CPU::selectIX()
{
	Z80h[simH] = &Z80.h.ixh;
	Z80h[simL] = &Z80.h.ixl;
	Z80x[simHL] = &Z80.x.ix;
}

// Init internal pointers for IY
void Z80CPU::selectIY()
{
	Z80h[simH] = &Z80.h.iyh;
	Z80h[simL] = &Z80.h.iyl;
	Z80x[simHL] = &Z80.x.iy;
}



// Execute 1 Statement
void Z80CPU::sim()
{
	// interrupts
	if ( nmi )
	{
		// Non-maskable interrupt
		if ( --nmi == 0 )
		{
			iff2 = iff1;
			iff1 = 0;
			putdata( --Z80.x.sp, pc_ >> 8 );
			putdata( --Z80.x.sp, pc_ );
			pc_ = 0x0066;
		}
	}
	else if ( iff1 && iff2 )
	{
		// accept interrupt
		if ( irq )
		{
			iff1 = iff2 = 0;
			switch (Z80.h.im)
			{
				case 0:
					simStop("IRQ in mode 0");
					break;
				case 1:
					putdata( --Z80.x.sp, pc_ >> 8 );
					putdata( --Z80.x.sp, pc_ );
					pc_ = 0x0038;
#ifdef DBG_INT
					simStop("IRQ in mode 1");
#endif
					break;
				case 2:
					simStop("IRQ in mode 2");
					break;
				default:
					simStop("IRQ in unknown mode");
					break;
			}
			return;
		}
	}
	else
	{
		iff2 |= iff1;
	}

	useix = useiy = 0;

	incR();
	opcode = fetch();
	(this->*instr[opcode].simop)();
	selectHL();
	runcycles( tstates );
	tstates = 0;
}

// 8 BIT LOAD GROUP ///////////////////////////////////////////////////////////

// LD r,r'
void Z80CPU::simLD_R_R ()
{
    assert(Z80h[instr[opcode].arg1]);
    assert(Z80h[instr[opcode].arg2]);
	ushort x;
	if (useix || useiy )
	{
		if ( instr[opcode].arg2 == simH )
			x = useix ? Z80.h.ixh : Z80.h.iyh;
		else if ( instr[opcode].arg2 == simL )
			x = useix ? Z80.h.ixl : Z80.h.iyl;
		else
			x = *Z80h[instr[opcode].arg2];

		if ( instr[opcode].arg1 == simH )
			*( useix ? &Z80.h.ixh : &Z80.h.iyh ) = x;
		else if ( instr[opcode].arg1 == simL )
			*( useix ? &Z80.h.ixl : &Z80.h.iyl ) = x;
		else
			*Z80h[instr[opcode].arg1] = x;
		// TODO: timing for ix/iy
	}
	else
	{
		*Z80h[instr[opcode].arg1] = *Z80h[instr[opcode].arg2];
	}
    tstates += 4;
}

// LD A,I & LD A,R
void Z80CPU::simLDS_A_R ()
{
    assert( instr[opcode].arg1 == simA );
    assert(Z80h[instr[opcode].arg2]);
	*Z80h[instr[opcode].arg1] = *Z80h[instr[opcode].arg2];
	flags->h = flags->n = 0;
	flags->p = iff2;
	flags->z = ( Z80.h.a ) ? 0 : 1;
	Z80.h.f &= ~0xA8;
	Z80.h.f |= Z80.h.a & 0xA8;

	tstates += 9;
}

// LD r,n
void Z80CPU::simLD_R_b ()
{
    assert(Z80h[instr[opcode].arg1]);
	ushort x = fetch();
	if (useix || useiy )
	{
		if ( instr[opcode].arg1 == simH )
			*( useix ? &Z80.h.ixh : &Z80.h.iyh ) = x;
		else if ( instr[opcode].arg1 == simL )
			*( useix ? &Z80.h.ixl : &Z80.h.iyl ) = x;
		else
			*Z80h[instr[opcode].arg1] = x;
		// TODO: timing for ix/iy
	}
	else
	{
		*Z80h[instr[opcode].arg1] = x;
	}
	tstates += 7;
    // TODO: ixh/ixl/iyh/iyl
}

// LD r,(HL)
// LD r,(IX+d)
// LD r,(IY+d)
// LD A,(BC)
// LD A,(DE)
void Z80CPU::simLD_R_ATRX ()
{
    assert(Z80h[instr[opcode].arg1]);
    assert(Z80x[instr[opcode].arg2]);
	ushort y = *Z80x[instr[opcode].arg2];
	if ( useix || useiy )
	{
		y += sfetch();
		selectHL();
		tstates += 19;
	}
	*Z80h[instr[opcode].arg1] = getdata( y );
}

// LD (HL),r
// LD (IX+d),r
// LD (IY+d),r
// LD (BC),A
// LD (DE),A
void Z80CPU::simLD_ATRX_R ()
{
    assert(Z80x[instr[opcode].arg1]);
    assert(Z80h[instr[opcode].arg2]);
	ushort y = *Z80x[instr[opcode].arg1];
	if ( useix || useiy )
	{
		y += sfetch();
		selectHL();
		tstates += 19;
	}
	putdata( y, *Z80h[instr[opcode].arg2] );
}

// LD (HL),n
// LD (IX+d),n
// LD (IY+d),n
void Z80CPU::simLD_ATRX_b ()
{
    assert(Z80x[instr[opcode].arg1]);
    if (useix && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        putdata(Z80.x.ix+offset,fetch());
        tstates += 19;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        putdata(Z80.x.iy+offset,fetch());
        tstates += 19;
    } else {
        putdata(*Z80x[instr[opcode].arg1], fetch());
        tstates += 10;
    }
}

// LD A,(nn)
void Z80CPU::simLD_R_ATw ()
{
    assert(Z80h[instr[opcode].arg1]);
    *Z80h[instr[opcode].arg1] = getdata(laddr()) ;
    tstates += 13;
}

// LD (nn),A
void Z80CPU::simLD_ATw_R ()
{
    assert(Z80h[instr[opcode].arg2]);
    putdata (laddr() , *Z80h[instr[opcode].arg2] ) ;
    tstates += 13;
}

// LD A,I

// LD A,R

// LD I,A

// LD R,A

// 16 BIT LOAD GROUP //////////////////////////////////////////////////////////

// LD dd,nn 	dd=BC,DE,HL,SP
// LD IX,nn
// LD IY,nn
void Z80CPU::simLD_RX_w ()
{
    assert(Z80x[instr[opcode].arg1] != 0);
    if (useix && instr[opcode].arg1 == simHL) {
        Z80.x.ix = laddr() ;
    	tstates += 14;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        Z80.x.iy = laddr() ;
    	tstates += 14;
    } else {
        *Z80x[instr[opcode].arg1] = laddr() ;
    	tstates += 10;
    }
}

// LD HL,(nn)
// LD dd,(nn)	dd=BC,DE,HL,SP
// LD IX,(nn)
// LD IY,(nn)
void Z80CPU::simLD_RX_ATw ()
{
    ushort i = laddr() ;
    ushort x = getdata(i)|(getdata(i+1)<<8);
    assert(Z80x[instr[opcode].arg1]);
    if (useix && instr[opcode].arg1 == simHL) {
        Z80.x.ix = x ;
    	tstates += 20;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        Z80.x.iy = x ;
    	tstates += 20;
    } else {
        *Z80x[instr[opcode].arg1] = x ;
    	tstates += 16; // 20 if BC/DE/SP
    }
}

// LD (nn),HL
// LD (nn),dd	dd=BC,DE,HL,SP
// LD (nn),IX
// LD (nn),IY
void Z80CPU::simLD_ATw_RX ()
{
    ushort i = laddr() ;
	ushort x;
    assert(Z80x[instr[opcode].arg2]);
    if (useix && instr[opcode].arg2 == simHL) {
        x = Z80.x.ix ;
    	tstates += 20;
    } else if (useiy && instr[opcode].arg2 == simHL) {
        x = Z80.x.iy ;
    	tstates += 20;
    } else {
        x = *Z80x[instr[opcode].arg2] ;
    	tstates += 16; // 20 if BC/DE/SP
    }
    putdata(i,x);
    putdata(i+1,x>>8);
}

// LD SP,HL
// LD SP,IX
// LD SP,IY
void Z80CPU::simLD_RX_RX ()
{
    assert(Z80x[instr[opcode].arg1]);
    assert(Z80x[instr[opcode].arg2]);
    assert(instr[opcode].arg1==simSP);
    assert(instr[opcode].arg2==simHL);
    if (useix && instr[opcode].arg2 == simHL) {
        *Z80x[instr[opcode].arg1] = Z80.x.ix ;
    	tstates += 10;
    } else if (useiy && instr[opcode].arg2 == simHL) {
        *Z80x[instr[opcode].arg1] = Z80.x.iy;
    	tstates += 10;
    } else {
        *Z80x[instr[opcode].arg1] = *Z80x[instr[opcode].arg2];
    	tstates += 6;
    }
}

// PUSH qq		qq=BC,DE,HL,AF
// PUSH IX
// PUSH IY
void Z80CPU::simPUSH_RX()
{
    assert(Z80x[instr[opcode].arg1]);
	ushort x;
    if (useix && instr[opcode].arg1 == simHL)
    {
        x = Z80.x.ix ;
    	tstates += 15;
	}
    else if (useiy && instr[opcode].arg1 == simHL)
    {
        x = Z80.x.iy ;
    	tstates += 15;
	}
    else
    {
        x = *Z80x[instr[opcode].arg1] ;
    	tstates += 11;
	}
    putdata(--Z80.x.sp, x >> 8 );
    putdata(--Z80.x.sp, x );
}

// POP qq		qq=BC,DE,HL,AF
// POP IX
// POP IY
void Z80CPU::simPOP_RX()
{
    assert(Z80x[instr[opcode].arg1]);
    ushort x = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
    if (useix && instr[opcode].arg1 == simHL)
    {
        Z80.x.ix = x;
    	tstates += 14;
	}
    else if (useiy && instr[opcode].arg1 == simHL)
    {
        Z80.x.iy = x;
    	tstates += 14;
	}
    else
    {
        *Z80x[instr[opcode].arg1] = x;
    	tstates += 10;
	}
}

// EXCHANGE, BLOCK XFER AND SEARCH GROUP //////////////////////////////////////

// EX DE,HL
// EX DE,IX
// EX DE,IY
void Z80CPU::simEX_RX_RX ()
{
    assert(instr[opcode].arg1 == simDE);
    assert(instr[opcode].arg2 == simHL);
    assert(Z80x[instr[opcode].arg1] );
    assert(Z80x[instr[opcode].arg2] );
	ushort y;
    if (useix) {
        y = Z80.x.de;
        Z80.x.de = Z80.x.ix;
        Z80.x.ix = y;
    	tstates += 10;
    } else if (useiy) {
        y = Z80.x.de;
        Z80.x.de = Z80.x.iy;
        Z80.x.iy = y;
    	tstates += 10;
    } else {
        y = Z80.x.de;
        Z80.x.de = Z80.x.hl;
        Z80.x.hl = y;
    	tstates += 4;
    }
}

// EX AF,AF'
void Z80CPU::simEX_AF_AFP ()
{
    ushort x = Z80.x.af;
    Z80.x.af = Z80.x.af1;
    Z80.x.af1 = x;
    tstates += 4;
}

// EXX
void Z80CPU::simEXX ()
{
    ushort x = Z80.x.hl;
    Z80.x.hl = Z80.x.hl1;
    Z80.x.hl1 = x;
    x = Z80.x.de;
    Z80.x.de = Z80.x.de1;
    Z80.x.de1 = x;
    x = Z80.x.bc;
    Z80.x.bc = Z80.x.bc1;
    Z80.x.bc1 = x;
    tstates += 4;
}

// EX (SP),HL
// EX (SP),IX
// EX (SP),IY
void Z80CPU::simEX_ATRX_RX ()
{
    assert(instr[opcode].arg1 == simSP);
    assert(instr[opcode].arg2 == simHL);
    assert(Z80x[instr[opcode].arg1] );
    assert(Z80x[instr[opcode].arg2] );
	ushort y;
    if (useix) {
        y = getdata(Z80.x.sp);
        putdata(Z80.x.sp, Z80.h.ixl);
        Z80.h.ixl = y;
        y = getdata(Z80.x.sp+1);
        putdata(Z80.x.sp+1, Z80.h.ixh);
        Z80.h.ixh = y;
        tstates += 23;
    } else if (useiy) {
        y = getdata(Z80.x.sp);
        putdata(Z80.x.sp, Z80.h.iyl);
        Z80.h.iyl = y;
        y = getdata(Z80.x.sp+1);
        putdata(Z80.x.sp+1, Z80.h.iyh);
        Z80.h.iyh = y;
        tstates += 23;
    } else {
        y = getdata(Z80.x.sp);
        putdata(Z80.x.sp, Z80.h.l);
        Z80.h.l = y;
        y = getdata(Z80.x.sp+1);
        putdata(Z80.x.sp+1, Z80.h.h);
        Z80.h.h = y;
        tstates += 19;
    }
}

// LDI
void Z80CPU::simLDI()
{
    --Z80.x.bc;
    resLDI( putdata(Z80.x.de++, getdata(Z80.x.hl++)), Z80.x.bc );
    tstates += 16;
}

// LDIR
void Z80CPU::simLDIR()
{
    if (--Z80.x.bc)
    {
        pc_ -= 2;
        tstates += 5;
	}
    resLDI( putdata(Z80.x.de++, getdata(Z80.x.hl++)), Z80.x.bc );
    tstates += 16;
}

// LDD
void Z80CPU::simLDD()
{
    --Z80.x.bc;
    resLDI( putdata(Z80.x.de--, getdata(Z80.x.hl--)), Z80.x.bc );
    tstates += 16;
}

// LDDR
void Z80CPU::simLDDR()
{
    if (--Z80.x.bc)
    {
        pc_ -= 2;
        tstates += 5;
	}
    resLDI( putdata(Z80.x.de--, getdata(Z80.x.hl--)), Z80.x.bc );
    tstates += 16;
}

// CPI
void Z80CPU::simCPI()
{
    ushort x = getdata(Z80.x.hl++) ;
    ushort y = Z80.h.a - x;
    
	uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;

    --Z80.x.bc;
    resCPI( y, Z80.x.bc, h );
    tstates += 16;
}

// CPIR
void Z80CPU::simCPIR()
{	
	ushort x = getdata(Z80.x.hl++) ;
    ushort y = Z80.h.a - x;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;

    if (--Z80.x.bc && y)
    {
    	pc_ -= 2;
    	tstates += 3;
	}
    resCPI( y, Z80.x.bc, h );
    tstates += 18;
}


// CPD
void Z80CPU::simCPD()
{
    ushort x = getdata(Z80.x.hl--) ;
    ushort y = Z80.h.a - x;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;

    --Z80.x.bc;
    resCPI( y, Z80.x.bc, h );
    tstates += 16;
}

// CPDR
void Z80CPU::simCPDR()
{
    ushort x = getdata(Z80.x.hl--) ;
    ushort y = Z80.h.a - x;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;

    if (--Z80.x.bc && y)
    {
    	pc_ -= 2;
    	tstates += 3;
	}
    resCPI( y, Z80.x.bc, h );
    tstates += 18;
}

// 8 BIT ARITH AND LOGICAL GROUP //////////////////////////////////////////////

// Note: flags->c = 0 or -1

// Set Overflow flag resulting from ADD/ADC and SUB/SBC/CP
#define ovadd(y,x,c) ( (signed int)( (signed char)x + (signed char)y - (signed char)c ) != (signed char)( (signed char)x + (signed char)y - (signed char)c ) )
#define ovsub(y,x,c) ( (signed int)( (signed char)x - (signed char)y + (signed char)c ) != (signed char)( (signed char)x - (signed char)y + (signed char)c ) )

// ADD A,r
void Z80CPU::simADD_R_R()
{
	// todo: IXH & IXL
    assert(Z80h[instr[opcode].arg1]);
    assert(Z80h[instr[opcode].arg2]);
    ushort x = *Z80h[instr[opcode].arg2];
	ushort z;
    ushort y = ( z = Z80.h.a ) + x;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resADD_byte(Z80.h.a = y, c, h, ovadd(x,z,0));
    tstates += 4;
    // TODO: IXh/IXl
}

// ADD A,n
void Z80CPU::simADD_R_b()
{
	// todo: IXH & IXL
    assert(Z80h[instr[opcode].arg1]);
    ushort x = fetch();
    ushort z;
    ushort y = ( z = Z80.h.a ) + x;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resADD_byte(Z80.h.a = y, c, h, ovadd(x,z,0));
    tstates += 7;
}

// ADD A,(HL)
// ADD A,(IX+d)
// ADD A,(IY+d)
void Z80CPU::simADD_R_ATRX()
{
    assert(Z80h[instr[opcode].arg1]);
    assert(Z80x[instr[opcode].arg2]);
    ushort x;
	if (useix && instr[opcode].arg2 == simHL) {
        offset = sfetch();
        x = getdata(Z80.x.ix+offset) ;
        tstates += 19;
    } else if (useiy && instr[opcode].arg2 == simHL) {
        offset = sfetch();
        x = getdata(Z80.x.iy+offset) ;
        tstates += 19;
    } else {
        x = getdata(*Z80x[instr[opcode].arg2]);
        tstates += 7;
    }
    ushort z;
    ushort y = ( z = Z80.h.a ) + x;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resADD_byte(Z80.h.a = y, c, h, ovadd(x,z,0));
}

// ADC A,r
void Z80CPU::simADC_R_R()
{
	// todo: IXH & IXL
    assert(Z80h[instr[opcode].arg1]);
    assert(Z80h[instr[opcode].arg2]);
    ushort x = *Z80h[instr[opcode].arg2] ;
    ushort z;
    ushort y = x + ( z = Z80.h.a ) - flags->c;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resADD_byte(Z80.h.a = y, c, h, ovadd(x,z,flags->c));
    tstates += 4; // TODO: ixyhl
}

// ADC A,n
void Z80CPU::simADC_R_b()
{
	// todo: IXH & IXL
    assert(Z80h[instr[opcode].arg1]);
    ushort x = fetch();
    ushort z;
    ushort y = x + ( z = Z80.h.a ) - flags->c;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resADD_byte(Z80.h.a = y, c, h, ovadd(x,z,flags->c));
    tstates += 7;
}

// ADC A,(HL)
// ADC A,(IX+d)
// ADC A,(IY+d)
void Z80CPU::simADC_R_ATRX()
{
    assert(Z80h[instr[opcode].arg1]);
    assert(Z80x[instr[opcode].arg2]);
    ushort x;
	if (useix && instr[opcode].arg2 == simHL) {
        offset = sfetch();
        x = getdata(Z80.x.ix+offset) ;
        tstates += 19;
    } else if (useiy && instr[opcode].arg2 == simHL) {
        offset = sfetch();
        x = getdata(Z80.x.iy+offset) ;
        tstates += 19;
    } else {
        x = getdata(*Z80x[instr[opcode].arg2]);
        tstates += 7;
    }
    ushort z;
    ushort y = x + ( z = Z80.h.a ) - flags->c;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resADD_byte(Z80.h.a = y, c, h, ovadd(x,z,flags->c));
}

// SUB r
void Z80CPU::simSUB_R()
{
	// todo: IXH & IXL
    assert(Z80h[instr[opcode].arg1]);
    ushort x = *Z80h[instr[opcode].arg1] ;
    ushort z;
    ushort y = ( z = Z80.h.a ) - x;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resSUB_byte(Z80.h.a = y, c, h, ovsub(x,z,0));
    tstates += 4;
}

// SUB n
void Z80CPU::simSUB_b()
{
    ushort x = fetch() ;
    ushort z;
    ushort y = ( z = Z80.h.a ) - x;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resSUB_byte(Z80.h.a = y, c, h, ovsub(x,z,0));
    tstates += 7;
}

// SUB (HL)
// SUB (IX+d)
// SUB (IY+d)
void Z80CPU::simSUB_ATRX()
{
    assert(Z80x[instr[opcode].arg1]);
    ushort x;
	if (useix && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        x = getdata(Z80.x.ix+offset) ;
        tstates += 19;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        x = getdata(Z80.x.iy+offset) ;
        tstates += 19;
    } else {
        x = getdata(*Z80x[instr[opcode].arg1]);
        tstates += 7;
    }
    ushort z;
    ushort y = ( z = Z80.h.a ) - x;
    uchar c = y>>8;
    uchar h = (((y ^ Z80.h.a ^ x)>>4)) & 1;
    resSUB_byte(Z80.h.a = y, c, h, ovsub(x,z,0));
}

// SBC A,r
void Z80CPU::simSBC_R_R()
{
    assert(Z80h[instr[opcode].arg1]);
    assert(Z80h[instr[opcode].arg2]);
    ushort x = *Z80h[instr[opcode].arg2];
    ushort z;
    ushort y = ( z = Z80.h.a ) - x + flags->c;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resSUB_byte(Z80.h.a = y, c, h, ovsub(x,z,flags->c));
    tstates += 4;
}

// SBC A,n
void Z80CPU::simSBC_R_b()
{
    ushort x = fetch();
    ushort z;
    ushort y = ( z = Z80.h.a ) - x + flags->c;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resSUB_byte(Z80.h.a = y, c, h, ovsub(x,z,flags->c));
    tstates += 7;
}

// SBC A,(HL)
// SBC A,(IX+d)
// SBC A,(IY+d)
void Z80CPU::simSBC_R_ATRX()
{
    assert(Z80x[instr[opcode].arg2]);
    ushort x;
	if (useix && instr[opcode].arg2 == simHL) {
        offset = sfetch();
        x = getdata(Z80.x.ix+offset) ;
        tstates += 19;
    } else if (useiy && instr[opcode].arg2 == simHL) {
        offset = sfetch();
        x = getdata(Z80.x.iy+offset) ;
        tstates += 19;
    } else {
        x = getdata(*Z80x[instr[opcode].arg2]);
        tstates += 7;
    }
    ushort z;
    ushort y = ( z = Z80.h.a ) - x + flags->c;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resSUB_byte(Z80.h.a = y, c, h, ovsub(x,z,flags->c));
}

// AND r
void Z80CPU::simAND_R()
{
    assert(Z80h[instr[opcode].arg1]);
    resAND_byte(Z80.h.a &= *Z80h[instr[opcode].arg1]);
    tstates += 4;
}

// AND n
void Z80CPU::simAND_b()
{
    resAND_byte(Z80.h.a &= fetch());
    tstates += 7;
}

// AND (HL)
// AND (IX+d)
// AND (IY+d)
void Z80CPU::simAND_ATRX()
{
    assert(Z80x[instr[opcode].arg1]);
    if (useix && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        resAND_byte(Z80.h.a &= getdata(Z80.x.ix+offset));
        tstates += 19;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        resAND_byte(Z80.h.a &= getdata(Z80.x.iy+offset));
        tstates += 19;
    } else {
        resAND_byte(Z80.h.a &= getdata(*Z80x[instr[opcode].arg1]));
        tstates += 7;
    }
}

// OR r
void Z80CPU::simOR_R()
{
    assert(Z80h[instr[opcode].arg1]);
    resOR_byte(Z80.h.a |= *Z80h[instr[opcode].arg1]);
    tstates += 4;
}

// OR n
void Z80CPU::simOR_b()
{
    resOR_byte(Z80.h.a |= fetch());
    tstates += 7;
}

// OR (HL)
// OR (IX+d)
// OR (IY+d)
void Z80CPU::simOR_ATRX()
{
    assert(Z80x[instr[opcode].arg1]);
    if (useix && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        resOR_byte(Z80.h.a |= getdata(Z80.x.ix+offset));
        tstates += 19;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        resOR_byte(Z80.h.a |= getdata(Z80.x.iy+offset));
        tstates += 19;
    } else {
        resOR_byte(Z80.h.a |= getdata(*Z80x[instr[opcode].arg1]));
        tstates += 7;
    }
}

// XOR r
void Z80CPU::simXOR_R()
{
    assert(Z80h[instr[opcode].arg1]);
    resOR_byte(Z80.h.a ^= *Z80h[instr[opcode].arg1]);
    tstates += 4;
}

// XOR n
void Z80CPU::simXOR_b()
{
    resOR_byte(Z80.h.a ^= fetch());
    tstates += 7;
}

// XOR (HL)
// XOR (IX+d)
// XOR (IY+d)
void Z80CPU::simXOR_ATRX()
{
    assert(Z80x[instr[opcode].arg1]);
    if (useix && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        resOR_byte(Z80.h.a ^= getdata(Z80.x.ix+offset)) ;
        tstates += 19;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        resOR_byte(Z80.h.a ^= getdata(Z80.x.iy+offset)) ;
        tstates += 19;
    } else {
        resOR_byte(Z80.h.a ^= getdata(*Z80x[instr[opcode].arg1]));
        tstates += 7;
    }
}

// CP r
void Z80CPU::simCP_R()
{
    assert(Z80h[instr[opcode].arg1]);
    ushort x = *Z80h[instr[opcode].arg1] ;
    ushort z;
    ushort y = ( z = Z80.h.a ) - x;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resSUB_byte(y, c, h, ovsub(x,z,0));
	// !!! X and Y copied from n and not from A_n
	Z80.h.f &= ~0x28;
	Z80.h.f |= ( x & 0x28);
    tstates += 4;
}

// CP n
void Z80CPU::simCP_b()
{
    ushort x = fetch() ;
    ushort z;
    ushort y = ( z = Z80.h.a ) - x;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resSUB_byte(y, c, h, ovsub(x,z,0));
	// !!! X and Y copied from n and not from A_n
	Z80.h.f &= ~0x28;
	Z80.h.f |= ( x & 0x28);

    tstates += 7;
}

// CP (HL)
// CP (IX+d)
// CP (IY+d)
void Z80CPU::simCP_ATRX()
{
    assert(Z80x[instr[opcode].arg1]);
	ushort x;
	if (useix && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        x = getdata(Z80.x.ix+offset);
        tstates += 19;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        x = getdata(Z80.x.iy+offset);
        tstates += 19;
    } else {
        x = getdata(*Z80x[instr[opcode].arg1]);
        tstates += 7;
    }
    ushort z;
    ushort y = ( z = Z80.h.a ) - x;
    uchar c = y>>8;
    uchar h = ((y ^ Z80.h.a ^ x)>>4) & 1;
    resSUB_byte(y, c, h, ovsub(x,z,0));
	// !!! X and Y copied from n and not from A_n
	Z80.h.f &= ~0x28;
	Z80.h.f |= ( x & 0x28);
}

// INC r
void Z80CPU::simINC_R ()
{
    assert(Z80h[instr[opcode].arg1]);
	if ( useix || useiy )
	{
		if ( instr[opcode].arg1 == simH )
			resINC_byte( ++*( useix ? &Z80.h.ixh : &Z80.h.iyh ) );
		else if ( instr[opcode].arg1 == simL )
			resINC_byte( ++*( useix ? &Z80.h.ixl : &Z80.h.iyl ) );
		else
			resINC_byte( ++*Z80h[instr[opcode].arg1] );
		//TODO: timing
	}
	else
	{
		resINC_byte(++*Z80h[instr[opcode].arg1]);
	}
    tstates += 4;
}

// INC (HL)
// INC (IX+d)
// INC (IY+d)
void Z80CPU::simINC_ATRX ()
{
    assert(Z80x[instr[opcode].arg1]);
    if (useix && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        resINC_byte( putdata( Z80.x.ix+offset, getdata( Z80.x.ix+offset ) + 1 ) );
        tstates += 26;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        resINC_byte( putdata( Z80.x.iy+offset, getdata( Z80.x.iy+offset ) + 1 ) );
        tstates += 26;
    } else {
        resINC_byte( putdata( *Z80x[instr[opcode].arg1], getdata( *Z80x[instr[opcode].arg1] ) + 1 ) );
        tstates += 11;
    }
}

// DEC r
void Z80CPU::simDEC_R ()
{
    assert(Z80h[instr[opcode].arg1]);
	if (useix || useiy )
	{
		if ( instr[opcode].arg1 == simH )
			resDEC_byte( --*( useix ? &Z80.h.ixh : &Z80.h.iyh ) );
		else if ( instr[opcode].arg1 == simL )
			resDEC_byte( --*( useix ? &Z80.h.ixl : &Z80.h.iyl ) );
		else
			resDEC_byte( --*Z80h[instr[opcode].arg1] );
		//TODO: timing
	}
	else
	{
	    resDEC_byte(--*Z80h[instr[opcode].arg1]);
	}
    tstates += 4;
}

// DEC (HL)
// DEC (IX+d)
// DEC (IY+d)
void Z80CPU::simDEC_ATRX ()
{
    assert(Z80x[instr[opcode].arg1]);
    if (useix && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        resDEC_byte( putdata( Z80.x.ix+offset, getdata( Z80.x.ix+offset ) - 1 ) );
        tstates += 26;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        offset = sfetch();
        resDEC_byte( putdata( Z80.x.iy+offset, getdata( Z80.x.iy+offset ) - 1 ) );
        tstates += 26;
    } else {
        resDEC_byte( putdata( *Z80x[instr[opcode].arg1], getdata( *Z80x[instr[opcode].arg1] ) - 1 ) );
        tstates += 11;
    }
}


// GENERAL PURPOSE ARITHMETIC AND CPU CONTROL GROUP ///////////////////////////

// DAA
void Z80CPU::simDAA()
{
	uchar &a = Z80.h.a;
	uchar hi = a >> 4;
	uchar lo = a & 0x0F;
	uchar diff;

	if ( flags->c ) 
	{
		diff = ( lo > 9 || flags->h ) ? 0x66 : 0x60;
	} 
	else 
	{
		if ( lo >= 10 ) 
		{
			diff = ( hi > 8 ) ? 0x66 : 0x06;
		} 
		else 
		{
			if ( hi >= 10 ) 
			{
				diff = flags->h ? 0x66 : 0x60;
			} 
			else 
			{
				diff = flags->h ? 0x06 : 0x00;
			}
		}
	}

	a = flags->n ? a - diff : a + diff;							//	......N.
	
	flags->c |= hi >= ( lo <= 9 ? 10 : 9 );						//	.......C
	flags->h = flags->n ? ( flags->h && lo <= 5 ) : lo >= 10;	//	...H....
	flags->p = parity( a );										//	.....P..
	flags->z = a ? 0 : 1;										//	.Z......
	Z80.h.f &= ~0xA8;
	Z80.h.f |= a & 0xA8;										//	S.Y.X...

    tstates += 4;
}

// CPL
void Z80CPU::simCPL()
{
    Z80.h.a ^= 0xFF;
    flags->h = flags->n = 1;
    Z80.h.f &= ~0x28;
    Z80.h.f |= Z80.h.a & 0x28;	// ..Y.X...
    tstates += 4;
}

// NEG
void Z80CPU::simNEG()
{
    ushort x = Z80.h.a;
    ushort z;
    ushort y = ( z = 0 ) - x;
    uchar c = y>>8;
    uchar h = ((y ^ x)>>4) & 1;
    resSUB_byte(Z80.h.a = y, c, h, ovsub(x,z,0));
	tstates += 8;
}

// CCF
void Z80CPU::simCCF()
{
    flags->h = flags->c;
    flags->c ^= 1;
    flags->n = 0;
    Z80.h.f &= ~0x28;
    Z80.h.f |= Z80.h.a & 0x28;
    tstates += 4;
}

// SCF
void Z80CPU::simSCF()
{
    flags->c = 1;
    flags->h = flags->n = 0;
    Z80.h.f &= ~0x28;
    Z80.h.f |= Z80.h.a & 0x28;
    tstates += 4;
}

// NOP
void Z80CPU::simNOP ()
{
	tstates += 4;
}

// HALT
void Z80CPU::simHALT()
{
	if ( breakOnHalt )
	{
		setMode( MODE_STOP );
	}
	else if ( opcode == 0x76 )
	{
		// true HALT => loop
		--pc_;
	}
    tstates += 4;
}

// DI
void Z80CPU::simDI()
{
    // disable interrupts
    iff1 = iff2 = 0;
    tstates += 4;
#ifdef DBG_INT
	simStop( "DI" );
#endif
}

// EI
void Z80CPU::simEI()
{
    // enable interrupts
    iff1 = 1;
    iff2 = 0; // normally 1 - trick to skip next instruction
    tstates += 4;
	if ( breakOnEI )
		simStop( "EI" );
}

// IM 0
// IM 1
// IM 2
void Z80CPU::simIM()
{
    Z80.h.im = instr[opcode].arg1;
    tstates += 8;
}




// 16 BIT ARITHMETIC GROUP ////////////////////////////////////////////////////

#define ovadd16(y,x,c) ( (signed long)( (signed short)x + (signed short)y - (signed short)c ) != (signed short)( (signed short)x + (signed short)y - (signed short)c ) )
#define ovsub16(y,x,c) ( (signed long)( (signed short)x - (signed short)y + (signed short)c ) != (signed short)( (signed short)x - (signed short)y + (signed short)c ) )

// ADD HL,rr	rr=BC,DE,HL,SP
// ADD IX,rr	rr=BC,DE,IX,SP
// ADD IY,rr	rr=BC,DE,IY,SP
void Z80CPU::simADD_RX_RX()
{
    assert(instr[opcode].arg1 == simHL);
    assert(Z80x[instr[opcode].arg1] );
    assert(Z80x[instr[opcode].arg2] );

	// Op 2
	uint x, y;
	if (useix && instr[opcode].arg2 == simHL) {
        x = Z80.x.ix ;
    } else if (useiy && instr[opcode].arg2 == simHL) {
        x = Z80.x.iy ;
    } else {
        x = *Z80x[instr[opcode].arg2] ;
    }

	uint z = x;

	// Op 1
    if (useix && instr[opcode].arg1 == simHL) {
        x += ( y = Z80.x.ix ) ;
        Z80.x.ix = x;
        tstates += 15;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        x += ( y = Z80.x.iy ) ;
        Z80.x.iy = x;
        tstates += 15;
    } else {
        x += ( y = *Z80x[instr[opcode].arg1] ) ;
        *Z80x[instr[opcode].arg1] = x;
        tstates += 11;
    }
    flags->c = x>>16;
    flags->n = 0;
    flags->h = ( (x ^ y ^ z ) >> 12 ) & 1;
    Z80.h.f &= ~0x28;
    Z80.h.f |= (x>>8) & 0x28;
}

// ADC HL,rr	rr=BC,DE,HL,SP
// ADC IX,rr	rr=BC,DE,IX,SP
// ADC IY,rr	rr=BC,DE,IY,SP
void Z80CPU::simADC_RX_RX()
{
    assert(instr[opcode].arg1 == simHL);
    assert(Z80x[instr[opcode].arg1] );
    assert(Z80x[instr[opcode].arg2] );

	uint x, y;
	// Op 2
	if (useix && instr[opcode].arg2 == simHL) {
        x = Z80.x.ix ;
    } else if (useiy && instr[opcode].arg2 == simHL) {
        x = Z80.x.iy ;
    } else {
        x = *Z80x[instr[opcode].arg2] ;
    }

	uint z = x;

	// Op 1
	if (useix && instr[opcode].arg1 == simHL) {
        x += ( y = Z80.x.ix ) - flags->c;
        Z80.x.ix = x;
        tstates += 15;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        x += ( y = Z80.x.iy ) - flags->c;
        Z80.x.iy = x;
        tstates += 15;
    } else {
        x += ( y = *Z80x[instr[opcode].arg1] ) - flags->c;
        *Z80x[instr[opcode].arg1] = x;
        tstates += 15;
    }

	flags->p = ovadd16( y, z, flags->c );
	flags->c = x>>16;
    flags->n = 0;
    flags->s = (x>>15) & 1;
    flags->z = x==0 ? 1 : 0;
    flags->h = ( ( x ^ y ^ z ) >> 12 ) & 1;
    // todo: overflow
    Z80.h.f &= ~0x28;
    Z80.h.f |= (x>>8) & 0x28;
}

// SBC HL,rr	rr=BC,DE,HL,SP
// SBC IX,rr	rr=BC,DE,IX,SP
// SBC IY,rr	rr=BC,DE,IY,SP
void Z80CPU::simSBC_RX_RX()
{
    assert(instr[opcode].arg1 == simHL);
    assert(Z80x[instr[opcode].arg1] );
    assert(Z80x[instr[opcode].arg2] );

	uint x, y;
	// Op 2
    uint z;
    if (useix && instr[opcode].arg2 == simHL) {
        x = 0 - ( z = Z80.x.ix );
    } else if (useiy && instr[opcode].arg2 == simHL) {
        x = 0 - ( z = Z80.x.iy );
    } else {
        x = 0 - ( z = *Z80x[instr[opcode].arg2] ) ;
    }

	// Op 1
	if (useix && instr[opcode].arg1 == simHL) {
        x += ( y = Z80.x.ix ) + flags->c;
        Z80.x.ix = x;
        tstates += 15;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        x += ( y = Z80.x.iy ) + flags->c;
        Z80.x.iy = x;
        tstates += 15;
    } else {
        x += ( y = *Z80x[instr[opcode].arg1] ) + flags->c;
        *Z80x[instr[opcode].arg1] = x;
        tstates += 15;
    }

	flags->p = ovsub16( y, z, flags->c ) ? 1 :0;
	flags->c = x>>16;
    flags->n = 1;
    flags->s = (x>>15) & 1;
    flags->z = x==0 ? 1 : 0;
    flags->h = ( ( x ^ y ^ z ) >> 12 ) & 1;		// new
    // todo: overflow
    Z80.h.f &= ~0x28;
    Z80.h.f |= (x>>8) & 0x28;

}

// INC ss		ss=BC,DE,HL,SP
// INC IX
// INC IY
void Z80CPU::simINC_RX ()
{
    assert(Z80x[instr[opcode].arg1]);
    if (useix && instr[opcode].arg1 == simHL) {
        ++Z80.x.ix ;
        tstates += 10;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        ++Z80.x.iy ;
        tstates += 10;
    } else {
        ++*Z80x[instr[opcode].arg1];
        tstates += 6;
    }
}

// DEC ss		ss=BC,DE,HL,SP
// DEC IX
// DEC IY
void Z80CPU::simDEC_RX ()
{
    assert(Z80x[instr[opcode].arg1]);
    if (useix && instr[opcode].arg1 == simHL) {
        --Z80.x.ix ;
        tstates += 10;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        --Z80.x.iy ;
        tstates += 10;
    } else {
        --*Z80x[instr[opcode].arg1];
        tstates += 6;
    }
}




// ROTATE AND SHIFT GROUP /////////////////////////////////////////////////////

// RLCA
void Z80CPU::simRLCA ()
{
    ushort x = Z80.h.a<<1;
    x |= x>>8;
    resROTA(Z80.h.a=x, x&1);
    tstates += 4;
}

// RLA
void Z80CPU::simRLA ()
{
    ushort x = Z80.h.a<<1 | (flags->c & 1);
    resROTA(Z80.h.a=x, x>>8);
    tstates += 4;
}

// RRCA
void Z80CPU::simRRCA ()
{
    ushort x = Z80.h.a<<7;
    x |= x>>8;
    resROTA(Z80.h.a=x, x>>7);
    tstates += 4;
}

// RRA
void Z80CPU::simRRA ()
{
    uchar c = Z80.h.a & 1;
    ushort x = (Z80.h.a>>1) | (flags->c << 7) ;
    resROTA(Z80.h.a=x, c);
    tstates += 4;
}

// RLC r
void Z80CPU::simRLC_R ()
{
    assert(Z80h[instr[opcode].arg1]);
    ushort x = *Z80h[instr[opcode].arg1]<<1;
    x |= x>>8;
    resROT_byte(*Z80h[instr[opcode].arg1]=x, x&1);
    tstates += 8;
}

// RLC (HL)
// RLC (IX+d)
// RLC (IY+d)
void Z80CPU::simRLC_ATRX ()
{
    assert(instr[opcode].arg1==simHL);
	ushort y;
	if (useix && instr[opcode].arg1 == simHL) {
        y = Z80.x.ix+offset;
        tstates += 23;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        y = Z80.x.iy+offset;
        tstates += 23;
    } else {
        y = *Z80x[instr[opcode].arg1];
        tstates += 15;
    }
    ushort x = getdata(y)<<1;
    x |= x>>8;
    resROT_byte(putdata(y,x), x&1);
}

// RL r
void Z80CPU::simRL_R ()
{
    assert(Z80h[instr[opcode].arg1]);
    ushort x = *Z80h[instr[opcode].arg1]<<1 | (flags->c & 1);
    resROT_byte(*Z80h[instr[opcode].arg1]=x, x>>8);
    tstates += 8;
}

// RL (HL)
// RL (IX+d)
// RL (IY+d)
void Z80CPU::simRL_ATRX ()
{
    assert(instr[opcode].arg1==simHL);
	ushort y;
    if (useix && instr[opcode].arg1 == simHL) {
        y = Z80.x.ix+offset;
        tstates += 23;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        y = Z80.x.iy+offset;
        tstates += 23;
    } else {
        y = *Z80x[instr[opcode].arg1];
        tstates += 15;
    }
    ushort x = getdata(y)<<1 | (flags->c & 1);
    resROT_byte(putdata(y,x), x>>8);
}

// RRC r
void Z80CPU::simRRC_R ()
{
    assert(Z80h[instr[opcode].arg1]);
    ushort x = *Z80h[instr[opcode].arg1]<<7;
    x |= x>>8;
    resROT_byte(*Z80h[instr[opcode].arg1]=x, x>>7);
    tstates += 8;
}

// RRC (HL)
// RRC (IX+d)
// RRC (IY+d)
void Z80CPU::simRRC_ATRX ()
{
    assert(instr[opcode].arg1==simHL);
	ushort y;
    if (useix && instr[opcode].arg1 == simHL) {
        y = Z80.x.ix+offset;
        tstates += 23;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        y = Z80.x.iy+offset;
        tstates += 23;
    } else {
        y = *Z80x[instr[opcode].arg1];
        tstates += 15;
    }
    ushort x = getdata(y)<<7;
    x |= x>>8;
    resROT_byte(putdata(y,x), x>>7);
}

// RR r
void Z80CPU::simRR_R ()
{
    assert(Z80h[instr[opcode].arg1]);
    uchar c = *Z80h[instr[opcode].arg1] & 1;
    ushort x = (*Z80h[instr[opcode].arg1]>>1) | (flags->c << 7) ;
    resROT_byte(*Z80h[instr[opcode].arg1]=x, c);
    tstates += 8;
}

// RR (HL)
// RR (IX+d)
// RR (IY+d)
void Z80CPU::simRR_ATRX ()
{
    assert(instr[opcode].arg1==simHL);
	ushort y;
    if (useix && instr[opcode].arg1 == simHL) {
        y = Z80.x.ix+offset;
        tstates += 23;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        y = Z80.x.iy+offset;
        tstates += 23;
    } else {
        y = *Z80x[instr[opcode].arg1];
        tstates += 15;
    }
    ushort x = getdata(y);
    uchar c = x & 1;
    x = (x>>1) | (flags->c << 7) ;
    resROT_byte(putdata(y,x), c);
}

// SLA r
void Z80CPU::simSLA_R ()
{
    assert(Z80h[instr[opcode].arg1]);
    ushort x = (*Z80h[instr[opcode].arg1]<<1);
    resROT_byte(*Z80h[instr[opcode].arg1]=x, x>>8);
    tstates += 8;
}

// SLA (HL)
// SLA (IX+d)
// SLA (IY+d)
void Z80CPU::simSLA_ATRX ()
{
    assert(instr[opcode].arg1==simHL);
    ushort y;
    if (useix && instr[opcode].arg1 == simHL) {
        y = Z80.x.ix+offset;
        tstates += 23;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        y = Z80.x.iy+offset;
        tstates += 23;
    } else {
        y = *Z80x[instr[opcode].arg1];
        tstates += 15;
    }
    ushort x = getdata(y);
    uchar c = x >> 7;
    x = (x<<1)/* | 1*/ ;
    resROT_byte(putdata(y,x), c);
}

// SRA r
void Z80CPU::simSRA_R ()
{
    assert(Z80h[instr[opcode].arg1]);
    ushort x = *Z80h[instr[opcode].arg1];
    uchar c = x&1;
    x = (x>>1) | (x&0x80);
    resROT_byte(*Z80h[instr[opcode].arg1]=x, c);
    tstates += 8;
}

// SRA (HL)
// SRA (IX+d)
// SRA (IY+d)
void Z80CPU::simSRA_ATRX ()
{
    assert(instr[opcode].arg1==simHL);
    ushort y;
    if (useix && instr[opcode].arg1 == simHL) {
        y = Z80.x.ix+offset;
        tstates += 23;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        y = Z80.x.iy+offset;
        tstates += 23;
    } else {
        y = *Z80x[instr[opcode].arg1];
        tstates += 15;
    }
    ushort x = getdata(y);
    uchar c = x & 1;
    x = (x>>1) | (x&0x80) ;
    resROT_byte(putdata(y,x), c);
}

// SLL r (undocumented)
void Z80CPU::simSLL_R ()
{
    assert(Z80h[instr[opcode].arg1]);
    ushort x = (*Z80h[instr[opcode].arg1]<<1)|1;
    resROT_byte(*Z80h[instr[opcode].arg1]=x, x>>8);
    tstates += 8;
}

// SLL (HL) (undocumented)
// SLL (IX+d)
// SLL (IY+d)
void Z80CPU::simSLL_ATRX ()
{
    assert(instr[opcode].arg1==simHL);
    ushort y;
    if (useix && instr[opcode].arg1 == simHL) {
        y = Z80.x.ix+offset;
        tstates += 23;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        y = Z80.x.iy+offset;
        tstates += 23;
    } else {
        y = *Z80x[instr[opcode].arg1];
        tstates += 15;
    }
    ushort x = getdata(y);
    uchar c = x >> 7;
    x = ( x << 1 ) | 1;
    resROT_byte(putdata(y,x), c);
}

// SRL r
void Z80CPU::simSRL_R ()
{
    assert(Z80h[instr[opcode].arg1]);
    ushort x = *Z80h[instr[opcode].arg1];
    uchar c = x&1;
    x = (x>>1);
    resROT_byte(*Z80h[instr[opcode].arg1]=x, c);
    tstates += 8;
}

// SRL (HL)
// SRL (IX+d)
// SRL (IY+d)
void Z80CPU::simSRL_ATRX ()
{
    assert(instr[opcode].arg1==simHL);
    ushort y;
    if (useix && instr[opcode].arg1 == simHL) {
        y = Z80.x.ix+offset;
        tstates += 23;
    } else if (useiy && instr[opcode].arg1 == simHL) {
        y = Z80.x.iy+offset;
        tstates += 23;
    } else {
        y = *Z80x[instr[opcode].arg1];
        tstates += 15;
    }
    ushort x = getdata(y);
    uchar c = x & 1;
    x = (x>>1);
    resROT_byte(putdata(y,x), c);
}

// RLD
void Z80CPU::simRLD()
{
	ushort x = getdata( Z80.x.hl ) << 4;
	x |= Z80.h.a & 0x0F;
	Z80.h.a &= 0xF0;
	Z80.h.a |= x >> 8;
	putdata( Z80.x.hl, x );
	resRXD_byte( Z80.h.a );
	tstates += 18;
}

// RRD
void Z80CPU::simRRD()
{
	ushort x = getdata( Z80.x.hl ) ;
	x |= ( Z80.h.a & 0x0F ) << 8;
	Z80.h.a &= 0xF0;
	Z80.h.a |= x & 0x0F;
	putdata( Z80.x.hl, x >> 4 );
	resRXD_byte( Z80.h.a );
	tstates += 18;
}


// BIT SET RESET AND TEST GROUP ///////////////////////////////////////////////

// BIT b,r
void Z80CPU::simBIT_n_R()
{
    assert(Z80h[instr[opcode].arg2]);
    ushort x = *Z80h[instr[opcode].arg2];
	Z80.h.f &= ~0xA8;
	Z80.h.f |= ( 0xA8 & x & ( ( 1 << instr[opcode].arg1 ) | 0x28 ) );
    flags->z = flags->p = (~(x >> instr[opcode].arg1)) & 1;
	flags->h = 1;
	flags->n = 0;
    tstates += 8;
}

// BIT b,(HL)
// BIT b,(IX+d)
// BIT b,(IY+d)
void Z80CPU::simBIT_n_ATRX()
{
    // IX/IY ...
    assert(Z80x[instr[opcode].arg2]);
    
	ushort x;
	if (useix && instr[opcode].arg2 == simHL) {
        x = Z80.x.ix+offset;
        tstates += 20;
    } else if (useiy && instr[opcode].arg2 == simHL) {
        x = Z80.x.iy+offset;
        tstates += 20;
    } else {
        x = *Z80x[instr[opcode].arg2];
        tstates += 12;
    }
	ushort z = getdata(x);
	Z80.h.f &= ~0xA8;
	Z80.h.f |= ( 0xA8 & ( ( x >> 8 ) & 0x28 | ( z & 0x80 ) ) & ( 1 << instr[opcode].arg1 ) );
    flags->z = flags->p = (~(z >> instr[opcode].arg1)) & 1;
	flags->h = 1;
	flags->n = 0;
}

// SET b,r
void Z80CPU::simSET_n_R()
{
    assert(Z80h[instr[opcode].arg2]);
    ushort x = *Z80h[instr[opcode].arg2];
    x |= (1 << instr[opcode].arg1);
    *Z80h[instr[opcode].arg2] = x;
    tstates += 8;
}

// SET b,(HL)
// SET b,(IX+d)
// SET b,(IY+d)
void Z80CPU::simSET_n_ATRX()
{
    // IX/IY ...
    assert(Z80x[instr[opcode].arg2]);
    ushort x;
	if (useix && instr[opcode].arg2 == simHL) {
        x = Z80.x.ix+offset;
        tstates += 23;
    } else if (useiy && instr[opcode].arg2 == simHL) {
        x = Z80.x.iy+offset;
        tstates += 23;
    } else {
        x = *Z80x[instr[opcode].arg2];
        tstates += 15;
    }
    ushort y = getdata(x);
    y |= (1 << instr[opcode].arg1);
    putdata(x,y);
}

// RES b,r
void Z80CPU::simRES_n_R()
{
    assert(Z80h[instr[opcode].arg2]);
    ushort x = *Z80h[instr[opcode].arg2];
    x &= ~(1 << instr[opcode].arg1);
    *Z80h[instr[opcode].arg2] = x;
    tstates += 8;
}

// RES b,(HL)
// RES b,(IX+d)
// RES b,(IY+d)
void Z80CPU::simRES_n_ATRX()
{
    // IX/IY ...
    assert(Z80x[instr[opcode].arg2]);
	ushort x;
	if (useix && instr[opcode].arg2 == simHL) {
        x = Z80.x.ix+offset;
        tstates += 23;
    } else if (useiy && instr[opcode].arg2 == simHL) {
        x = Z80.x.iy+offset;
        tstates += 23;
    } else {
        x = *Z80x[instr[opcode].arg2];
        tstates += 15;
    }
    ushort y = getdata(x);
    y &= ~(1 << instr[opcode].arg1);
    putdata(x,y);
}


// JUMP GROUP /////////////////////////////////////////////////////////////////


// JP nn
void Z80CPU::simJP ()
{
	pc_ = laddr ();
	tstates += 10;
}

// JP Z,nn
void Z80CPU::simJP_Z ()
{
	ushort x = laddr();
	if (flags->z)
		pc_ = x;
	tstates += 10;
}

// JP NZ,nn
void Z80CPU::simJP_NZ ()
{
	ushort x = laddr();
	if (!flags->z)
		pc_ = x;
	tstates += 10;
}

// JP C,nn
void Z80CPU::simJP_C ()
{
	ushort x = laddr();
	if (flags->c)
		pc_ = x;
	tstates += 10;
}

// JP NC,nn
void Z80CPU::simJP_NC ()
{
	ushort x = laddr();
	if (!flags->c)
		pc_ = x;
	tstates += 10;
}

// JP PE,nn
void Z80CPU::simJP_PE ()
{
	ushort x = laddr();
	if (flags->p)
		pc_ = x;
	tstates += 10;
}

// JP PO,nn
void Z80CPU::simJP_PO ()
{
	ushort x = laddr();
	if (!flags->p)
		pc_ = x;
	tstates += 10;
}

// JP P,nn
void Z80CPU::simJP_P ()
{
	ushort x = laddr();
	if (!flags->s)
		pc_ = x;
	tstates += 10;
}

// JP M,nn
void Z80CPU::simJP_M ()
{
	ushort x = laddr();
	if (flags->s)
		pc_ = x;
	tstates += 10;
}

// JR e
void Z80CPU::simJR ()
{
	pc_ = saddr();
	tstates += 12;
}

// JR Z,e
void Z80CPU::simJR_Z ()
{
	ushort x = saddr();
	if (flags->z)
	{
		pc_ = x;
		tstates += 5;
	}
	tstates += 7;
}

// JR NZ,e
void Z80CPU::simJR_NZ ()
{
	ushort x = saddr();
	if (!flags->z)
	{
		pc_ = x;
		tstates += 5;
	}
	tstates += 7;
}

// JR C,e
void Z80CPU::simJR_C ()
{
	ushort x = saddr();
	if (flags->c)
	{
		pc_ = x;
		tstates += 5;
	}
	tstates += 7;
}

// JR NC,e
void Z80CPU::simJR_NC ()
{
	ushort x = saddr();
	if (!flags->c)
	{
		pc_ = x;
		tstates += 5;
	}
	tstates += 7;
}

// JP (HL)
// JP (IX)
// JP (IY)
void Z80CPU::simJP_ATRX()
{
    assert(Z80x[instr[opcode].arg1]);
	ushort x;
    if (useix && instr[opcode].arg1 == simHL) {
        x = Z80.x.ix;
        tstates += 8;

    } else if (useiy && instr[opcode].arg1 == simHL) {
        x = Z80.x.iy;
        tstates += 8;
    } else {
        x = *Z80x[instr[opcode].arg1];
        tstates += 4;
    }
    pc_ = x;
}

// DJNZ e
void Z80CPU::simDJNZ ()
{
	ushort x = saddr();
	tstates += 10;
	if (--Z80.h.b)
	{
		pc_ = x;
		tstates += 3;
	}
}


// CALL AND RETURN GROUP //////////////////////////////////////////////////////

// CALL nn
void Z80CPU::simCALL ()
{
	ushort x = laddr ();
	putdata( --Z80.x.sp, pc_ >> 8 );
	putdata( --Z80.x.sp, pc_ );
	pc_ = x;
	tstates += 17;
}

// CALL Z,nn
void Z80CPU::simCALL_Z ()
{
	ushort x = laddr();
	if (flags->z) {
		putdata(--Z80.x.sp, pc_ >> 8);
		putdata(--Z80.x.sp, pc_ );
		pc_ = x;
		tstates += 7;
	}
	tstates += 10;
}

// CALL NZ,nn
void Z80CPU::simCALL_NZ ()
{
	ushort x = laddr();
	if (!flags->z){
		putdata(--Z80.x.sp, pc_ >> 8);
		putdata(--Z80.x.sp, pc_ );
		pc_ = x;
		tstates += 7;
	}
	tstates += 10;
}

// CALL C,nn
void Z80CPU::simCALL_C ()
{
	ushort x = laddr();
	if (flags->c){
		putdata(--Z80.x.sp, pc_ >> 8);
		putdata(--Z80.x.sp, pc_ );
		pc_ = x;
		tstates += 7;
	}
	tstates += 10;
}

// CALL NC,nn
void Z80CPU::simCALL_NC ()
{
	ushort x = laddr();
	if (!flags->c){
		putdata(--Z80.x.sp, pc_ >> 8);
		putdata(--Z80.x.sp, pc_ );
		pc_ = x;
		tstates += 7;
	}
	tstates += 10;
}

// CALL PE,nn
void Z80CPU::simCALL_PE ()
{
	ushort x = laddr();
	if (flags->p){
		putdata(--Z80.x.sp, pc_ >> 8);
		putdata(--Z80.x.sp, pc_ );
		pc_ = x;
		tstates += 7;
	}
	tstates += 10;
}

// CALL PO,nn
void Z80CPU::simCALL_PO ()
{
	ushort x = laddr();
	if (!flags->p){
		putdata(--Z80.x.sp, pc_ >> 8);
		putdata(--Z80.x.sp, pc_ );
		pc_ = x;
		tstates += 7;
	}
	tstates += 10;
}

// CALL P,nn
void Z80CPU::simCALL_P ()
{
	ushort x = laddr();
	if (!flags->s){
		putdata(--Z80.x.sp, pc_ >> 8);
		putdata(--Z80.x.sp, pc_ );
		pc_ = x;
		tstates += 7;
	}
	tstates += 10;
}

// CALL M,nn
void Z80CPU::simCALL_M ()
{
	ushort x = laddr();
	if (flags->s){
		putdata(--Z80.x.sp, pc_ >> 8);
		putdata(--Z80.x.sp, pc_ );
		pc_ = x;
		tstates += 7;
	}
	tstates += 10;
}

// RET
void Z80CPU::simRET ()
{
	pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
	tstates += 10;
}

// RET Z
void Z80CPU::simRET_Z ()
{
	if (flags->z) {
		pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
		tstates += 12;
	}
	tstates += 5;
}

// RET NZ
void Z80CPU::simRET_NZ ()
{
	if (!flags->z){
		pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
		tstates += 12;
	}
	tstates += 5;
}

// RET C
void Z80CPU::simRET_C ()
{
	if (flags->c){
		pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
		tstates += 12;
	}
	tstates += 5;
}

// RET NC
void Z80CPU::simRET_NC ()
{
	if (!flags->c){
		pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
		tstates += 12;
	}
	tstates += 5;
}

// RET PE
void Z80CPU::simRET_PE ()
{
	if (flags->p){
		pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
		tstates += 12;
	}
	tstates += 5;
}

// RET PO
void Z80CPU::simRET_PO ()
{
	if (!flags->p){
		pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
		tstates += 12;
	}
	tstates += 5;
}

// RET P
void Z80CPU::simRET_P ()
{
	if (!flags->s){
		pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
		tstates += 12;
	}
	tstates += 5;
}

// RET M
void Z80CPU::simRET_M ()
{
	if (flags->s){
		pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
		tstates += 12;
	}
	tstates += 5;
}

// RETI
void Z80CPU::simRETI ()
{
    pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
	tstates += 14;
}

// RETN
void Z80CPU::simRETN ()
{
	iff1 = iff2;
	pc_ = (getdata(Z80.x.sp++)) | (getdata(Z80.x.sp++)<<8);
	tstates += 14;
}

// RST p
void Z80CPU::simRST ()
{
	putdata( --Z80.x.sp, pc_ >> 8 );
	putdata( --Z80.x.sp, pc_ );
	pc_ = instr[opcode].arg1;
	tstates += 11;
}


// INPUT AND OUTPUT GROUP /////////////////////////////////////////////////////

// IN A,(n)
void Z80CPU::simIN_R_ATb()
{
    ushort x = fetch();
    *Z80h[instr[opcode].arg1] = indata( x );
    tstates += 11;
}

// IN A,(n)
void Z80CPU::simIN_R_ATR()
{
    ushort x = *Z80h[instr[opcode].arg1] = indata( *Z80h[instr[opcode].arg2] );
	flags->n = flags->h = 0;
    flags->p = parity(x);
    flags->z = x==0 ? 1 : 0;
    Z80.h.f &= ~0xA8;
    Z80.h.f |= x & 0xA8;
    tstates += 12;
}

// IN r,(C)

// INI
void Z80CPU::simINI()
{
	putdata( Z80.x.hl, indata( Z80.h.c ) );
	--Z80.h.b;
	++Z80.x.hl;
	resDEC_byte(Z80.h.b); // temp: flags not all ok
	tstates += 16;
}


// INIR
void Z80CPU::simINIR()
{
	putdata( Z80.x.hl, indata( Z80.h.c ) );
	++Z80.x.hl;
    if ( --Z80.h.b )
    {
    	pc_ -= 2;
    	tstates += 3;
	}

	resDEC_byte(Z80.h.b); // temp: flags not all ok
	tstates += 16;
}


// IND
// INI
void Z80CPU::simIND()
{
	putdata( Z80.x.hl, indata( Z80.h.c ) );
	--Z80.h.b;
	--Z80.x.hl;
	resDEC_byte(Z80.h.b); // temp: flags not all ok
	tstates += 16;
}


// INDR

// OUT (n),a
void Z80CPU::simOUT_ATb_R()
{
    ushort x = fetch();
    outdata( x, *Z80h[instr[opcode].arg2] );
    tstates += 11;
}

// OUT (C),r
void Z80CPU::simOUT_ATR_R()
{
    // output to port
    outdata( *Z80h[instr[opcode].arg1], *Z80h[instr[opcode].arg2] );
    tstates += 12;
}

// OUTI
void Z80CPU::simOUTI()
{
	outdata( Z80.h.c, getdata( Z80.x.hl ) );
	--Z80.h.b;
	++Z80.x.hl;
	resDEC_byte(Z80.h.b); // temp: flags not all ok
	tstates += 16;
}

// OTIR
void Z80CPU::simOTIR()
{
	outdata( Z80.h.c, getdata( Z80.x.hl ) );
	++Z80.x.hl;
    if ( --Z80.h.b )
    {
    	pc_ -= 2;
    	tstates += 3;
	}

	resDEC_byte(Z80.h.b); // temp: flags not all ok
	tstates += 16;
}

// OUTD
void Z80CPU::simOUTD()
{
	outdata( Z80.h.c, getdata( Z80.x.hl ) );
	--Z80.h.b;
	--Z80.x.hl;
	resDEC_byte(Z80.h.b); // temp: flags not all ok
	tstates += 16;
}

// OTDR

// MULTI-BYTE OPCODES /////////////////////////////////////////////////////////

// Z-80 extended instructions set with CB prefix
void Z80CPU::simCB()
{
    if (useix || useiy) 
	{
		// Fetch offset for indexed memory access
        offset = sfetch() ;
	}
	else
	{
		incR();
    }

    opcode = 0x100 | fetch();
	(this->*instr[opcode].simop)();
}

// Z-80 extended instructions set with DD prefix (select IX register instead of HL)
void Z80CPU::simDD()
{
	incR();
    useix = 1;
    opcode = fetch();
	selectIX();
	(this->*instr[opcode].simop)();
	selectHL();
}

// Z-80 extended instructions set with ED prefix
void Z80CPU::simED()
{
	incR();
    opcode = 0x200 | fetch();
	(this->*instr[opcode].simop)();
}

// Z-80 extended instructions set with DD prefix (select IY register instead of HL)
void Z80CPU::simFD()
{
	incR();
    useiy = 1;
    opcode = fetch();
	selectIY();
	(this->*instr[opcode].simop)();
	selectHL();
}


// EXTENDED SIMULATOR INSTRUCTIONS - DAVID KEIL ///////////////////////////////

// David Keil's extended instructions

void Z80CPU::simDIV16() // DIV16 HL, DE
{
	uint n, d, q, r;
	n = (uint)Z80.x.hl;
	d = (uint)Z80.x.de;
	if ( d )
	{
		q = n / d;
		r = n % d;
		flags->c = flags->p = false;
		Z80.x.hl = q;
		Z80.x.de = r;
	}
	else
	{
		flags->c = flags->p = true;
	}
	trace( 	cprintf( "n=%d d=%d q=%d r=%d\r\n", n, d, q, r );
			cprintf( "n=%08x d=%08x q=%08x r=%08x\r\n", n, d, q, r );
			getch();
		);
}

void Z80CPU::simDIV32() // DIV32 HL:DE, IY:IX (OK)
{
	ulong n, d, q, r;
	n = ( (ulong)Z80.x.hl << 16 ) | ( (ulong)Z80.x.de & 0xFFFF ) ;
	d = ( (ulong)Z80.x.iy << 16 ) | ( (ulong)Z80.x.ix & 0xFFFF ) ;
	if ( d )
	{
		q = n / d;
		r = n % d;
		flags->c = flags->p = false;
		Z80.x.hl = (ushort)( q >> 16 );
		Z80.x.de = (ushort)( q & 0xFFFF );
		Z80.x.iy = (ushort)( r >> 16 );
		Z80.x.ix = (ushort)( r & 0xFFFF );
	}
	else
	{
		flags->c = flags->p = true;
	}
	trace( 	cprintf( "n=%d d=%d q=%d r=%d\r\n", n, d, q, r );
			cprintf( "n=%08x d=%08x q=%08x r=%08x\r\n", n, d, q, r );
			getch();
		);
}

void Z80CPU::simIDIV32() // DIV32 HL:DE, IY:IX (OK)
{
	signed long n, d, q, r;
	n = ( (ulong)Z80.x.hl << 16 ) | ( (ulong)Z80.x.de & 0xFFFF ) ;
	d = ( (ulong)Z80.x.iy << 16 ) | ( (ulong)Z80.x.ix & 0xFFFF ) ;
	if ( d )
	{
		q = n / d;
		r = n % d;
		flags->c = flags->p = false;
		Z80.x.hl = (ushort)( q >> 16 );
		Z80.x.de = (ushort)( q & 0xFFFF );
		Z80.x.iy = (ushort)( r >> 16 );
		Z80.x.ix = (ushort)( r & 0xFFFF );
	}
	else
	{
		flags->c = flags->p = true;
	}
	trace( 	cprintf( "n=%d d=%d q=%d r=%d\r\n", n, d, q, r );
			cprintf( "n=%08x d=%08x q=%08x r=%08x\r\n", n, d, q, r );
			getch();
		);
}

/*	Set CPU speed to value defined by register A.

	bits 0-1
	   00   2/1.77mhz (initial speed)
	   01   4mhz
	   10   1mhz
	   11   8mhz

	bit 2   Turbo speed if set

	bit 3   Speed port locked if set

*/
void Z80CPU::simSSPD()
{
	Z80SimSpeedFlags = Z80.h.a;
	setSimSpeed();
}

/*	Get CPU speed and return value in register A.

	bits 0-1
	   00   2/1.77mhz (initial speed)
	   01   4mhz
	   10   1mhz
	   11   8mhz

	bit 2   Turbo speed if set

	bit 3   Speed port locked if set
*/
void Z80CPU::simGSPD()
{
	Z80.h.a = Z80SimSpeedFlags;
}

void Z80CPU::simOPEN() // ED 30 = OPEN BC, DE, HL
{
#if USE_DOS
	unsigned int mode, access, handle;
	char name[MAXPATH];
	int i;

	for( i=0; i<MAXPATH; i++ )
	{
		name[i] = getdata( Z80.x.hl + i );
		if ( name[i] == 0 )
			break;
	}
	mode = Z80.x.bc & O_ACCMODE ; 	// fcntl.h: mask for file access modes

	if ( Z80.x.bc & (1<<6) )
		mode |= O_CREAT;			// fcntl.h: create and open file
	if ( Z80.x.bc & (1<<7) )
		mode |= O_EXCL;				// fcntl.h: exclusive open
	if ( Z80.x.bc & (1<<9) )
		mode |= O_TRUNC;			// fcntl.h: open with truncation
	if ( Z80.x.bc & (1<<10) )
		mode |= O_APPEND;			// fcntl.h: to end of file
	mode |= O_BINARY | O_RDWR;		// fcntl.h: no translation
	access = ( ( Z80.x.de & 0x100 ) ? S_IWRITE : 0 ) 
		   | ( ( Z80.x.de & 0x80  ) ? S_IREAD  : 0 );				
									// file access rights
	handle = open( name, mode, access );
	Z80.x.de = handle;				// output: DE = file handle
	if ( handle == -1 )
	{
		Z80.h.a = errno_();			// global variable containing error #
		flags->z = 0;				// reset Z flag
		perror("simOPEN");
	}
	else
	{
		Z80.h.a = 0;				// 0 = no error
		flags->z = 1;				// set Z flag
	}

#endif
}

void Z80CPU::simCLOSE() // ED 31 = CLOSE DE
{
#if USE_DOS
	if ( close( Z80.x.de ) )
	{
		// unsuccessful
		Z80.h.a = errno_();			// global variable containing error #
		flags->z = 0;				// reset Z flag
		perror("simCLOSE");
	}
	else
	{
		// successful
		Z80.h.a = 0;				// 0 = no error
		flags->z = 1;				// set Z flag
	}
#endif
}

/* 	Read the number of bytes defined by register BC into the buffer area pointed to by register HL
	from the channel defined by register DE. If the Z flag is set then the read was successful,
	register BC will contain the number of bytes actually read and the buffer will contain the data read.
	If BC is less than the value before execution of the instruction and there was no error then
	the end of file was reached. If an read error occurred then the Z flag is reset and BC will
	contain the number of bytes actually read, & register A will contain the error number.
*/
void Z80CPU::simREAD() // ED 32 = READ BC, DE, HL
{
#if USE_DOS
	uint length = Z80.x.bc;
	uint i;
	char* buffer;

	buffer = (char*)malloc(length);
	if ( buffer )
	{
		if ( (int)( length = read( Z80.x.de, buffer, length ) ) == -1 )
		{
			// unsuccessful
			Z80.h.a = errno_();			// global variable containing error #
			flags->z = 0;				// reset Z flag
			perror("simREAD");
		}
		else
		{
			// successful
			Z80.h.a = 0;				// 0 = no error
			flags->z = 1;				// set Z flag
			for (i=0; i<length; ++i )
			{
				putdata( Z80.x.hl+i, *(buffer+i) );
			}
		}
		Z80.x.bc = length;
		free( buffer );
	}
	else
		simStop( "simREAD: buffer alloc failed" );
#endif
}

/* 	Write the number of bytes defined by register BC from the buffer area pointed to by register HL
	to the channel defined by register DE. If the Z flag is set then the write was successful,
	register BC will contain the number of bytes actually written and the buffer will be unchanged.
	If an write error occurred then the Z flag is reset and BC will contain the number of bytes
	actually written & register A will contain the error number.
*/
void Z80CPU::simWRITE() // ED 33 = WRITE BC, DE, HL
{
#if USE_DOS
	uint length = Z80.x.bc;
	uint i;
	char* buffer;

	buffer = (char*)malloc(length);
	if ( buffer )
	{
		for (i=0; i<length; ++i )
		{
			buffer[i] = getdata( Z80.x.hl+i );
		}
		if ( (int)( length = write( Z80.x.de, buffer, length ) ) == -1 )
		{
			// unsuccessful
			Z80.h.a = errno_();			// global variable containing error #
			flags->z = 0;				// reset Z flag
			perror("simWRITE");
		}
		else
		{
			// successful
			Z80.h.a = 0;				// 0 = no error
			flags->z = 1;				// set Z flag
		}
		Z80.x.bc = length;
		free( buffer );
	}
	else
		simStop( "simWRITE: buffer alloc failed" );
#endif
}

/*	Set the file position to the 32 bit signed integer value pointed to by register HL using the 
	method defined in register C. If successful the Z flag is set and the 32 bit signed integer 
	value pointed to by HL is updated with the new file position. If an error occurs then the Z 
	flag is reset and register A contains the error number. 

	Using method 1 or 2 it is possible to position beyond the start of the file without an error, 
	an error will occur upon a subsequent read or write to the file.

	Method code:
	C = 0 absolute byte offset from beginning of file (always positive double integer)
	C = 1 byte offset from current location (positive or negative double integer)
	C = 2 byte offset from end of file (positive or negative double integer)
*/
void Z80CPU::simSEEK() // ED 34 - SEEK HL=ptr, DE=handle, C=method
{
#if USE_DOS
	char buffer[4];
	long offset;

	for ( int i=0; i<sizeof( buffer ); ++i )
	{
		buffer[i] = getdata( Z80.x.hl+i );
	}
	offset = ( ( ( ( ( buffer[3] << 8 ) | buffer[2] ) << 8 ) | buffer[1] ) << 8 ) | buffer[0];
	offset = lseek( Z80.x.de, offset, Z80.h.c>2 ? 0 : Z80.h.c ); // method = 0 if invalid

	for ( int i=0; i<sizeof( buffer ); ++i )
	{
		putdata( Z80.x.hl+i, buffer[i] );
	}

	if ( offset == -1 )
	{
		Z80.h.a = errno_();
		flags->z = 0;
		perror( "simSEEK" );
		simStop( "simSEEK" );
	}
	else
	{
		Z80.h.a = 0;
		flags->z = 1;
	}
#endif
}

/*	Builds error message in buffer pointed to by register HL for the error number in register A. 
	If the length of the buffer is not able to hold the entire message the message is truncated 
	to fit and register A contains range error, else register A is set to zero.
*/
void Z80CPU::simERROR()
{
#if USE_DOS
	const char *error;
	int len;
	int i;

	error = strerror( Z80.h.a );
	len = (int)strlen( error );
	Z80.h.a = 0;
	if ( len > Z80.x.bc - 2)
	{
		len = Z80.x.bc - 2;
		Z80.h.a = 0x24; // Sharing buffer overflow
	}
	flags->z = Z80.h.a ? 0 : 1;
	for ( i=0; i<len; ++i )
	{
		putdata( Z80.x.hl + i, error[i] );
	}

	putdata( Z80.x.hl + len, 0x0D ); // additional CR for TRS-80 DOS
	putdata( Z80.x.hl + len + 1, 0 );
#endif
}



#pragma pack(push,1)
typedef struct simFINDDATA
{
	char reserved[21];
	uchar fileattr;
	unsigned short filetime;
	unsigned short filedate;
	unsigned long filesize;
	char filename[13];
} simFINDDATA;
#pragma pack(pop)

/*	Given a file specification in the form of an ASCIIZ string, searches the default or specified
	directory on the default or specified disk drive for the first matching file. If a match is
	found the buffer pointed to by register HL will be loaded with the file data found and the Z
	flag is set, else the Z flag is reset and register A contains the error number.

	Wildcard characters ? and * are allowed in the filename. Files found are based on the attributes
	set in the DE register.

	WARNING: Buffer must be at least 43 bytes long.
*/
void Z80CPU::simFINDFIRST()
{
#if USE_DIR
	simFINDDATA buffer;
	char* pbuffer = (char*)&buffer;
	struct ffblk* pffblk;
	uint i;
	int done;

	for ( i=0; i<43; i++ )
	{
		pbuffer[i] = getdata( Z80.x.hl + i );
	}

	if ( !strcmp( pbuffer, "*.*" ) )
		strcpy( pbuffer, "????????.???" );

	pffblk = (struct ffblk*)malloc( sizeof( struct ffblk ) );

	done = findfirst( pbuffer, pffblk, Z80.x.de|0xFFC0 );

	if ( !done )
	{
		flags->z = 1;
		Z80.h.a = 0;
		buffer.fileattr = pffblk->ff_attrib & 0xFF;
		buffer.filetime = pffblk->ff_ftime;
		buffer.filedate = pffblk->ff_fdate;
		buffer.filesize = pffblk->ff_fsize;
		for ( i=0; i<13; i++ )
		{
			buffer.filename[i] = toupper( pffblk->ff_name[i] );
		}
		buffer.filename[12] = 0;
	}
	else
	{
		flags->z = 0;
		Z80.h.a = errno_();
		buffer.filename[0] = 0;
	}


	*(struct ffblk**)(pbuffer) = pffblk;


	for ( i=0; i<43; i++)
	{
		 putdata( Z80.x.hl + i, pbuffer[i] );
	}

#endif
}

void Z80CPU::simFINDNEXT()
{
#if USE_DIR
	simFINDDATA buffer;
	char* pbuffer = (char*)&buffer;
	struct ffblk* pffblk;
	uint i;
	int done;

	for ( i=0; i<43; i++ )
	{
		pbuffer[i] = getdata( Z80.x.hl + i );
	}

	pffblk = *(struct ffblk**)(pbuffer);

	if ( pffblk )
	{
		done = findnext( pffblk );

		if ( !done )
		{
			flags->z = 1;
			Z80.h.a = 0;
			buffer.fileattr = pffblk->ff_attrib & 0xFF;
			buffer.filetime = pffblk->ff_ftime;
			buffer.filedate = pffblk->ff_fdate;
			buffer.filesize = pffblk->ff_fsize;
			for ( i=0; i<13; i++ )
			{
				buffer.filename[i] = toupper( pffblk->ff_name[i] );
			}
			buffer.filename[12] = 0;
		}
		else
		{
			findclose( pffblk );
			free ( pffblk );
			*(struct ffblk**)(pbuffer) = 0;
			Z80.h.a = errno_();
			flags->z = 0;
		}
	}

	for ( i=0; i<43; i++)
	{
		 putdata( Z80.x.hl + i, pbuffer[i] );
	}

#endif
}

void Z80CPU::simLOAD()
{
#if USE_LOAD
	char name[MAXPATH];
	int i;

	for( i=0; i<MAXPATH; i++ )
	{
		name[i] = getdata( Z80.x.hl + i );
		if ( name[i] == 0 )
			break;
	}

	Z80.x.bc = loadcmdfile( name );
	flags->z = Z80.x.bc ? 1 : 0;
	Z80.h.a = Z80.x.bc ? 0 : 0xFF;
#endif
}

/*	Changes the current drive and directory to the path pointed to by the register HL. If the command
	is successful the Z flag is set else the Z flag is reset and the A register contains the error
	number.
*/
void Z80CPU::simCHDIR()
{
#if USE_DIR
	char dir[MAXDIR];
	int i;

	for( i=0; i<MAXDIR; i++ )
	{
		dir[i] = getdata( Z80.x.hl+i );
	}

	flags->z = chdir( dir ) ? 0 : 1;
	Z80.h.a = flags->z ? 0 : errno_();
#endif
}

/*	Gets the current working directory and drive. The path and drive are stored in the buffer pointed
	to by the register HL. If the command is successful the Z flag is set else the Z flag is reset
	and the A register contains the error number. If the full path is longer than the buffer an error
	will occur.
*/
void Z80CPU::simGETDIR()
{
#if USE_DIR
	char dir[MAXDIR] = "@:\\";
	int i;

	dir[0] = 'A' + getdisk();

	flags->z = getcurdir( 0, dir+3 ) ? 0 : 1;

	for( i=0; i<Z80.x.bc && i<MAXDIR; i++ )
	{
		putdata( Z80.x.hl+i, toupper( dir[i] ) );
	}

	Z80.h.a = flags->z ? 0 : errno_();
#endif
}

void Z80CPU::simPATHOPEN()
{
#if USE_DOS
	unsigned int mode, access, handle;
	char name[MAXPATH];
#if USE_HARD
	char mask[] = "HARD?-?";
#endif
	int i;

	for( i=0; i<MAXPATH; i++ )
	{
		name[i] = getdata( Z80.x.hl + i );
		if ( name[i] == 0 )
			break;
	}

#if USE_HARD
	for( i=0; i < sizeof( mask ); i++ )
	{
		if ( mask[i] == '?' )
		{
			if ( name[i] == 0 )
				break;
		}
		else
		{
			if ( ( name[i] & 0xDF ) != ( mask[i] & 0xDF ) )
				break;
		}
	}

	if ( i == sizeof( mask ) )
	{
		strcpy( name, hardfiles[name[6]-'0'] );
	}
#endif

	mode = Z80.x.bc & O_ACCMODE ; 	// fcntl.h: mask for file access modes

	if ( Z80.x.bc & (1<<6) )
		mode |= O_CREAT;			// fcntl.h: create and open file
	if ( Z80.x.bc & (1<<7) )
		mode |= O_EXCL;				// fcntl.h: exclusive open
	if ( Z80.x.bc & (1<<9) )
		mode |= O_TRUNC;			// fcntl.h: open with truncation
	if ( Z80.x.bc & (1<<10) )
		mode |= O_APPEND;			// fcntl.h: to end of file
	mode |= O_BINARY | O_RDWR;		// fcntl.h: no translation
	access = ( ( Z80.x.de & 0x100 ) ? S_IWRITE : 0 ) 
		   | ( ( Z80.x.de & 0x80  ) ? S_IREAD  : 0 );				
									// file access rights
	handle = open( name, mode, access );
	Z80.x.de = handle;				// output: DE = file handle
	if ( handle == -1 )
	{
		Z80.h.a = errno_();			// global variable containing error #
		flags->z = 0;				// reset Z flag
		perror("simOPEN");
	}
	else
	{
		Z80.h.a = 0;				// 0 = no error
		flags->z = 1;				// set Z flag
	}

#endif
}


void Z80CPU::simCLOSEALL()
{
	Z80.h.a = 0x00;
	resOR_byte( Z80.h.a );
	simStop("#CLOSEALL");
}



// EXTENDED SIMULATOR INSTRUCTIONS - SIMZ8032 /////////////////////////////////

// extended instructions

#if USE_DOS
File_I		*csfile=0;
short		csbyte=0;
char		cscount=0;
#endif

#ifndef DO_updateDisplay
int DO_updateDisplay();
#endif

// Break execution and invoke debugger
void Z80CPU::simBREAK()
{
	setMode( MODE_STOP );
}

// End session and exit emulator
void Z80CPU::simEXIT()
{
	setMode( MODE_EXIT );
}

// Input cassette bit (TRS-80)
void Z80CPU::simCSINB()
{
#if USE_DOS
    if ( csfile == NULL || !*csfile ) 
	{
        csfile = readfile( console_, this, "Load CAS file:", "rb", ".CAS" );
		cscount = 0;
        csbyte = 0;
    }

    if ( csfile == NULL || !*csfile )
	{
		simStop( "Load CAS file failed." );
	}
	else if ( cscount == 0 ) 
	{
		csbyte |= csfile->getc();
        cscount = 8;
    }
    csbyte <<= 1;
    cscount --;
    Z80.h.a = csbyte >> 8;
#endif
}

// Output cassette byte (TRS-80)
void Z80CPU::simCSOUTC()
{
#if USE_DOS
    if(csfile==NULL) {
        csfile = readfile( console_, this, "Save CAS file:", "wb", ".CAS" );
    }
	csfile->putc( Z80.h.a );
#endif
}

// Stop cassette (TRS-80)
void Z80CPU::simCSEND()
{
#if USE_DOS
	if ( csfile )
	{
		csfile->close();
		delete csfile;
		csfile = NULL;
	}
#endif
}

// Scan keyboard
void Z80CPU::simKBINC()
{
	if ( true )
	{
		Z80.h.a = kbhit() ? getch() : 0;
	}
	else
	{
		Z80.h.a = Z80KeyPressed;
		Z80KeyPressed = 0;
	}
}

// Wait keystroke
void Z80CPU::simKBWAIT()
{
	if ( true )
	{
		Z80.h.a = getch();
	}
	else
	{
		if ( Z80KeyPressed == 0 ) 
		{
			if ( getMode() != MODE_STOP ) 
			{
				Z80Wait();
				DO_updateDisplay();
				ungetch( getch() );
				pc_ -= 2;
			}
		} 
		else 
		{
			Z80.h.a = Z80KeyPressed;
			Z80KeyPressed = 0;
		}
	}
}

// Send character to terminal
void Z80CPU::simPUTCH()
{
	putch( Z80.h.a );
}

// Enable Break on EI
void Z80CPU::simEIBRKON()
{
	breakOnEI = 1;
}

// Disable Break on EI
void Z80CPU::simEIBRKOFF()
{
	breakOnEI = 0;
}

// PROCESSOR INSTRUCTIONS TABLE ///////////////////////////////////////////////

//  Processor's instruction set
instr Z80CPU::instr[] = {
// 00-0F
        NOP,            0,              0,              &Z80CPU::simNOP,         0,              0,
        LD,             RX,             WORD,           &Z80CPU::simLD_RX_w,     simBC,          0,
        LD,             ATRX,           R,              &Z80CPU::simLD_ATRX_R,   simBC,          simA,
        INC,            RX,             0,              &Z80CPU::simINC_RX,      simBC,          0,
        INC,            R,              0,              &Z80CPU::simINC_R,       simB,           0,
        DEC,            R,              0,              &Z80CPU::simDEC_R,       simB,           0,
        LD,             R,              BYTE,           &Z80CPU::simLD_R_b,      simB,           0,
        RLCA,           0,              0,              &Z80CPU::simRLCA,        0,              0,
        EX,             RX,             AFP,            &Z80CPU::simEX_AF_AFP,   simAF,          0,
        ADD,            RX,             RX,             &Z80CPU::simADD_RX_RX,   simHL,          simBC,
        LD,             R,              ATRX,           &Z80CPU::simLD_R_ATRX,   simA,           simBC,
        DEC,            RX,             0,              &Z80CPU::simDEC_RX,      simBC,          0,
        INC,            R,              0,              &Z80CPU::simINC_R,       simC,           0,
        DEC,            R,              0,              &Z80CPU::simDEC_R,       simC,           0,
        LD,             R,              BYTE,           &Z80CPU::simLD_R_b,      simC,           0,
        RRCA,           0,              0,              &Z80CPU::simRRCA,        0,              0,
// 10-1F
        DJNZ,           OFFSET,         0,              &Z80CPU::simDJNZ,        0,              0,
        LD,             RX,             WORD,           &Z80CPU::simLD_RX_w,     simDE,          0,
        LD,             ATRX,           R,              &Z80CPU::simLD_ATRX_R,   simDE,          simA,
        INC,            RX,             0,              &Z80CPU::simINC_RX,      simDE,          0,
        INC,            R,              0,              &Z80CPU::simINC_R,       simD,           0,
        DEC,            R,              0,              &Z80CPU::simDEC_R,       simD,           0,
        LD,             R,              BYTE,           &Z80CPU::simLD_R_b,      simD,           0,
        RLA,            0,              0,              &Z80CPU::simRLA,         0,              0,
        JR,             OFFSET,         0,              &Z80CPU::simJR,          0,              0,
        ADD,            RX,             RX,             &Z80CPU::simADD_RX_RX,   simHL,          simDE,
        LD,             R,              ATRX,           &Z80CPU::simLD_R_ATRX,   simA,           simDE,
        DEC,            RX,             0,              &Z80CPU::simDEC_RX,      simDE,          0,
        INC,            R,              0,              &Z80CPU::simINC_R,       simE,           0,
        DEC,            R,              0,              &Z80CPU::simDEC_R,       simE,           0,
        LD,             R,              BYTE,           &Z80CPU::simLD_R_b,      simE,           0,
        RRA,            0,              0,              &Z80CPU::simRRA,         0,              0,
// 20-2F
        JR,             NZ,             OFFSET,         &Z80CPU::simJR_NZ,       0,              0,
        LD,             RX,             WORD,           &Z80CPU::simLD_RX_w,     simHL,          0,
        LD,             ATWORD,         RX,             &Z80CPU::simLD_ATw_RX,   0,              simHL,
        INC,            RX,             0,              &Z80CPU::simINC_RX,      simHL,          0,
        INC,            R,              0,              &Z80CPU::simINC_R,       simH,           0,
        DEC,            R,              0,              &Z80CPU::simDEC_R,       simH,           0,
        LD,             R,              BYTE,           &Z80CPU::simLD_R_b,      simH,           0,
        DAA,            0,              0,              &Z80CPU::simDAA,         0,              0,
        JR,             Z,              OFFSET,         &Z80CPU::simJR_Z,        0,              0,
        ADD,            RX,             RX,             &Z80CPU::simADD_RX_RX,   simHL,          simHL,
        LD,             RX,             ATWORD,         &Z80CPU::simLD_RX_ATw,   simHL,          0,
        DEC,            RX,             0,              &Z80CPU::simDEC_RX,      simHL,          0,
        INC,            R,              0,              &Z80CPU::simINC_R,       simL,           0,
        DEC,            R,              0,              &Z80CPU::simDEC_R,       simL,           0,
        LD,             R,              BYTE,           &Z80CPU::simLD_R_b,      simL,           0,
        CPL,            0,              0,              &Z80CPU::simCPL,         0,              0,
// 30-3F
        JR,             NC,             OFFSET,         &Z80CPU::simJR_NC,       0,              0,
        LD,             RX,             WORD,           &Z80CPU::simLD_RX_w,     simSP,          0,
        LD,             ATWORD,         R,              &Z80CPU::simLD_ATw_R,    0,              simA,
        INC,            RX,             0,              &Z80CPU::simINC_RX,      simSP,          0,
        INC,            ATRX,           0,              &Z80CPU::simINC_ATRX,    simHL,          0,
        DEC,            ATRX,           0,              &Z80CPU::simDEC_ATRX,    simHL,          0,
        LD,             ATRX,           BYTE,           &Z80CPU::simLD_ATRX_b,   simHL,          0,
        SCF,            0,              0,              &Z80CPU::simSCF,         0,              0,
        JR,             C,              OFFSET,         &Z80CPU::simJR_C,        0,              0,
        ADD,            RX,             RX,             &Z80CPU::simADD_RX_RX,   simHL,          simSP,
        LD,             R,              ATWORD,         &Z80CPU::simLD_R_ATw,    simA,           0,
        DEC,            RX,             0,              &Z80CPU::simDEC_RX,      simSP,          0,
        INC,            R,              0,              &Z80CPU::simINC_R,       simA,           0,
        DEC,            R,              0,              &Z80CPU::simDEC_R,       simA,           0,
        LD,             R,              BYTE,           &Z80CPU::simLD_R_b,      simA,           0,
        CCF,            0,              0,              &Z80CPU::simCCF,         0,              0,
// 40-4F
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simB,           simB,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simB,           simC,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simB,           simD,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simB,           simE,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simB,           simH,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simB,           simL,
        LD,             R,              ATRX,           &Z80CPU::simLD_R_ATRX,   simB,           simHL,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simB,           simA,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simC,           simB,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simC,           simC,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simC,           simD,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simC,           simE,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simC,           simH,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simC,           simL,
        LD,             R,              ATRX,           &Z80CPU::simLD_R_ATRX,   simC,           simHL,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simC,           simA,
// 50-5F
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simD,           simB,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simD,           simC,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simD,           simD,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simD,           simE,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simD,           simH,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simD,           simL,
        LD,             R,              ATRX,           &Z80CPU::simLD_R_ATRX,   simD,           simHL,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simD,           simA,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simE,           simB,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simE,           simC,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simE,           simD,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simE,           simE,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simE,           simH,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simE,           simL,
        LD,             R,              ATRX,           &Z80CPU::simLD_R_ATRX,   simE,           simHL,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simE,           simA,
// 60-6F
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simH,           simB,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simH,           simC,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simH,           simD,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simH,           simE,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simH,           simH,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simH,           simL,
        LD,             R,              ATRX,           &Z80CPU::simLD_R_ATRX,   simH,           simHL,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simH,           simA,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simL,           simB,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simL,           simC,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simL,           simD,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simL,           simE,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simL,           simH,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simL,           simL,
        LD,             R,              ATRX,           &Z80CPU::simLD_R_ATRX,   simL,           simHL,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simL,           simA,
// 70-7F
        LD,             ATRX,           R,              &Z80CPU::simLD_ATRX_R,   simHL,          simB,
        LD,             ATRX,           R,              &Z80CPU::simLD_ATRX_R,   simHL,          simC,
        LD,             ATRX,           R,              &Z80CPU::simLD_ATRX_R,   simHL,          simD,
        LD,             ATRX,           R,              &Z80CPU::simLD_ATRX_R,   simHL,          simE,
        LD,             ATRX,           R,              &Z80CPU::simLD_ATRX_R,   simHL,          simH,
        LD,             ATRX,           R,              &Z80CPU::simLD_ATRX_R,   simHL,          simL,
        HALT,           0,              0,              &Z80CPU::simHALT,        0,              0,
        LD,             ATRX,           R,              &Z80CPU::simLD_ATRX_R,   simHL,          simA,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simA,           simB,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simA,           simC,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simA,           simD,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simA,           simE,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simA,           simH,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simA,           simL,
        LD,             R,              ATRX,           &Z80CPU::simLD_R_ATRX,   simA,           simHL,
        LD,             R,              R,              &Z80CPU::simLD_R_R,      simA,           simA,
// 80-8F
        ADD,            R,              R,              &Z80CPU::simADD_R_R,     simA,           simB,
        ADD,            R,              R,              &Z80CPU::simADD_R_R,     simA,           simC,
        ADD,            R,              R,              &Z80CPU::simADD_R_R,     simA,           simD,
        ADD,            R,              R,              &Z80CPU::simADD_R_R,     simA,           simE,
        ADD,            R,              R,              &Z80CPU::simADD_R_R,     simA,           simH,
        ADD,            R,              R,              &Z80CPU::simADD_R_R,     simA,           simL,
        ADD,            R,              ATRX,           &Z80CPU::simADD_R_ATRX,  simA,           simHL,
        ADD,            R,              R,              &Z80CPU::simADD_R_R,     simA,           simA,
        ADC,            R,              R,              &Z80CPU::simADC_R_R,     simA,           simB,
        ADC,            R,              R,              &Z80CPU::simADC_R_R,     simA,           simC,
        ADC,            R,              R,              &Z80CPU::simADC_R_R,     simA,           simD,
        ADC,            R,              R,              &Z80CPU::simADC_R_R,     simA,           simE,
        ADC,            R,              R,              &Z80CPU::simADC_R_R,     simA,           simH,
        ADC,            R,              R,              &Z80CPU::simADC_R_R,     simA,           simL,
        ADC,            R,              ATRX,           &Z80CPU::simADC_R_ATRX,  simA,           simHL,
        ADC,            R,              R,              &Z80CPU::simADC_R_R,     simA,           simA,
// 90-9F
        SUB,            R,              0,              &Z80CPU::simSUB_R,       simB,           0,
        SUB,            R,              0,              &Z80CPU::simSUB_R,       simC,           0,
        SUB,            R,              0,              &Z80CPU::simSUB_R,       simD,           0,
        SUB,            R,              0,              &Z80CPU::simSUB_R,       simE,           0,
        SUB,            R,              0,              &Z80CPU::simSUB_R,       simH,           0,
        SUB,            R,              0,              &Z80CPU::simSUB_R,       simL,           0,
        SUB,            ATRX,           0,              &Z80CPU::simSUB_ATRX,    simHL,          0,
        SUB,            R,              0,              &Z80CPU::simSUB_R,       simA,           0,
        SBC,            R,              R,              &Z80CPU::simSBC_R_R,     simA,           simB,
        SBC,            R,              R,              &Z80CPU::simSBC_R_R,     simA,           simC,
        SBC,            R,              R,              &Z80CPU::simSBC_R_R,     simA,           simD,
        SBC,            R,              R,              &Z80CPU::simSBC_R_R,     simA,           simE,
        SBC,            R,              R,              &Z80CPU::simSBC_R_R,     simA,           simH,
        SBC,            R,              R,              &Z80CPU::simSBC_R_R,     simA,           simL,
        SBC,            R,              ATRX,           &Z80CPU::simSBC_R_ATRX,  simA,           simHL,
        SBC,            R,              R,              &Z80CPU::simSBC_R_R,     simA,           simA,
// A0-AF
        AND,            R,              0,              &Z80CPU::simAND_R,       simB,           0,
        AND,            R,              0,              &Z80CPU::simAND_R,       simC,           0,
        AND,            R,              0,              &Z80CPU::simAND_R,       simD,           0,
        AND,            R,              0,              &Z80CPU::simAND_R,       simE,           0,
        AND,            R,              0,              &Z80CPU::simAND_R,       simH,           0,
        AND,            R,              0,              &Z80CPU::simAND_R,       simL,           0,
        AND,            ATRX,           0,              &Z80CPU::simAND_ATRX,    simHL,          0,
        AND,            R,              0,              &Z80CPU::simAND_R,       simA,           0,
        XOR,            R,              0,              &Z80CPU::simXOR_R,       simB,           0,
        XOR,            R,              0,              &Z80CPU::simXOR_R,       simC,           0,
        XOR,            R,              0,              &Z80CPU::simXOR_R,       simD,           0,
        XOR,            R,              0,              &Z80CPU::simXOR_R,       simE,           0,
        XOR,            R,              0,              &Z80CPU::simXOR_R,       simH,           0,
        XOR,            R,              0,              &Z80CPU::simXOR_R,       simL,           0,
        XOR,            ATRX,           0,              &Z80CPU::simXOR_ATRX,    simHL,          0,
        XOR,            R,              0,              &Z80CPU::simXOR_R,       simA,           0,
// B0-BF
        OR,             R,              0,              &Z80CPU::simOR_R,        simB,           0,
        OR,             R,              0,              &Z80CPU::simOR_R,        simC,           0,
        OR,             R,              0,              &Z80CPU::simOR_R,        simD,           0,
        OR,             R,              0,              &Z80CPU::simOR_R,        simE,           0,
        OR,             R,              0,              &Z80CPU::simOR_R,        simH,           0,
        OR,             R,              0,              &Z80CPU::simOR_R,        simL,           0,
        OR,             ATRX,           0,              &Z80CPU::simOR_ATRX,     simHL,          0,
        OR,             R,              0,              &Z80CPU::simOR_R,        simA,           0,
        CP,             R,              0,              &Z80CPU::simCP_R,        simB,           0,
        CP,             R,              0,              &Z80CPU::simCP_R,        simC,           0,
        CP,             R,              0,              &Z80CPU::simCP_R,        simD,           0,
        CP,             R,              0,              &Z80CPU::simCP_R,        simE,           0,
        CP,             R,              0,              &Z80CPU::simCP_R,        simH,           0,
        CP,             R,              0,              &Z80CPU::simCP_R,        simL,           0,
        CP,             ATRX,           0,              &Z80CPU::simCP_ATRX,     simHL,          0,
        CP,             R,              0,              &Z80CPU::simCP_R,        simA,           0,
// C0-CF
        RET,            NZ,             0,              &Z80CPU::simRET_NZ,      0,              0,
        POP,            RX,             0,              &Z80CPU::simPOP_RX,      simBC,          0,
        JP,             NZ,             WORD,           &Z80CPU::simJP_NZ,       0,              0,
        JP,             WORD,           0,              &Z80CPU::simJP,          0,              0,
        CALL,           NZ,             WORD,           &Z80CPU::simCALL_NZ,     0,              0,
        PUSH,           RX,             0,              &Z80CPU::simPUSH_RX,     simBC,          0,
        ADD,            R,              BYTE,           &Z80CPU::simADD_R_b,     simA,           0,
        RST,            DIRECT,         0,              &Z80CPU::simRST,         0x00,           0,
        RET,            Z,              0,              &Z80CPU::simRET_Z,       0,              0,
        RET,            0,              0,              &Z80CPU::simRET,         0,              0,
        JP,             Z,              WORD,           &Z80CPU::simJP_Z,        0,              0,
        DEFB,           DIRECT,         BYTE,           &Z80CPU::simCB,          0xCB,           0,
        CALL,           Z,              WORD,           &Z80CPU::simCALL_Z,      0,              0,
        CALL,           WORD,           0,              &Z80CPU::simCALL,        0,              0,
        ADC,            R,              BYTE,           &Z80CPU::simADC_R_b,     simA,           0,
        RST,            DIRECT,         0,              &Z80CPU::simRST,         0x08,           0,
// D0-DF
        RET,            NC,             0,              &Z80CPU::simRET_NC,      0,              0,
        POP,            RX,             0,              &Z80CPU::simPOP_RX,      simDE,          0,
        JP,             NC,             WORD,           &Z80CPU::simJP_NC,       0,              0,
        OUT,            ATBYTE,         R,              &Z80CPU::simOUT_ATb_R,   0,              simA,
        CALL,           NC,             WORD,           &Z80CPU::simCALL_NC,     0,              0,
        PUSH,           RX,             0,              &Z80CPU::simPUSH_RX,     simDE,          0,
        SUB,            BYTE,           0,              &Z80CPU::simSUB_b,       0,              0,
        RST,            DIRECT,         0,              &Z80CPU::simRST,         0x10,           0,
        RET,            C,              0,              &Z80CPU::simRET_C,       0,              0,
        EXX,            0,              0,              &Z80CPU::simEXX,         0,              0,
        JP,             C,              WORD,           &Z80CPU::simJP_C,        0,              0,
        IN,             R,              ATBYTE,         &Z80CPU::simIN_R_ATb,    simA,           0,
        CALL,           C,              WORD,           &Z80CPU::simCALL_C,      0,              0,
        DEFB,           DIRECT,         0,              &Z80CPU::simDD,          0xDD,           0,
        SBC,            R,              BYTE,           &Z80CPU::simSBC_R_b,     simA,           0,
        RST,            DIRECT,         0,              &Z80CPU::simRST,         0x18,           0,
// E0-EF
        RET,            PO,             0,              &Z80CPU::simRET_PO,      0,              0,
        POP,            RX,             0,              &Z80CPU::simPOP_RX,      simHL,          0,
        JP,             PO,             WORD,           &Z80CPU::simJP_PO,       0,              0,
        EX,             ATRX,           RX,             &Z80CPU::simEX_ATRX_RX,  simSP,          simHL,
        CALL,           PO,             WORD,           &Z80CPU::simCALL_PO,     0,              0,
        PUSH,           RX,             0,              &Z80CPU::simPUSH_RX,     simHL,          0,
        AND,            BYTE,           0,              &Z80CPU::simAND_b,       0,              0,
        RST,            DIRECT,         0,              &Z80CPU::simRST,         0x20,           0,
        RET,            PE,             0,              &Z80CPU::simRET_PE,      0,              0,
        JP,             ATRX,           0,              &Z80CPU::simJP_ATRX,     simHL,          0,
        JP,             PE,             WORD,           &Z80CPU::simJP_PE,       0,              0,
        EX,             RX,             RX,             &Z80CPU::simEX_RX_RX,    simDE,          simHL,
        CALL,           PE,             WORD,           &Z80CPU::simCALL_PE,     0,              0,
        DEFB,           DIRECT,         BYTE,           &Z80CPU::simED,          0xED,           0,
        XOR,            BYTE,           0,              &Z80CPU::simXOR_b,       0,              0,
        RST,            DIRECT,         0,              &Z80CPU::simRST,         0x28,           0,
// F0-FF
        RET,            P,              0,              &Z80CPU::simRET_P,       0,              0,
        POP,            RX,             0,              &Z80CPU::simPOP_RX,      simAF,          0,
        JP,             P,              WORD,           &Z80CPU::simJP_P,        0,              0,
        DI,             0,              0,              &Z80CPU::simDI,          0,              0,
        CALL,           P,              WORD,           &Z80CPU::simCALL_P,      0,              0,
        PUSH,           RX,             0,              &Z80CPU::simPUSH_RX,     simAF,          0,
        OR,             BYTE,           0,              &Z80CPU::simOR_b,        0,              0,
        RST,            DIRECT,         0,              &Z80CPU::simRST,         0x30,           0,
        RET,            M,              0,              &Z80CPU::simRET_M,       0,              0,
        LD,             RX,             RX,             &Z80CPU::simLD_RX_RX,    simSP,          simHL,
        JP,             M,              WORD,           &Z80CPU::simJP_M,        0,              0,
        EI,             0,              0,              &Z80CPU::simEI,          0,              0,
        CALL,           M,              WORD,           &Z80CPU::simCALL_M,      0,              0,
        DEFB,           DIRECT,         0,              &Z80CPU::simFD,          0xFD,           0,
        CP,             BYTE,           0,              &Z80CPU::simCP_b,        0,              0,
        RST,            DIRECT,         0,              &Z80CPU::simRST,         0x38,           0,
// CB 00-0F
        RLC,            R,              0,              &Z80CPU::simRLC_R,       simB,           0,
        RLC,            R,              0,              &Z80CPU::simRLC_R,       simC,           0,
        RLC,            R,              0,              &Z80CPU::simRLC_R,       simD,           0,
        RLC,            R,              0,              &Z80CPU::simRLC_R,       simE,           0,
        RLC,            R,              0,              &Z80CPU::simRLC_R,       simH,           0,
        RLC,            R,              0,              &Z80CPU::simRLC_R,       simL,           0,
        RLC,            ATRX,           0,              &Z80CPU::simRLC_ATRX,    simHL,          0,
        RLC,            R,              0,              &Z80CPU::simRLC_R,       simA,           0,
        RRC,            R,              0,              &Z80CPU::simRRC_R,       simB,           0,
        RRC,            R,              0,              &Z80CPU::simRRC_R,       simC,           0,
        RRC,            R,              0,              &Z80CPU::simRRC_R,       simD,           0,
        RRC,            R,              0,              &Z80CPU::simRRC_R,       simE,           0,
        RRC,            R,              0,              &Z80CPU::simRRC_R,       simH,           0,
        RRC,            R,              0,              &Z80CPU::simRRC_R,       simL,           0,
        RRC,            ATRX,           0,              &Z80CPU::simRRC_ATRX,    simHL,          0,
        RRC,            R,              0,              &Z80CPU::simRRC_R,       simA,           0,
// CB 10-1F
        RL,             R,              0,              &Z80CPU::simRL_R,        simB,           0,
        RL,             R,              0,              &Z80CPU::simRL_R,        simC,           0,
        RL,             R,              0,              &Z80CPU::simRL_R,        simD,           0,
        RL,             R,              0,              &Z80CPU::simRL_R,        simE,           0,
        RL,             R,              0,              &Z80CPU::simRL_R,        simH,           0,
        RL,             R,              0,              &Z80CPU::simRL_R,        simL,           0,
        RL,             ATRX,           0,              &Z80CPU::simRL_ATRX,     simHL,          0,
        RL,             R,              0,              &Z80CPU::simRL_R,        simA,           0,
        RR,             R,              0,              &Z80CPU::simRR_R,        simB,           0,
        RR,             R,              0,              &Z80CPU::simRR_R,        simC,           0,
        RR,             R,              0,              &Z80CPU::simRR_R,        simD,           0,
        RR,             R,              0,              &Z80CPU::simRR_R,        simE,           0,
        RR,             R,              0,              &Z80CPU::simRR_R,        simH,           0,
        RR,             R,              0,              &Z80CPU::simRR_R,        simL,           0,
        RR,             ATRX,           0,              &Z80CPU::simRR_ATRX,     simHL,          0,
        RR,             R,              0,              &Z80CPU::simRR_R,        simA,           0,
// CB 20-2F
        SLA,            R,              0,              &Z80CPU::simSLA_R,       simB,           0,
        SLA,            R,              0,              &Z80CPU::simSLA_R,       simC,           0,
        SLA,            R,              0,              &Z80CPU::simSLA_R,       simD,           0,
        SLA,            R,              0,              &Z80CPU::simSLA_R,       simE,           0,
        SLA,            R,              0,              &Z80CPU::simSLA_R,       simH,           0,
        SLA,            R,              0,              &Z80CPU::simSLA_R,       simL,           0,
        SLA,            ATRX,           0,              &Z80CPU::simSLA_ATRX,    simHL,          0,
        SLA,            R,              0,              &Z80CPU::simSLA_R,       simA,           0,
        SRA,            R,              0,              &Z80CPU::simSRA_R,       simB,           0,
        SRA,            R,              0,              &Z80CPU::simSRA_R,       simC,           0,
        SRA,            R,              0,              &Z80CPU::simSRA_R,       simD,           0,
        SRA,            R,              0,              &Z80CPU::simSRA_R,       simE,           0,
        SRA,            R,              0,              &Z80CPU::simSRA_R,       simH,           0,
        SRA,            R,              0,              &Z80CPU::simSRA_R,       simL,           0,
        SRA,            ATRX,           0,              &Z80CPU::simSRA_ATRX,    simHL,          0,
        SRA,            R,              0,              &Z80CPU::simSRA_R,       simA,           0,
// CB 30-3F
        SLL,            R,              0,              &Z80CPU::simSLL_R,       simB,           0,
        SLL,            R,              0,              &Z80CPU::simSLL_R,       simC,           0,
        SLL,            R,              0,              &Z80CPU::simSLL_R,       simD,           0,
        SLL,            R,              0,              &Z80CPU::simSLL_R,       simE,           0,
        SLL,            R,              0,              &Z80CPU::simSLL_R,       simH,           0,
        SLL,            R,              0,              &Z80CPU::simSLL_R,       simL,           0,
        SLL,            ATRX,           0,              &Z80CPU::simSLL_ATRX,    simHL,          0,
        SLL,            R,              0,              &Z80CPU::simSLL_R,       simA,           0,
        SRL,            R,              0,              &Z80CPU::simSRL_R,       simB,           0,
        SRL,            R,              0,              &Z80CPU::simSRL_R,       simC,           0,
        SRL,            R,              0,              &Z80CPU::simSRL_R,       simD,           0,
        SRL,            R,              0,              &Z80CPU::simSRL_R,       simE,           0,
        SRL,            R,              0,              &Z80CPU::simSRL_R,       simH,           0,
        SRL,            R,              0,              &Z80CPU::simSRL_R,       simL,           0,
        SRL,            ATRX,           0,              &Z80CPU::simSRL_ATRX,    simHL,          0,
        SRL,            R,              0,              &Z80CPU::simSRL_R,       simA,           0,
// CB 40-4F
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     0,              simB,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     0,              simC,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     0,              simD,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     0,              simE,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     0,              simH,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     0,              simL,
        BIT,            DIRECT,         ATRX,           &Z80CPU::simBIT_n_ATRX,  0,              simHL,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     0,              simA,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     1,              simB,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     1,              simC,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     1,              simD,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     1,              simE,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     1,              simH,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     1,              simL,
        BIT,            DIRECT,         ATRX,           &Z80CPU::simBIT_n_ATRX,  1,              simHL,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     1,              simA,
// CB 50-5F
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     2,              simB,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     2,              simC,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     2,              simD,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     2,              simE,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     2,              simH,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     2,              simL,
        BIT,            DIRECT,         ATRX,           &Z80CPU::simBIT_n_ATRX,  2,              simHL,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     2,              simA,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     3,              simB,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     3,              simC,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     3,              simD,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     3,              simE,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     3,              simH,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     3,              simL,
        BIT,            DIRECT,         ATRX,           &Z80CPU::simBIT_n_ATRX,  3,              simHL,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     3,              simA,
// CB 60-6F
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     4,              simB,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     4,              simC,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     4,              simD,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     4,              simE,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     4,              simH,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     4,              simL,
        BIT,            DIRECT,         ATRX,           &Z80CPU::simBIT_n_ATRX,  4,              simHL,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     4,              simA,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     5,              simB,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     5,              simC,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     5,              simD,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     5,              simE,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     5,              simH,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     5,              simL,
        BIT,            DIRECT,         ATRX,           &Z80CPU::simBIT_n_ATRX,  5,              simHL,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     5,              simA,
// CB 70-7F
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     6,              simB,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     6,              simC,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     6,              simD,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     6,              simE,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     6,              simH,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     6,              simL,
        BIT,            DIRECT,         ATRX,           &Z80CPU::simBIT_n_ATRX,  6,              simHL,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     6,              simA,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     7,              simB,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     7,              simC,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     7,              simD,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     7,              simE,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     7,              simH,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     7,              simL,
        BIT,            DIRECT,         ATRX,           &Z80CPU::simBIT_n_ATRX,  7,              simHL,
        BIT,            DIRECT,         R,              &Z80CPU::simBIT_n_R,     7,              simA,
// CB 80-8F
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     0,              simB,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     0,              simC,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     0,              simD,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     0,              simE,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     0,              simH,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     0,              simL,
        RES,            DIRECT,         ATRX,           &Z80CPU::simRES_n_ATRX,  0,              simHL,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     0,              simA,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     1,              simB,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     1,              simC,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     1,              simD,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     1,              simE,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     1,              simH,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     1,              simL,
        RES,            DIRECT,         ATRX,           &Z80CPU::simRES_n_ATRX,  1,              simHL,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     1,              simA,
// CB 90-9F
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     2,              simB,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     2,              simC,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     2,              simD,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     2,              simE,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     2,              simH,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     2,              simL,
        RES,            DIRECT,         ATRX,           &Z80CPU::simRES_n_ATRX,  2,              simHL,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     2,              simA,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     3,              simB,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     3,              simC,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     3,              simD,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     3,              simE,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     3,              simH,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     3,              simL,
        RES,            DIRECT,         ATRX,           &Z80CPU::simRES_n_ATRX,  3,              simHL,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     3,              simA,
// CB A0-AF
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     4,              simB,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     4,              simC,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     4,              simD,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     4,              simE,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     4,              simH,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     4,              simL,
        RES,            DIRECT,         ATRX,           &Z80CPU::simRES_n_ATRX,  4,              simHL,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     4,              simA,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     5,              simB,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     5,              simC,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     5,              simD,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     5,              simE,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     5,              simH,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     5,              simL,
        RES,            DIRECT,         ATRX,           &Z80CPU::simRES_n_ATRX,  5,              simHL,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     5,              simA,
// CB B0-BF
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     6,              simB,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     6,              simC,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     6,              simD,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     6,              simE,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     6,              simH,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     6,              simL,
        RES,            DIRECT,         ATRX,           &Z80CPU::simRES_n_ATRX,  6,              simHL,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     6,              simA,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     7,              simB,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     7,              simC,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     7,              simD,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     7,              simE,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     7,              simH,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     7,              simL,
        RES,            DIRECT,         ATRX,           &Z80CPU::simRES_n_ATRX,  7,              simHL,
        RES,            DIRECT,         R,              &Z80CPU::simRES_n_R,     7,              simA,
// CB C0-CF
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     0,              simB,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     0,              simC,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     0,              simD,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     0,              simE,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     0,              simH,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     0,              simL,
        SET,            DIRECT,         ATRX,           &Z80CPU::simSET_n_ATRX,  0,              simHL,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     0,              simA,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     1,              simB,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     1,              simC,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     1,              simD,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     1,              simE,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     1,              simH,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     1,              simL,
        SET,            DIRECT,         ATRX,           &Z80CPU::simSET_n_ATRX,  1,              simHL,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     1,              simA,
// CB D0-DF
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     2,              simB,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     2,              simC,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     2,              simD,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     2,              simE,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     2,              simH,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     2,              simL,
        SET,            DIRECT,         ATRX,           &Z80CPU::simSET_n_ATRX,  2,              simHL,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     2,              simA,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     3,              simB,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     3,              simC,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     3,              simD,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     3,              simE,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     3,              simH,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     3,              simL,
        SET,            DIRECT,         ATRX,           &Z80CPU::simSET_n_ATRX,  3,              simHL,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     3,              simA,
// CB E0-EF
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     4,              simB,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     4,              simC,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     4,              simD,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     4,              simE,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     4,              simH,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     4,              simL,
        SET,            DIRECT,         ATRX,           &Z80CPU::simSET_n_ATRX,  4,              simHL,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     4,              simA,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     5,              simB,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     5,              simC,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     5,              simD,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     5,              simE,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     5,              simH,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     5,              simL,
        SET,            DIRECT,         ATRX,           &Z80CPU::simSET_n_ATRX,  5,              simHL,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     5,              simA,
// CB F0-FF
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     6,              simB,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     6,              simC,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     6,              simD,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     6,              simE,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     6,              simH,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     6,              simL,
        SET,            DIRECT,         ATRX,           &Z80CPU::simSET_n_ATRX,  6,              simHL,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     6,              simA,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     7,              simB,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     7,              simC,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     7,              simD,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     7,              simE,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     7,              simH,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     7,              simL,
        SET,            DIRECT,         ATRX,           &Z80CPU::simSET_n_ATRX,  7,              simHL,
        SET,            DIRECT,         R,              &Z80CPU::simSET_n_R,     7,              simA,
// ED 00-0F
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x00,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x01,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x02,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x03,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x04,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x05,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x06,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x07,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x08,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x09,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x0a,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x0b,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x0c,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x0d,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x0e,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x0f,
// ED 10-1F
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x10,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x11,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x12,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x13,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x14,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x15,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x16,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x17,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x18,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x19,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x1a,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x1b,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x1c,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x1d,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x1e,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x1f,
// ED 20-2F
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x20,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x21,
        DIV16,          0,         		0,         		&Z80CPU::simDIV16,       0,           	0,
        DIV32,          0,         		0,         		&Z80CPU::simDIV32,       0,           	0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x24,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x25,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x26,
        IDIV32,         0,         		0,         		&Z80CPU::simIDIV32,      0,           	0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x28,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x29,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x2a,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x2b,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x2c,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x2d,
        SSPD,           R,         		0,         		&Z80CPU::simSSPD,        simA,           0,
        GSPD,           R,         		0,         		&Z80CPU::simGSPD,        simA,           0,
// ED 30-3F
        OPEN,           0,         		0,         		&Z80CPU::simOPEN,        0,           	0,
        CLOSE,          0,         		0,         		&Z80CPU::simCLOSE,       0,           	0,
        READ,           0,         		0,         		&Z80CPU::simREAD,        0,           	0,
        WRITE,          0,         		0,         		&Z80CPU::simWRITE,       0,           	0,
		SEEK,           RX,				ATRX,			&Z80CPU::simSEEK,        simDE,			simHL,
        ERROR,          0,				0,				&Z80CPU::simERROR,       0,				0,
        FINDFIRST,      RX,         	ATRX,         	&Z80CPU::simFINDFIRST,   simDE,          simHL,
        FINDNEXT,       RX,         	ATRX,         	&Z80CPU::simFINDNEXT,    simDE,          simHL,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x38,
        LOAD,           RX,         	RX,         	&Z80CPU::simLOAD,        simBC,          simHL,
        CHDIR,      	0,         		0,         		&Z80CPU::simCHDIR,      	0,           	0,
        GETDIR,      	0,         		0,         		&Z80CPU::simGETDIR,      0,           	0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x3c,
        SEEK,           RX,				ATRX,			&Z80CPU::simSEEK,        simDE,			simHL,
        PATHOPEN,       RX,         	ATRX,         	&Z80CPU::simPATHOPEN,    simDE,          simHL,
        CLOSEALL,       RX,         	0,         		&Z80CPU::simCLOSEALL,    simDE,          0,
// ED 40-4F
        IN,				R,				ATR,			&Z80CPU::simIN_R_ATR,    simB,           simC,
		OUT,           	ATR,         	R,         		&Z80CPU::simOUT_ATR_R,   simC,           simB,
        SBC,            RX,             RX,             &Z80CPU::simSBC_RX_RX,   simHL,          simBC,
        LD,             ATWORD,         RX,             &Z80CPU::simLD_ATw_RX,   0,              simBC,
        NEG,            0,              0,              &Z80CPU::simNEG,         0,              0,
        RETN,           0,         		0,         		&Z80CPU::simRETN,        0,           	0,
        IM,				0,				0,				&Z80CPU::simIM,			0,				0,
        LD,           	R,         		R,         		&Z80CPU::simLD_R_R,      simI,           simA,
        IN,				R,				ATR,			&Z80CPU::simIN_R_ATR,    simC,           simC,
		OUT,			ATR,			R,				&Z80CPU::simOUT_ATR_R,   simC,           simC,
        ADC,           	RX,         	RX,         	&Z80CPU::simADC_RX_RX,   simHL,          simBC,
        LD,             RX,             ATWORD,         &Z80CPU::simLD_RX_ATw,   simBC,          0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x4c,
		RETI,           0,				0,				&Z80CPU::simRETI,        0,				0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x4e,
		LD,				R,				R,				&Z80CPU::simLD_R_R,      simR,           simA,	// TODO: special version of LD
// ED 50-5F
		IN,				R,				ATR,			&Z80CPU::simIN_R_ATR,    simD,           simC,
        OUT,           	ATR,         	R,         		&Z80CPU::simOUT_ATR_R,   simC,           simD,
        SBC,            RX,             RX,             &Z80CPU::simSBC_RX_RX,   simHL,          simDE,
        LD,             ATWORD,         RX,             &Z80CPU::simLD_ATw_RX,   0,              simDE,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x54,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x55,
        IM,             DIRECT,         0,              &Z80CPU::simIM,          1,              0,
        LD,           	R,         		R,         		&Z80CPU::simLDS_A_R,     simA,           simI,
        IN,				R,				ATR,			&Z80CPU::simIN_R_ATR,    simE,           simC,
		OUT,			ATR,			R,				&Z80CPU::simOUT_ATR_R,   simC,           simE,
        ADC,            RX,             RX,             &Z80CPU::simADC_RX_RX,   simHL,          simDE,
        LD,             RX,             ATWORD,         &Z80CPU::simLD_RX_ATw,   simDE,          0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x5c,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x5d,
        IM,				DIRECT,         0,				&Z80CPU::simIM,			2,				0,
        LD,             R,              R,              &Z80CPU::simLDS_A_R,     simA,           simR,
// ED 60-6F
		IN,				R,				ATR,			&Z80CPU::simIN_R_ATR,    simH,           simC,
        OUT,            ATR,            R,              &Z80CPU::simOUT_ATR_R,   simC,           simH,
        SBC,            RX,             RX,             &Z80CPU::simSBC_RX_RX,   simHL,          simHL,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x63,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x64,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x65,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x66,
        RRD,           	0,         		0,         		&Z80CPU::simRRD,        	0,           	0,
		IN,				R,				ATR,			&Z80CPU::simIN_R_ATR,	simL,           simC,
        OUT,            ATR,            R,              &Z80CPU::simOUT_ATR_R,   simC,           simL,
        ADC,           	RX,         	RX,         	&Z80CPU::simADC_RX_RX,   simHL,          simHL,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x6b,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x6c,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x6d,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x6e,
        RLD,           	0,         		0,         		&Z80CPU::simRLD,        	0,           	0,
// ED 70-7F
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x70,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x71,
		SBC,			RX,				RX,				&Z80CPU::simSBC_RX_RX,	simHL,          simSP,
        LD,             ATWORD,         RX,             &Z80CPU::simLD_ATw_RX,   0,              simSP,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x74,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x75,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x76,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x77,
		IN,				R,				ATR,			&Z80CPU::simIN_R_ATR,    simA,           simC,
        OUT,           	ATR,         	R,         		&Z80CPU::simOUT_ATR_R,   simC,           simA,
        ADC,			RX,				RX,				&Z80CPU::simADC_RX_RX,   simHL,          simSP,
        LD,             RX,             ATWORD,         &Z80CPU::simLD_RX_ATw,   simSP,          0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x7c,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x7d,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x7e,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x7f,
// ED 80-8F
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x80,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x81,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x82,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x83,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x84,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x85,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x86,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x87,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x88,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x89,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x8a,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x8b,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x8c,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x8d,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x8e,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x8f,
// ED 90-9F
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x90,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x91,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x92,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x93,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x94,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x95,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x96,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x97,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x98,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x99,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x9a,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x9b,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x9c,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x9d,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x9e,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0x9f,
// ED A0-AF
        LDI,           	0,         		0,         		&Z80CPU::simLDI,        	0,           	0,
        CPI,           	0,         		0,         		&Z80CPU::simCPI,        	0,           	0,
        INI,           	0,         		0,         		&Z80CPU::simINI,        	0,           	0,
        OUTI,           0,         		0,         		&Z80CPU::simOUTI,       	0,           	0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xa4,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xa5,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xa6,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xa7,
        LDD,            0,              0,              &Z80CPU::simLDD,         0,              0,
        CPD,           	0,         		0,         		&Z80CPU::simCPD,        	0,           	0,
        IND,           	0,         		0,         		&Z80CPU::simIND,        	0,           	0,
        OUTD,           0,         		0,         		&Z80CPU::simOUTD,       	0,           	0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xac,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xad,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xae,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xaf,
// ED B0-BF
        LDIR,           0,              0,              &Z80CPU::simLDIR,        0,              0,
        CPIR,           0,         		0,         		&Z80CPU::simCPIR,        0,           	0,
        INIR,           0,				0,				&Z80CPU::simINIR,        0,				0,
        OTIR,           0,				0,				&Z80CPU::simOTIR,        0,				0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xb4,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xb5,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xb6,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xb7,
        LDDR,           0,              0,              &Z80CPU::simLDDR,        0,              0,
        CPDR,           0,         		0,         		&Z80CPU::simCPDR,        0,           	0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xba,		//INDR
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xbb,		//OTDR
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xbc,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xbd,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xbe,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xbf,
// ED C0-CF
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xc0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xc1,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xc2,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xc3,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xc4,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xc5,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xc6,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xc7,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xc8,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xc9,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xca,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xcb,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xcc,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xcd,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xce,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xcf,
// ED D0-DF
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xd0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xd1,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xd2,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xd3,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xd4,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xd5,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xd6,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xd7,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xd8,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xd9,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xda,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xdb,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xdc,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xdd,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xde,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xdf,
// ED E0-EF
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xe0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xe1,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xe2,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xe3,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xe4,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xe5,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xe6,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xe7,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xe8,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xe9,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xea,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xeb,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xec,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xed,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xee,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xef,
// ED F0-FF
        CSINB,          0,              0,              &Z80CPU::simCSINB,       0,              0,
        CSOUTC,         0,              0,              &Z80CPU::simCSOUTC,      0,              0,
        CSEND,          0,              0,              &Z80CPU::simCSEND,       0,              0,
        KBINC,          0,              0,              &Z80CPU::simKBINC,       0,              0,
        KBWAIT,         0,              0,              &Z80CPU::simKBWAIT,      0,              0,
        BREAK,          0,         		0,         		&Z80CPU::simBREAK,       0,           	0,
        EXIT,           0,         		0,         		&Z80CPU::simEXIT,        0,           	0,
        PUTCH,          0,				0,				&Z80CPU::simPUTCH,       0,				0,
        EIBRKON,        0,				0,				&Z80CPU::simEIBRKON,     0,				0,
        EIBRKOFF,       0,				0,				&Z80CPU::simEIBRKOFF,    0,				0,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xfa,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xfb,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xfc,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xfd,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xfe,
        DEFB,           DIRECT,         DIRECT,         &Z80CPU::simHALT,        0xED,           0xff,
// END
        0,              0,              0,              0,		         0,              0
};

