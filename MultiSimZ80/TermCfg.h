#ifndef _TERMCFG_H
#define _TERMCFG_H

#define USE_DISASS 1
#define USE_MFCTERM 0
#define USE_ARDUCONIO 0
#define USE_MFCTERMCONIO 0
#define USE_FDC 1
#define USE_BORL2INO 0
#define USE_BORL2MS 0
#define USE_MAIN 0
#define USE_TRS80 1
#define USE_TRS80_DO 1
#define USE_TRS80_KI 0
#define USE_PRINTER 0
#define USE_CASS 0
#define USE_AUDIO 0

#define USE_DOS 1
#define USE_WINDOWS 0

#if defined( ESP_PLATFORM ) // ESP32 WROVER KIT (ESP32 Dev Module)

#define USE_PREFS 1
#define USE_MCORE 1
#define USE_DUEFLASH 0
#define USE_WROVER_KIT_LCD 1
#define USE_ADAFRUIT_ILI9341 0
#define USE_SDMMC 1
#define USE_SD 0
#define USE_RES 0
#define USE_RTC 1

#elif defined( __SAM3X8E__ ) // Arduino Due (Programming Port)

#define USE_PREFS 0
#define USE_MCORE 0
#define USE_DUEFLASH 1
#define USE_WROVER_KIT_LCD 0
#define USE_ADAFRUIT_ILI9341 1
#define USE_SDMMC 0
#define USE_SD 0
#define USE_RES 1
#define USE_RTC 0

#else

#error Board not supported

#endif

#endif
