#include "DO.h"

#include "Color.h"
 
using namespace ::Color;

DO::DO(void) : 
	initialized( false ), displayupdated( true ), font( 1 ), fontbanks( -1 ), 
	video4( false ), inverse( false ), colormode( 2 ), trs80model( 0 ), 
	bigDisplay( false ), graphOverlay( 0 ), trueColorMode( false )
{
	init();
}

DO::~DO(void)
{
}

uchar DO::write(ushort addr, uchar data)
{
  displayupdated = true;
  uchar ret = addr<0x1000 ? ( video[addr] = video[addr] = data ) : data;
  displayupdated = true;
  return ret;
}

uchar DO::read(ushort addr)
{
	return addr<0x1000 ? video[addr] : 0xFF;
}

void DO::putVideo( int x, int y, const char* text )
{
	int i = x + y*64;
	while ( char c = *text++ )
	{
		write( i++, c );
	}
}

void DO::init()
{
	if ( initialized )
		return;

    int i;
	for ( i=0; i<0x800; ++i )
		video[i] = ' ';
	for ( ; i<0x1000; ++i )
		video[i] = 0x0F;

//  putVideo( 16, 8, "** Arduino Due TRS-80 Demo **" );
	putVideo( 16, 1, "===========================" );
	putVideo( 16, 2, "=  SimCore Z-80 & MCS-51  =" );
	putVideo( 16, 3, "===========================" );
	initialized = 1;
}

int DO::updateDisplay(void)
{
    int i,j;
  	unsigned short c;
    static unsigned char buffer[24*80*2];
    static const char trans[] =
        "@ABCDEFGHIJKLMNO"
        "PQRSTUVWXYZ\x18\x19\x1A\x1B_"
        " !\"#$%&'()*+,-./"
        "0123456789:;<=>?"
        "@ABCDEFGHIJKLMNO"
        "PQRSTUVWXYZ\x18\x19\x1A\x1B_"
        "@abcdefghijklmno"
        "pqrstuvwxyz{|}~\xB2"
        "\x20\xD9\xC0\xDF\xB5\xBE\xBE\xDF"
        "\xC6\xD4\xD4\xDF\xCD\xDF\xDF\xDF"
        "\xBF\xB5\xDF\xDF\xB8\xDD\xDD\xC9"
        "\xD5\xDD\xDE\xDE\xC9\xDD\xCC\xDF"
        "\xDA\xDA\xC6\xDF\xDC\xDD\xDE\xDF"
        "\xD5\xDF\xDE\xBB\xBB\xBB\xB9\xBB"
        "\xDC\xDC\xDC\x08\xDC\xC8\xC8\x5B"
        "\xDC\xBC\xBC\x5D\xDC\xDC\xDC\xDB"
        "\x20\xD9\xC0\xDF\xB5\xBE\xBE\xDF"
        "\xC6\xD4\xD4\xDF\xCD\xDF\xDF\xDF"
        "\xBF\xB5\xDF\xDF\xB8\xDD\xDD\xC9"
        "\xD5\xDD\xDE\xDE\xC9\xDD\xCC\xDF"
        "\xDA\xDA\xC6\xDF\xDC\xDD\xDE\xDF"
        "\xD5\xDF\xDE\xBB\xBB\xBB\xB9\xBB"
        "\xDC\xDC\xDC\x08\xDC\xC8\xC8\x5B"
        "\xDC\xBC\xBC\x5D\xDC\xDC\xDC\xDB";
    static const char trans4[] =
        "@ABCDEFGHIJKLMNO"
        "PQRSTUVWYZ\x18\x19\x1A\x1B_"
        " !\"#$%&'()*+,-./"
        "0123456789:;<=>?"
        "@ABCDEFGHIJKLMNO"
        "PQRSTUVWXYZ[\\]^_"
        "@abcdefghijklmno"
        "pqrstuvwxyz{|}~\xB2"
        "\x20\xD9\xC0\xDF\xB5\xBE\xBE\xDF"
        "\xC6\xD4\xD4\xDF\xCD\xDF\xDF\xDF"
        "\xBF\xB5\xDF\xDF\xB8\xDD\xDD\xC9"
        "\xD5\xDD\xDE\xDE\xC9\xDD\xCC\xDF"
        "\xDA\xDA\xC6\xDF\xDC\xDD\xDE\xDF"
        "\xD5\xDF\xDE\xBB\xBB\xBB\xB9\xBB"
        "\xDC\xDC\xDC\x08\xDC\xC8\xC8\x5B"
        "\xDC\xBC\xBC\x5D\xDC\xDC\xDC\xDB"
        "@?BCDEFGHIJKLMNO"
        "PQRSTUVWXYZ?\x19?\x1B_"
        "@?bcdefghijklmno"
        "pqrstuvwxyz{|}~\xB2";

	static const unsigned char dispcolors[][5] = 
	{	//	Letters				Digits				Low Symbols			High Symbols		BlockGraphics	
			WHITE,				WHITE,				WHITE,				WHITE,				WHITE,
			LIGHTGREEN,			LIGHTGREEN,			LIGHTGREEN,			LIGHTGREEN,			LIGHTGREEN, 
			WHITE,				LIGHTRED,			LIGHTBLUE,			LIGHTPINK,			LIGHTCYAN,
			YELLOW,				LIGHTRED,			WHITE,				ORANGE,				LIGHTGREEN,
			YELLOW|BK_BLUE,		LIGHTGREEN|BK_BLUE,	WHITE|BK_BLUE,		LIGHTRED|BK_BLUE,	LIGHTCYAN|BK_BLUE,
			BLUE|BK_WHITE,		BLACK|BK_WHITE,		RED|BK_WHITE,		GREEN|BK_WHITE,		MAGENTA|BK_WHITE
	};

    if ( displayupdated ) 
	{
        displayupdated = 0;
		
		if ( video4 )
		{
			for (i=0; i<24; i++) {
				for (j=0; j<80; j++) {
					if ( inverse && ( video[i*80+j] & 0x80 ) )
					{
						c = buffer[2*(i*80+j)] = ( font == 1 ) ? trans[ video[i*80+j] & 0x7F ] :
												 ( font == 4 ) ? trans4[ video[i*80+j] & 0x7F ] :
													video[i*80+j] & 0x7F ;
						if ( trueColorMode )
						{
							c = video[0x800+i*80+j];
							buffer[2*(i*80+j)+1] = ( ( c << 4 ) | ( c >> 4 ) ) & 0xFF;
						}
						else
						{
							c = ( c >= 0xC0 ) ? dispcolors[colormode][3]
								: ( c >= 0x80 ) ? dispcolors[colormode][4]
								: ( c >= 'A' && c <='z' && ( c <= 'Z' || c >= 'a' ) ) ? dispcolors[colormode][0]
								: ( c >= '0' && c <='9' ) ? dispcolors[colormode][1]
								: dispcolors[colormode][2];
							buffer[2*(i*80+j)+1] = ( dispcolors[colormode][4] << 4 | ( ( c ^ dispcolors[colormode][4] ) ) ) & 0xFF;
						}
					}
					else
					{
						c = buffer[2*(i*80+j)] = ( font == 1 ) ? trans[ video[i*80+j] ] :
												( font == 4 ) ? trans4[ video[i*80+j] ] :
												video[i*80+j] ;
						if ( trueColorMode )
						{
							c = video[0x800+i*80+j];
							buffer[2*(i*80+j)+1] = c;
						}
						else
						{
							buffer[2*(i*80+j)+1] 
								= ( c >= 0xC0 ) ? dispcolors[colormode][3]
								: ( c >= 0x80 ) ? dispcolors[colormode][4]
								: ( c >= 'A' && c <='z' && ( c <= 'Z' || c >= 'a' ) ) ? dispcolors[colormode][0]
								: ( c >= '0' && c <='9' ) ? dispcolors[colormode][1]
								: dispcolors[colormode][2];
						}
					}
				}
			}

			if ( getMode() == MODE_RUN )
			{
				setGraphViewport( 1, 1, 80, 24 );
				setScreenSize( 80, 24 );
				setGraphViewport( 1, 1, 80, 24 );
				putText(1,1,80,24,(char*)buffer);
			}
			else
			{
				if ( bigDisplay )
				{
					setGraphViewport( 1, 20, 80, 43 );
					setScreenSize( 132, 43 );
					setGraphViewport( 1, 20, 80, 43 );
					putText(1,20,80,43,(char*)buffer);
					setCursorType(_NOCURSOR);
					textAttr(BLACK);
					gotoXY(132,43);
					clrEol();
				}
				else
				{
					setGraphViewport( 1, 1, 80, 24 );
					setScreenSize( 80, 25 );
					putText(1,1,80,24,(char*)buffer);
					setCursorType(_NOCURSOR);
					textAttr(BLACK);
					gotoXY(80,25);
					clrEol();
				}
			}
		}
		else // !video4
		{
			for (i=0; i<16; i++) {
				for (j=0; j<64; j++) {
					c = buffer[2*(i*64+j)] = ( font == 1 ) ? trans[video[i*64+j]] :
						  					 ( font == 4 ) ? trans4[video[i*64+j]] :
										     ( trs80model != 1 || video[i*64+j] < 192 ) ? video[i*64+j] : ( video[i*64+j] - 64 );
					buffer[2*(i*64+j)+1] = ( c >= 0xC0 ) ? dispcolors[colormode][3]
						: ( c >= 0x80 ) ? dispcolors[colormode][4]
						: ( c >= 'A' && c<='z' && ( c<='Z' || c>='a' ) ) ? dispcolors[colormode][0]
						: ( c >='0' && c <='9' ) ? dispcolors[colormode][1]
						: dispcolors[colormode][2];
				}
			}

			setCursorType( _NOCURSOR );

			if ( getMode() == MODE_RUN )
			{
				setGraphViewport( 1, 1, 64, 16 );
				setScreenSize( 64, 16 );
				putText(1,1,64,16,(char*)buffer);
			}
			else
			{
				if ( bigDisplay )
				{
					setGraphViewport( 9, 24, 72, 39 );
					setScreenSize( 132, 43 );
					setGraphViewport( 9, 24, 72, 39 );
					putText(9,24,72,39,(char*)buffer);
					textAttr(BLACK);
					gotoXY(132,43);
					clrEol();
				}
				else
				{
					setGraphViewport( 9, 10, 72, 25 );
					setScreenSize( 80, 25 );
					putText(9,10,72,25,(char*)buffer);
					textAttr(BLACK);
					gotoXY(80,25);
					clrEol();
				}
			}
		}
        return 1;
    }
    return 0;
}


void DO::setDisplayUpdated( char p_DisplayUpdated )
{
	displayupdated = p_DisplayUpdated;
}

void DO::setColorMode( char p_ColorMode )
{
	colormode = ( p_ColorMode + numcolormodes ) % numcolormodes;
}

char DO::getColorMode()
{
	return colormode ;
}

void DO::setFont( char p_Font )
{
	font = p_Font;
}

void DO::setVideo4( char p_Video4 )
{
	video4 = p_Video4;
}

void DO::setGraphOverlay( char p_GraphOvl )
{
	graphOverlay = p_GraphOvl;
	setGraphOverlay( graphOverlay );
}

void DO::setInverse( char p_Inverse )
{
	inverse = p_Inverse;
}

void DO::setModel( char p_Model )
{
	trs80model = p_Model;
}

void DO::selectFont( char p_Font )
{
	if ( p_Font != fontbanks )
	{
		fontbanks = p_Font;
		selectFontBank( 6, p_Font?6:12 );
		selectFontBank( 7, p_Font?7:13 );
	}
}

void DO::setBigDisplay( int p_BigDisplay )
{
	bigDisplay = p_BigDisplay;
}

int DO::getBigDisplay( )
{
	return bigDisplay;
}

void DO::setTrueColorMode( int color )
{
	if ( trueColorMode != color )
	{
		trueColorMode = color;
		setDisplayUpdated( 1 );
	}
}

