#include "SerialTerm.h"

#include "simz80_gfx.h"

#define USE_SERIALUSB 0

extern SimZ80_GFX tft;

static const char *pinput = "";
static int curx = 0;
static int cury = 0;
static int attr = 0x0F;

static uint16_t colors[] = {
  ILI9341_BLACK,
  ILI9341_NAVY,
  ILI9341_DARKGREEN,
  ILI9341_DARKCYAN,
  ILI9341_MAROON,
  ILI9341_PURPLE,
  ILI9341_OLIVE,
  ILI9341_LIGHTGREY,
  ILI9341_DARKGREY,
  ILI9341_BLUE,
  ILI9341_GREEN,
  ILI9341_CYAN,
  ILI9341_RED,
  ILI9341_MAGENTA,
  ILI9341_YELLOW,
  ILI9341_WHITE         
  };

void SerialTerm::begin()
{
#if USE_SERIALUSB
    SerialUSB.begin(115200);
    delay( 300 );
    SerialUSB.println( "TRS-80 Terminal ready" );
#endif
    Serial.println( "TRS-80 Terminal ready" );
}

void SerialTerm::end()
{
    Serial.println( "TRS-80 Terminal closed" );
#if USE_SERIALUSB
    SerialUSB.println( "TRS-80 Terminal closed" );
    SerialUSB.end();
#endif
}

int SerialTerm::input( const char *input )
{
    pinput = input;
}

int SerialTerm::kbhit()
{
    int c = *pinput;
    
    if ( c )
    {
        ++pinput;
    }
#if USE_SERIALUSB
    else if ( SerialUSB.available() )
    {
        c = SerialUSB.read();
    }
#endif
    
    if ( c )
    {
        //printf( "[%02X]", c );
    }
    return c;
}

int SerialTerm::getch()
{
    int c;
    while ( !(c = kbhit() ) )
    {
        delay(10);
    }
    //putch( c );
    return c;
}

int SerialTerm::ungetch( int ch )
{
    return 0;
}

int SerialTerm::putch( int ch )
{
    if ( ch == 0x0D )
    {
#if USE_SERIALUSB
        SerialUSB.println();
#endif
        tft.println();
    }
    else
    {
#if USE_SERIALUSB
        SerialUSB.print( (char)ch );
#endif
        tft.print( (char)ch );
    }
}

int SerialTerm::puts( const char *str )
{
    Serial.print( str );
#if USE_SERIALUSB
    SerialUSB.print( str );
#endif
    tft.print( str );
    return 0;
}

char *SerialTerm::gets( char *str )
{
    int len = (int)(*str);
    int p = 0;
    int c = getch();

    while ( c != '\r' )
    {
        if ( c >= ' ' && p < len )
        {
            str[2+p] = (char)c;
            ++p;
            putch( c );
        }
        else if ( ( c == 8 || c == 0x7F ) && p > 0 )
        {
            --p;
            puts( "\x08\x20\x08" );
        }
        else
        {
            printf( "[%04X]\x08\x08\x08\x08\x08\x08", c );
        }
        c = getch();
    }

    putch( c );
    str[2+p] = 0;
    str[1] = p;

    return str + 2;
}

int SerialTerm::vprintf( const char *format, va_list args )
{
    char buffer[1000];
    //vsprintf_s( buffer, sizeof( buffer ), format, args );
    vsprintf( buffer, format, args );
    Serial.print( buffer );
    int ret = tft.print( buffer );
#if USE_SERIALUSB
    return SerialUSB.print( buffer );
#else
    return ret;
#endif
}

void SerialTerm::setScreenSize( int x, int y )
{
}

void SerialTerm::setFullScreen( bool fs )
{
}

int SerialTerm::textAttr( const int c )
{
    return 0;
}

int SerialTerm::curAttr()
{
    return 0;
}

int SerialTerm::clrScr()
{
    return 0;
}

int SerialTerm::clrEol()
{
    return 0;
}

int SerialTerm::gotoXY( int x, int y )
{
    tft.setCursor( ( x - 1 ) * tft.charwidth, ( y - 1 ) * tft.charheight );
    curx = x;
    cury = y;    
    return 0;
}

int SerialTerm::whereX()
{
    return curx;
}

int SerialTerm::whereY()
{
    return cury;
}

int SerialTerm::getTextInfo( text_info* ti )
{
    return 0;
}

int SerialTerm::getText( int left, int top, int right, int bottom, char* buffer )
{
    return 0;
}

int SerialTerm::putText( int left, int top, int right, int bottom, const char* buffer )
{
//Serial.println( "Serialerm::putText()" );
    static char screen[16*64*2];
    int bufpos=0;
    for ( int l=top-1; l<bottom; ++l )
    {
        int scrpos = ( l*64 + left - 1 ) * 2;
        for ( int c=left-1; c<right; ++c, bufpos+=2, scrpos+=2 )
        {
            char chr = buffer[bufpos];
            char att = buffer[bufpos+1];
            if  (   screen[scrpos] != chr
                ||  screen[scrpos+1] != att
                ) 
            {
                screen[scrpos] = chr;
                screen[scrpos+1] = att;
//                tft.drawChar( c*tft.charwidth, l*tft.charheight, chr, 0xFFFF, att>>4, 1 );
                tft.drawChar( c*tft.charwidth, l*tft.charheight, chr, colors[att&15], colors[att>>4], 1, 1 );
            }
        }
    }
}

int SerialTerm::setCursorType( int curstype )
{
    return 0;
}

int SerialTerm::setGraphViewport( int x0, int y0, int x1, int y1 )
{
    return 0;
}

void SerialTerm::setGraphOverlay( int p_GraphOverlay )
{
}

int SerialTerm::selectFontBank( unsigned nPage, unsigned nBank )
{
    return 0;
}

int SerialTerm::scanKbd( int row )
{
    return 0;
}

int SerialTerm::setKbdScanMode( int mode )
{
    return 0;
}    

void SerialTerm::setWideMode( bool wide )
{
}
