#include "SystemClock.h"

#include <time.h>
#include <arduino.h>

#define _AUDIO_ENABLE_ 0

void SystemClock::runCycles(long cycles)
{
	tstates_ += cycles;

	if ( clockSpeed_ )
	{
		while( tstates_ > 0 )
		{
			// Wait 1/100 sec clock change
			if ( clock100_ < 0 )
			{	// 1st call: initialize
				clock100_ = clock() / ( CLOCKS_PER_SEC / throttleFreq_ );
#if _AUDIO_ENABLE_
				tstatesAudio = 0;
#endif
				tstates_ = 0;
			}

			long s = clock100_;
			long timeout = 500L;
			
			while ( s == clock100_  )
			{
				s = clock() / ( CLOCKS_PER_SEC / throttleFreq_ );
				if ( s == clock100_ )
				{
          sleep( 5 );
					timeout -= 5;
					if ( timeout < 0 )
          {
						clock100_ = s = -1;
          }
				}
   	  }
			
			// ds = elapsed hundredths of seconds
			long ds = s - clock100_;
			clock100_ = s;

			while ( ds < 0 )
      {
				ds += 3600L*throttleFreq_; // Add 1 hour
      }

			if ( ds > 10L*throttleFreq_ )
			{
				// avoid lockups after wake-up
				clock100_ = -1;
			}
			else
			{

				tstates_ -= ds * 1000L * clockSpeed_ / throttleFreq_;
#if _AUDIO_ENABLE_
				tstatesAudio += ds * 1000L * clockspeed * ( AudioFreq/1000 ) / ThrottleFreq;
#endif
				// RTCINT counter
				rtcTest_ += ds * rtcRate_;

				// Update wave output
#if _AUDIO_ENABLE_
				outWaveUpdate();
#endif
				if ( rtcTest_ > 0 )
				{
					trigIrq();
					rtcTest_ -= throttleFreq_;
//					Serial.print("<INT RT>");
				}
			}
		}
#if _AUDIO_ENABLE_
		outWaveUpdate();
#endif
	}
	else
	{
		clock100_ = -1;
	}
}
