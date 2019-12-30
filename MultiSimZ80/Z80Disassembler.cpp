#include "Z80Disassembler.h"
#include "Z80CPU.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
//#include <io.h>
#include <search.h>

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

//  Mnemonics for disassembler
static char mnemo[][9] = {
        "NOP", "LD", "INC", "DEC", "ADD", "SUB", "ADC", "SBC", "AND", "OR", "XOR", "RLCA", 
        "RRCA", "RLA", "RRA", "EX", "EXX", "DJNZ", "JR", "JP", "CALL", "RET", "RST", "CPL", "NEG", "SCF", "CCF", 
        "CP", "IN", "OUT", "PUSH", "POP", "HALT", "DI", "EI", "DAA", "RLD", "RRD", 
        "RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL", "BIT", "RES", "SET", 
        "LDI", "LDD", "LDIR", "LDDR", "CPI", "CPIR", "CPD", "CPDR", 
        "INI", "INIR", "IND", "INDR", "OUTI", "OTIR", "OUTD", "OTDR", "IM", "RETI", "RETN", 
        "$BREAK", "$EXIT", "$CSINB", "$CSOUTC", "$CSEND", "$KBINC", "$KBWAIT", "$PUTCH", "$EIBON", "$EIBOFF", 
        "#DIV16", "#DIV32", "#IDIV32", 
		"#OPEN", "#CLOSE", "#READ", "#WRITE", "#SEEK C", 
		"#GSPD", "#SSPD", "#ERROR", "#FFST BC", "#FNXT BC", "#LOAD", 
        "#CHDIR", "#GDIR", "#PATHOPN", "#CLOSALL", 
        "DEFB"
        };

//  Enumerated constants for operands, also array subscripts
enum {R=1, RX, BYTE, WORD, OFFSET, ATR, ATRX, AFP, 
      Z, C, NZ, NC, PE, PO, P, M, ATBYTE, ATWORD, DIRECT};


enum {simA=1, simB, simC, simD, simE, simH, simL, simI, simR, simBC, simDE, simHL, simAF, simSP, simIR};

static char regnames[][3] = { "??", 
    "A", "B", "C", "D", "E", "H", "L", "I", "R", "BC", "DE", "HL", "AF", "SP", "IR"
    } ;

//  return hex-string or label for double-byte x( dasm )
const char* Z80Disassembler::getxaddr( uint x )
{
	static char addr[41];

	if ( symbols_ )
	{
		symbol_t *sym = symbols_->getSymbol( 'C', x );
		if ( sym )
		{
			strcpy( addr, sym->name );
			return addr;
		}
	}
	
	if ( x > 0x9FFF )
		sprintf( addr, "%05Xh", x );
	else
		sprintf( addr, "%04Xh", x );

	return addr;
}


//  return hex-string or label for DOS SVC x( dasm )
const char* Z80Disassembler::getsvc( uint x )
{
	static char addr[41];

	if ( symbols_ )
	{
		symbol_t *sym = symbols_->getSymbol( 'S', x );
		if ( sym )
		{
			strcpy( addr, sym->name );
			return addr;
		}
	}

	if ( x > 0x9F )
		sprintf( addr, "%03XH", x );
	else
		sprintf( addr, "%02XH", x );

	return addr;
}


// fetch long external address and return it as hex string or as label
const char* Z80Disassembler::getladdr()
{
	uint x;
	x = fetch();
	x += fetch() << 8;
	return getxaddr( x );
}

// fetch short relative external address and return it as hex string or as label
const char* Z80Disassembler::getsaddr()
{
	uint x;
	signed char d;
	d = (signed char)fetch();
	x = pc_ + d;
	return getxaddr( x );
}

// Get nth opcode( 1st or 2nd )
__inline int getopn( int opcode, int pos )
{ 
	return pos==1? Z80CPU::instr[opcode].opn1 : Z80CPU::instr[opcode].opn2 ; 
}

// Get nth argument( 1st or 2nd )
__inline int getarg( int opcode, int pos )
{ 
	return pos==1? Z80CPU::instr[opcode].arg1 : Z80CPU::instr[opcode].arg2 ; 
}

// return operand name or value
const char* Z80Disassembler::getoperand( int opcode, int pos )
{
	static char op[41];
	uint x;

	strcpy( op, "??" );

	switch ( getopn( opcode, pos ) )
	{
	case 0:
		return NULL;
	case R:
	case RX:
		return regnames[getarg( opcode, pos )] ;
	case WORD:
		return getladdr();
	case BYTE:
		x = fetch();
		if ( opcode == 0x3E && code_->read( pc_ ) == 0xEF )
		{
			// if	LD		A, @svc
			//		RST		28H
			// then get SVC label
			return getsvc( x );
		}
		if ( x>0x9F )
			sprintf( op, "%03Xh", x );
		else
			sprintf( op, "%02Xh", x );
		break;
	case ATR:
	case ATRX:
		strcpy( op, "(" );
		strcat( op, regnames[getarg( opcode, pos )] );
		strcat( op, ")" );
		break;
	case ATWORD:
		strcpy( op, "(" );
		strcat( op, getladdr() );
		strcat( op, ")" );
		break;
	case ATBYTE:
		x = fetch();
		if ( x>0x9F )
			sprintf( op, "(%03Xh)", x );
		else
			sprintf( op, "(%02Xh)", x );
		break;
	case OFFSET:
		return getsaddr();
	case DIRECT:
		sprintf( op, "%03Xh", getarg( opcode, pos ) );
		break;
	case Z:
		return "Z";
	case NZ:
		return "NZ";
	case C:
		return "C";
	case NC:
		return "NC";
	case PE:
		return "PE";
	case PO:
		return "PO";
	case P:
		return "P";
	case M:
		return "M";
	case AFP:
		return "AF'";
	}
	return op;
}

// get single instruction source
const char* Z80Disassembler::source()
{
	int opcode;
	static char src[80];
	char substr[41];
	int i;
	const char *op;
	signed char offset;

	useix = useiy = false;

	opcode = fetch();
	if ( opcode == 0xCB )
		opcode = 0x100 | fetch();
	else if ( opcode == 0xED )
		opcode = 0x200 | fetch();
	else if ( opcode == 0xDD )
	{
		useix = true;
		opcode = fetch();
	}
	else if ( opcode == 0xFD )
	{
		useiy = true;
		opcode = fetch();
	}

	if ( useix || useiy )
	{
		if ( opcode==0xE9 )
		{
			offset = 0;
		} 
		else if ( Z80CPU::instr[opcode].opn1 == ATRX || Z80CPU::instr[opcode].opn2 == ATRX )
		{
			offset = (signed char)fetch();
		} 
		else if ( opcode == 0xCB )
		{
			offset = (signed char)fetch();
			opcode = 0x100 | fetch();
		}
	}

	strcpy( src, mnemo[Z80CPU::instr[opcode].mnemon] );

	for ( i = (int)strlen( src ); i < 8; i++ )
	{
		src[i] = ' ';
	} /* endfor */

	src[i] = '\0';

	op = getoperand1( opcode );
	if ( op != NULL )
	{
		if ( ( useix || useiy ) && Z80CPU::instr[opcode].arg1 == simHL )
		{
			if ( Z80CPU::instr[opcode].opn1 == RX )
			{
				op = useix ? "IX" : "IY" ;
			} 
			else if ( Z80CPU::instr[opcode].opn1 == ATRX )
			{
				sprintf( substr, "(%s%+d)", useix ? "IX" : "IY", offset );
				op = substr ;
			}
		}
		
		strcat( src, op );
		op = getoperand2( opcode );
		
		if ( op != NULL )
		{
			strcat( src, "," );
			if ( ( useix || useiy ) && Z80CPU::instr[opcode].arg2 == simHL )
			{
				if ( Z80CPU::instr[opcode].opn2 == RX )
				{
					op = useix ? "IX" : "IY" ;
				} 
				else if ( Z80CPU::instr[opcode].opn2 == ATRX )
				{
					sprintf( substr, "(%s%+d)", useix ? "IX" : "IY", offset );
					op = substr ;
				}
			}
			strcat( src, op );
		}
	}

	for ( i = (int)strlen( src ); i < 32 ; i++ )
	{
		src[i] = ' ';
	}

	src[i] = '\0';

	return src;
}
