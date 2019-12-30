#include "SdFileSystem_SDMMC.h"

#include "SD_MMC.h"

SdFileSystem_SDMMC::SdFileSystem_SDMMC()
	: SdFileSystem( SD_MMC )
{
    if ( !SD_MMC.begin("/sdcard",true) )
    {
        Serial.println( "Card Mount Failed -- retrying" );
        SD_MMC.end();
        delay( 3000 );

        if ( !SD_MMC.begin("/sdcard",true) )
        {
            Serial.println( "Card Mount Failed" );
            return;
        }
    }

    uint8_t cardType = SD_MMC.cardType();

    if ( cardType == CARD_NONE )
    {
        Serial.println( "No SD_MMC card attached" );
        return;
    }

    Serial.print( "SD_MMC Card Type: " );
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

SdFileSystem_SDMMC::~SdFileSystem_SDMMC()
{
	SD_MMC.end();
}
