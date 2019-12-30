#ifndef _ADAFRUIT_GFX_DECO_H
#define _ADAFRUIT_GFX_DECO_H

#if ARDUINO >= 100
 #include "Arduino.h"
 #include "Print.h"
#else
 #include "WProgram.h"
#endif

#include "Adafruit_GFX.h"

/**
 *  Base decorator for Adafruit_GFX
 */
class Adafruit_GFX_Deco : public Adafruit_GFX {

 protected:
  Adafruit_GFX& gfx_;

 public:

  Adafruit_GFX_Deco( Adafruit_GFX& gfx ) :// Constructor
    Adafruit_GFX( gfx.width(), gfx.height() ), gfx_( gfx )
  {
  }

  // This MUST be defined by the subclass:
  void drawPixel(int16_t x, int16_t y, uint16_t color)
  {
    gfx_.drawPixel( x, y, color );
  }

  // TRANSACTION API / CORE DRAW API
  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  virtual void startWrite(void)
  {
    gfx_.startWrite();
  }
  
  virtual void writePixel(int16_t x, int16_t y, uint16_t color)
  {
    gfx_.writePixel( x, y, color );
  }
  
  virtual void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
  {
    gfx_.writeFillRect( x, y, w, h, color );
  }
  
  virtual void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
  {
    gfx_.writeFastVLine( x, y, h, color );
  }
  
  virtual void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
  {
    gfx_.writeFastHLine( x, y, w, color );
  }
  
  virtual void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
  {
    gfx_.writeLine( x0, y0, x1, y1, color );
  }
  
  virtual void endWrite(void)
  {
    gfx_.endWrite();
  }
  

  // BASIC DRAW API
  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
  {
    gfx_.drawLine( x0, y0, x1, y1, color );
  }
  
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
  {
    gfx_.drawFastVLine( x, y, h, color );
  }
  
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
  {
    gfx_.drawFastHLine( x, y, w, color );
  }
  
  void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
  {
    gfx_.drawRect( x, y, w, h, color );
  }
  
   void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
  {
    gfx_.fillRect( x, y, w, h, color );
  }
  
  void fillScreen(uint16_t color)
  {
    gfx_.fillScreen( color );
  }
  
  // CONTROL API
  // These MAY be overridden by the subclass to provide device-specific
  // optimized code.  Otherwise 'generic' versions are used.
  void invertDisplay(boolean i)
  {
    gfx_.invertDisplay( i );
  }

#if ARDUINO >= 100
  virtual size_t write(uint8_t c)
  {
    return gfx_.write( c );
  }
#else
  virtual void   write(uint8_t c)
  {
    gfx_.write( c );
  }
#endif

  int16_t height(void) const
  {
    return gfx_.height();
  }

  int16_t width(void) const
  {
    return gfx_.width();
  }

  uint8_t getRotation(void) const
  {
    return gfx_.getRotation();
  }

};

#endif // _ADAFRUIT_GFX_H

