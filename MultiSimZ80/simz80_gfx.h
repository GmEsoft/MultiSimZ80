#ifndef _SINZ80_GFX_H
#define _SIMZ80_GFX_H

#include "TermCfg.h"

#include "Adafruit_GFX_Deco.h"

#if USE_WROVER_KIT_LCD // ESP32 WROVER KIT
    #include "WROVER_KIT_LCD.h"
    #define LCD_DISPLAY WROVER_KIT_LCD
#elif USE_ADAFRUIT_ILI9341 // Arduino Due with Adafruit LCD display
    #include "Adafruit_ILI9341.h"
    #define LCD_DISPLAY Adafruit_ILI9341
#else
    #error LCD_DISPLAY not defined
#endif

#ifdef __AVR__
    #include <avr/io.h>
    #include <avr/pgmspace.h>
#else
    #define PROGMEM
#endif

/**
 *  Base decorator for Adafruit_GFX
 */
class SimZ80_GFX : public Adafruit_GFX_Deco {

 protected:
  LCD_DISPLAY& gfx0_;

 public:
  SimZ80_GFX( LCD_DISPLAY& gfx ) :// Constructor
    Adafruit_GFX_Deco( gfx ), gfx0_( gfx )
  {
  }


#if ARDUINO >= 100
  virtual size_t write(uint8_t);
#else
  virtual void   write(uint8_t);
#endif

  void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color,
      uint16_t bg, uint8_t size_x, uint8_t size_y),
    setCursor(int16_t x, int16_t y),
    setTextColor(uint16_t c),
    setTextColor(uint16_t c, uint16_t bg),
    setTextSize(uint8_t s_x, uint8_t s_y),
    setTextWrap(boolean w),
    setRotation0(uint8_t r);

    // Transaction API not used by GFX
    void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
    {
        gfx0_.setAddrWindow( x, y, w, h);
    }

    using Adafruit_GFX_Deco::writePixel;
    
    void writePixel(uint16_t color)
    {
        gfx0_.writePixel( color );
    }
    

    static const unsigned char charheight = 12;
    static const unsigned char charwidth  = 5;
};

#endif // _ADAFRUIT_GFX_H
