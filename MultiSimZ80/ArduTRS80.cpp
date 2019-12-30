#include "ArduTRS80.h"

#include "TermCfg.h"
//#include "runtime.h"

#include "Model1.h"
#include "romLEVEL2.h"
#include "ResSCARFMAN.h"
#include "ResMETEOR.h"
#include "ResGALAXY.h"
#include "ResFileSystem.h"
#include "SerialTerm.h"
#include "ArduSleeper.h"

#if USE_FDC
#include "DskImage.h"
#endif

#if USE_SDMMC
#include "SdFileSystem_SDMMC.h"
#endif

#if USE_SD
#include "SdFileSystem_SD.h"
#endif

#if USE_PREFS
#include <Preferences.h>
Preferences preferences;
#endif

#if USE_MCORE
#include <freertos/portable.h>
#endif

#if USE_DUEFLASH
#include "DueFlashStorage.h"
#endif

//#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "simz80_gfx.h"
extern SimZ80_GFX tft;

static const char* commands[] =
    {
        "clock\r",
        
        "cat 1\r",
        
        "dir\r"
        "cosmic\r",
        
        "dir\r"
        "nova\r",
        
        "himem\r"
        "robotd\r",
        
        "dir\r"
        "stellar\r",
        
        "dir\r"
        "defcom\r",
        
        "dir\r"
        "attack\r",
        
        "dir\r"
        "meteor\r",
        
        "dir\r"
        "galaxypl\r",
        
        "\r"
        "?mem\r\r\r",
        
        "\r"
        "?mem\r\r\r"
        "system\r"
        "galaxy\r"
        "GALAXY\r"
        "/\r"
    };

static const char colorModes[] = {
  0,  // DOS
  1,  // cat
  2,  // cosmic
  2,  // nova
  3,  // robot
  2,  // stellar
  2,  // defcom
  2,  // attack
  3,  // meteor
  2,  // galaxypl
  1,  // ?mem
  2   // galaxy
  };

#if USE_SDMMC || USE_SD
SdFileSystem *pSdFileSystem;
#endif

static Model1 vsystem;
static SerialTerm term;
static ArduSleeper sleeper;

#if USE_RES
static ResFileSystem resFileSystem;
#endif

static uint8_t select;

#if USE_DUEFLASH
static DueFlashStorage dueFlashStorage;
#endif

#if USE_FDC
static DskImage dskImages[2];
#endif

static void dir( FileSystem_I &fs )
{
    Dir_I *pDir = fs.openDir( "" );
    if ( !pDir )
    {
        Serial.print( "NO DIR: " );
        Serial.println( fs.strerror( fs.errno_() ) );
        return;
    }
    
    while ( DirEntry entry = pDir->next() )
    {
        Serial.print( " Name: " );
        Serial.print( entry.name );
        if ( entry.isDir )
        {
            Serial.println( " DIR" );
        }
        else
        {
            Serial.print( " size: " );
            Serial.println( entry.size );
        }
    }
    pDir->close();
    delete pDir;
}

CFUNC void ArduTRS80_setup()
{
    SerialTerm::begin();
    vsystem.setTerm( &term );
    vsystem.setSleeper( &sleeper );

#if USE_RES
    addResSCARFMAN();
    addResMETEOR();
    addResGALAXY();

    Serial.println( "Dir of resFileSystem:" );
    dir( resFileSystem );
#endif    

#if USE_SDMMC
    pSdFileSystem = new SdFileSystem_SDMMC();
    Serial.println( "Dir of sdFileSystem:" );
    dir( *pSdFileSystem );
#endif

#if USE_SD
    pSdFileSystem = new SdFileSystem_SD();
    Serial.println( "Dir of sdFileSystem:" );
    dir( *pSdFileSystem );
#endif

#if USE_PREFS
    preferences.begin( "TRS-80", false );
    select = preferences.getUInt( "select", 0 );
#elif USE_DUEFLASH
    select = dueFlashStorage.read( 0 );
#else
  	select = 2;
#endif

    if ( select >= sizeof( commands ) / sizeof( const char * ) )
        select = 0;
    

#if USE_TRS80
    
    int TRS80Model = 1;
    vsystem.init();

#if USE_SDMMC || USE_SD

    if ( Dir_I *pDir = pSdFileSystem->openDir( "" ) )
    {
        vsystem.setFileSystem( pSdFileSystem );
        pDir->close();
        delete pDir;
        Serial.println("using SD file system");
        tft.println("using SD file system");
    }
    else
    {
#if USE_RES
        vsystem.setFileSystem( &resFileSystem );
        Serial.println("using RES file system");
        tft.println("using RES file system");
#endif
    }
    
#else

#if USE_RES
    vsystem.setFileSystem( &resFileSystem );
    Serial.println("using RES file system");
    tft.println("using RES file system");
#endif

#endif

    delay( 1000 );
    tft.setCursor( 0, 16*tft.charheight );

#if USE_MCORE
    int mainCore = xPortGetCoreID();
//    term.printf( "Main task core: %d - ", mainCore );
    xTaskCreatePinnedToCore(
                        ArduTRS80_subloop,  /* Function to implement the task */
                        "subloop",          /* Name of the task */
                        10000,              /* Stack size in words */
                        NULL,               /* Task input parameter */
                        0,                  /* Priority of the task */
                        NULL,               /* Task handle. */
                        1-mainCore);        /* Core where the task should run */
     
    delay( 300 );
    Serial.println("subloop task created...");
#endif

#if USE_FDC
    if ( commands[select][0] != '\r' )
    {
        dskImages[0].set( 0, "BOOT.DSK", 0, false );
        dskImages[0].setFileSystem( pSdFileSystem );
        dskImages[0].open();
        vsystem.setDiskImage( 0, &dskImages[0] );
        dskImages[1].set( 1, "BIG5.DSK", 0, false );
        dskImages[1].setFileSystem( pSdFileSystem );
        dskImages[1].open();
        vsystem.setDiskImage( 1, &dskImages[1] );
        vsystem.getFdc().setEnabled( true );
    }
#endif

    vsystem.getDO()->setColorMode( colorModes[select] );
    term.input( commands[select++] );

#if USE_PREFS
    preferences.putUInt( "select", select );
    preferences.end();
#elif USE_DUEFLASH
    dueFlashStorage.write( 0, select );
#endif

#endif // USE_TRS80
}

CFUNC void ArduTRS80_loop()
{
#if USE_TRS80    
    vsystem.loop();  
#endif
}

CFUNC void ArduTRS80_subloop( void * )
{
#if USE_TRS80 && USE_MCORE
    int subCore = xPortGetCoreID();
    Serial.println("Starting subloop");
//    term.printf( "Sub task core: %d\n", subCore );
    for (;;)
    {
        vsystem.getDO()->updateDisplay();
        delay( 1 );
    }
#endif
}

