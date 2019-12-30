#include "SdFileSystem_SD.h"

#include "SD.h"

SdFileSystem_SD::SdFileSystem_SD()
	: SdFileSystem( SD )
{
    if ( !SD.begin() )
    {
        Serial.println( "Card Mount Failed -- retrying" );
        SD.end();
        delay( 3000 );
        if ( !SD.begin() )
        {
            Serial.println( "Card Mount Failed" );
            return;
        }
    }

    uint8_t cardType = SD.cardType();

    if ( cardType == CARD_NONE )
    {
        Serial.println( "No SD card attached" );
        return;
    }

    Serial.print( "SD Card Type: " );
    if ( cardType == CARD_MMC )
    {
        Serial.println( "MMC" );
    } 
    else if ( cardType == CARD_SD )
    {
        Serial.println( "SDSC" );
    } 
    else if ( cardType == CARD_SDHC )
    {
        Serial.println( "SDHC" );
    } 
    else 
    {
        Serial.println( "UNKNOWN" );
    }

}

SdFileSystem_SD::~SdFileSystem_SD()
{
	SD.end();
}
