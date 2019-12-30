#include "TRS80Term.h"

#include <ctype.h>

/// Write to TRS-80 terminal
int TRS80Term::putch( int ch )
{
	if ( !term_ )
		return 0;

	switch ( ch )
	{
	case 0x1C:
		gotoXY( 1, 1 );
		break;
	case 0x1F:
		clrScr();
		break;
	case 0x0E:
		setCursorType( 1 );
		break;
	case 0x0F:
		setCursorType( 0 );
		break;
	default:
		ch = term_->putch( ch );
		switch ( ch )
		{
		case 0x08:
			term_->putch( 0x20 );
			term_->putch( 0x08 );
			break;
		case 0x0D:
			term_->putch( 0x0A );
			break;
		}
		break;
	}

	
	return ch;
}

int TRS80Term::kbhit()
{
	return term_ ? term_->kbhit() : 0;
}

int TRS80Term::getch()
{
	int c = term_ ? term_->getch() : 0;

	if		(c>='A' && c<='Z')	c += 0x20;					// Upper case to lower
	else if (c>='a' && c<='z')	c -= 0x20;					// Lower case to upper
	else if (c==25||c==-148)	c = 0, shiftTab_=!shiftTab_;// shift-tab
	else if (c==24||c==127)		c = 0, shiftBack_=!shiftBack_;// shift-back
	else if (c==9 && shiftTab_)	c = 0x19;					// tab
	else if (c==8 && shiftBack_)c = 0x18;					// backspace
	else if (c==8||c==9||c==13)	c = c;						// unmodified BS,TAB,ENTER
	else if (c>=1 && c<=26)		c |= 0x0400;				// Ctrl-Letter
	else if (c==-134)			c = 0x60; 					// pause (shift @)
	else if (c==27)				c = 0x01; 					// break (esc)
	else if (c==-71)			c = 0x1F; 					// clear (home)
	else if (c==-79)			c = 0x16; 					// ctrl-v (end)
	//else if (c==-26)			c = '<';					// '<' (mu sign)
	//else if (c==-100)			c = '>';					// '>' (British pound)
	//else if (c==-43)			c = '\\';					// '\' ('`')
	else if (c==-75)			c = 0x08;					// Left Arrow
	else if (c==-77)			c = 0x09;					// Right Arrow
	else if (c==-72)			c = 0x0B;  					// Up Arrow
	else if (c==-80)			c = 0x0A;					// Down Arrow
	else if (c==-115)			c = 0x18;					// Shift (ctrl) Left Arrow
	else if (c==-116)			c = 0x19;					// Shift (ctrl) Right Arrow
	else if (c==-145)			c = 0x1A;					// Shift (ctrl) Down Arrow
	else if (c==-141)			c = 0x1B;					// Shift (ctrl) Up Arrow
	else if (c==-83)			c = 0x04;					// ctrl-d (delete)
	else if (c==-82)			c = 0x06;					// ctrl-f(insert)
	else if (c==-81)			c = 0x041A;					// (pgdn) -> ctrl-z
	else if (c==-73)			c = 0x041D;					// (pgup) -> ctrl-]
	else if (c==-119)			c = 0x02;					// Shift-Home (Ctrl-Home)
	else if (c==-117)			c = 0x0E;					// Shift-End (Ctrl-End)
	else if (c==-132)			c = 0x1C;					// Block Begin (Ctrl-PgUp)
	else if (c==-118)			c = 0x1E;					// Block End (Ctrl-PgDn)

	return c;
}
