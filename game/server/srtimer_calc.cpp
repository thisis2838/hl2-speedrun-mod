#include "cbase.h"
#include "srtimer_calc.h"

static ConVar sr_timer_end_map("sr_timer_end_map", "");

// quirky dev note: why didnt valve just do this for us?!?!?!

void SpeedrunTimer::SetFormattedCurrentTime()
{
	int hours = (int)(runtime / 3600.0f);
	int minutes = (int)(((runtime / 3600.0f) - hours) * 60.0f);
	int seconds = (int)(((((runtime / 3600.0f) - hours) * 60.0f) - minutes) * 60.0f);
	int millis = (int)(((((((runtime / 3600.0f) - hours) * 60.0f) - minutes) * 60.0f) - seconds) * 10000.0f);

	Q_snprintf(runtimeprint, sizeof(runtimeprint), "%02d:%02d:%02d.%04d",
		hours,//hours
		minutes, //minutes
		seconds,//seconds
		millis);

	Msg("Run End! Time was %s \n", runtimeprint);
	pausedticks = 0;
}

void SpeedrunTimer::Init(float offsetAfterLoad)
{
	startTick = offsetAfterLoad;
	SetLevelLoad(false);
}

void SpeedrunTimer::SetOffsetBefore(float newOff) {
	offsetBefore = newOff;
}

void SpeedrunTimer::Start()
{
	forcestoppausecalc = false;
	totalTicks = 0;
	startTick = gpGlobals->tickcount;
	Msg("Run Start! \n");
	SetRunning(true);
}

void SpeedrunTimer::SetLevelLoad(bool newVal) {
	inLevelLoad = newVal;
}

bool SpeedrunTimer::InLevelLoad() {
	return inLevelLoad;
}

void SpeedrunTimer::Stop()
{
	if (engine->IsPaused())
	{
		totalTicks += pausedtickstemp / 0.015;
		DevMsg("%f %f \n", pausedtickstemp, totalTicks);
	}
	m_bIsRunning = false;
	forcestoppausecalc = true;
	SetOffsetBefore(0.0f);
	SetFormattedCurrentTime();
	countpausedticks = false;
	settimetoggle = true;
	pausedtickstemp = 0;
}

bool SpeedrunTimer::IsRunning()
{
	return m_bIsRunning;
}

void SpeedrunTimer::SetRunning(bool newb)
{
	m_bIsRunning = newb;
}

void SpeedrunTimer::CalcTime()
{
	// checks for if the game is paused to comply with "new" timing method.
	// first check if the game is paused
	gamepaused = engine->IsPaused();

	// monitor if the game was paused or unpaused by getting its previous state
	if (gamepausedtoggle) { prevgamepaused = gamepaused; gamepausedtoggle = false; }
	else { gamepausedtoggle = true; }

	// if we paused, start counting; if not reset the previous pause calculations
	//if (gamepaused == true && prevgamepaused == false) { countpausedticks = true; settimetoggle = true; }
	//else if (gamepaused == false && prevgamepaused == true) { countpausedticks = false; }

	// the way this works is it will mark the time on which the game is paused using Plat_FloatTime()
	// then when the game is unpaused OR loads, stop and compare the time right now to the original time value to get the delta
	// which is then added onto the timer; very HACKHACK apporach but this is the only way with the given toolset.
	if (!forcestoppausecalc)
	{
		if (gamepaused)
		{
			if (settimetoggle) { a = Plat_FloatTime(); settimetoggle = false; }
			else
			{
				pausedtickstemp = Plat_FloatTime() - a;
			}
		}
		else if (pausedtickstemp != 0)
		{
			pausedticks += pausedtickstemp / 0.015;
			DevMsg("Paused measured, added %f seconds to time. \n", pausedtickstemp);
			pausedtickstemp = 0;
			settimetoggle = true;
		}
		else
		{
			settimetoggle = true;
		}
	}

	if (Q_strcmp(gpGlobals->mapname.ToCStr(), sr_timer_end_map.GetString()) == 0)
	{
		sr_timer_end_map.SetValue("");
		Stop();
	}
	else
	{
		curTime = (float)gpGlobals->tickcount;
		totalTicks += curTime - startTick;
		startTick = curTime;
		runtime = totalTicks * 0.015;
	}
	//DevMsg("%f %f %f %f %f\n", pausedticks, curTime, startTick, totalTicks, pausedtickstemp);
	DevMsg("%i %f\n", gamepaused, pausedtickstemp);

}

float SpeedrunTimer::GetCurrentTime()
{
	if (m_bIsRunning) CalcTime();
	return totalTicks;
}

void SpeedrunTimer::DispatchTimeMessage()
{
	CSingleUserRecipientFilter user(UTIL_GetLocalPlayer());
	user.MakeReliable();
	UserMessageBegin(user, "SpeedrunTimer_Time");
	WRITE_FLOAT(GetCurrentTime());
	MessageEnd();
}

float SpeedrunTimer::GetOffsetBefore()
{
	return offsetBefore;
}