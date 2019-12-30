#include "SPI.h"
#include "Adafruit_GFX.h"

#include "TermCfg.h"
#include "simz80_gfx.h"
#include "ArduTRS80.h"

//------------------------------------------------------------------
#if USE_WROVER_KIT_LCD

#include "WROVER_KIT_LCD.h"
WROVER_KIT_LCD tft0;
SimZ80_GFX tft( tft0 );

#elif USE_ADAFRUIT_ILI9341

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft0( TFT_CS, TFT_DC );
SimZ80_GFX tft( tft0 );

#endif
//------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay( 200 );

    init_tft();
    delay( 200 );

    tft.println( "configuring hw" );
    
    tft.println( "starting emulator" );

    ArduTRS80_setup();
}


void loop(void)
{
    ArduTRS80_loop();
}

void init_tft()
{
    tft0.begin();

    // read diagnostics (optional but can help debug problems)
    uint8_t x = tft0.readcommand8(ILI9341_RDMODE);
    Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
    x = tft0.readcommand8(ILI9341_RDMADCTL);
    Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
    x = tft0.readcommand8(ILI9341_RDPIXFMT);
    Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
    x = tft0.readcommand8(ILI9341_RDIMGFMT);
    Serial.print("Image Format: 0x"); Serial.println(x, HEX);
    x = tft0.readcommand8(ILI9341_RDSELFDIAG);
    Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX);

    tft.setRotation( 1 );
    tft0.setRotation( 1 ); // we have to do this because Adafruit_GFX::setRotation() is not defined as virtual

    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor( 2*tft.charwidth, 0 );
    tft.setTextColor(ILI9341_GREEN);
    tft.println( "** Multi TRS-80 Demo **" );
    tft.println();
}

