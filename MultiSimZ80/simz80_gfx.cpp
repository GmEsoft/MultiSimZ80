#include "simz80_gfx.h"

extern const unsigned char simz80font[] PROGMEM;

#if ARDUINO >= 100
size_t SimZ80_GFX::write(uint8_t c) {
#else
void SimZ80_GFX::write(uint8_t c) {
#endif
  if (c == '\n') {
    cursor_y += textsize_y*charheight;
    cursor_x  = 0;
  } else if (c == '\r') {
    // skip em
  } else if (c == 0x08) {
    cursor_x -= textsize_x*charwidth;
    if ( cursor_x < 0 )
    {
        cursor_y += textsize_y*charheight;
        cursor_x  = 0;
        if ( cursor_y < 0 )
            cursor_y = 0;
    }
  } else {
    drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
    cursor_x += textsize_x*charwidth;
    if (wrap && (cursor_x > (_width - textsize_x*charwidth))) {
      cursor_y += textsize_y*charheight;
      cursor_x = 0;
    }
  }
#if ARDUINO >= 100
  return 1;
#endif
}


// Draw a character
void SimZ80_GFX::drawChar(int16_t x, int16_t y, unsigned char c,
			    uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y) {
  if ( (x >= _width)              || // Clip right
       (y >= _height)             || // Clip bottom
       ((x + charwidth - 1) < 0)  || // Clip left
       ((y + charheight - 1) < 0) || // Clip top
       size_x != 1 || size_y != 1    // char size must == 1
     )
    return;

  startWrite();

  for (int8_t i=0; i<charwidth; i++ ) {
    uint8_t line = pgm_read_byte( simz80font + ( c * charwidth ) + i );
    uint8_t lline = line & 0x80;
    int8_t pix;
    setAddrWindow( x+i, y, 1, charheight );
    for (int8_t j = 0; j<8; j++) {
      pix = line & 1;
      writePixel(pix ? color : bg==color ? 0 : bg);
      
      if ( c<0x80 || c>0xBF || ( j!=2 && j!=5 ) )
        line >>= 1;
    }
    
    if ( c<0x80 || c>0xBF )
      lline = 0;
    
    for (int8_t j = 8; j<charheight; j++) {
      writePixel(lline ? color : bg==color ? 0 : bg);
    }
  }
  endWrite();
}

void SimZ80_GFX::setCursor(int16_t x, int16_t y) {
  Adafruit_GFX::setCursor( x, y );
  gfx_.setCursor( x, y );
}

void SimZ80_GFX::setTextSize(uint8_t s_x, uint8_t s_y) {
  // size forced to 1
  Adafruit_GFX::setTextSize( 1, 1 );
  gfx_.setTextSize( 1, 1 );
}

void SimZ80_GFX::setTextColor(uint16_t c) {
  // For 'transparent' background, we'll set the bg 
  // to the same as fg instead of using a flag
  Adafruit_GFX::setTextColor( c );
  gfx_.setTextColor( c );
}

void SimZ80_GFX::setTextColor(uint16_t c, uint16_t b) {
  Adafruit_GFX::setTextColor( c, b );
  gfx_.setTextColor( c, b );
}

void SimZ80_GFX::setTextWrap(boolean w) {
  Adafruit_GFX::setTextWrap( w );
  gfx_.setTextWrap( w );
}

void SimZ80_GFX::setRotation0(uint8_t x) {
  Adafruit_GFX::setRotation( x );
  gfx_.setRotation( x );
}
