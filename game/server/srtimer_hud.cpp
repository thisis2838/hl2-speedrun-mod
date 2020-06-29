#include "cbase.h"

#include <tier1\convar.h>
#include "srtimer_calc.h"



void startTimer() {
	SpeedrunTimer::timer()->Start();
}

void stopTimer() {
	SpeedrunTimer::timer()->Stop();
}

ConCommand startTimer_c("sr_timer_start", startTimer, "Starts the timer.", 0);
ConCommand stopTimer_c("sr_timer_stop", stopTimer, "Stops the timer.", 0);
