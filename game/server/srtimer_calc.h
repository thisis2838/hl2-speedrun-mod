#ifndef TIMER_H_
#define TIMER_H_

#include "cbase.h"
#include "fasttimer.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include "platform.h"
#include "saverestoretypes.h"
#include "shared.h"

extern IFileSystem* filesystem;

#ifdef WIN32
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif

class SpeedrunTimer
{
public:

	SpeedrunTimer() : m_bIsRunning(false), m_flSecondsRecord(0.0f)
	{
		inLevelLoad = false;
		curTime = 0.0f;
		totalTicks = 0.0f;
		startTick = 0;
		pausedticks = 0;
		offset = 0.0f;
		offsetBefore = 0.0f;
		startTime = 0.0f;
		m_vStart.Init();
		m_vGoal.Init();
	}
	~SpeedrunTimer() {}


private:
	Vector m_vStart;
	Vector m_vGoal;
	CFastTimer* m_ftTimer;
	CCycleCount m_ccCycles;
	bool inLevelLoad;
	bool m_bIsRunning;
	bool m_IsPaused;
	float m_flSecondsRecord;
	float offset;
	float offsetBefore;
	float curTime;
	float startTime;
	float a;
	bool custmapendtrig;

public:
	float totalTicks;//to be sent to the hud and converted into HH:MM:SS
	float personalbest;
	float grandtotalpausedticks;

private:
	float startTick;//the tick in which the level/reload started

public:
	bool forcestoppausecalc; // just to make sure...
	bool gamepaused;
	bool settimetoggle;
	float pausedticks;
	float pausedtickstemp;

	static SpeedrunTimer* timer()
	{
		static SpeedrunTimer* timer = new SpeedrunTimer();
		return timer;
	}

	float runtime;
	char runtimeprint[15];

	const char* SetFormattedCurrentTime(float input);
	void Init(float offsetAfterLoad);
	void SetOffsetBefore(float newOff);
	void Start(int method);
	void SetLevelLoad(bool newVal);
	bool InLevelLoad();
	void Stop(int method);
	bool IsRunning();
	void SetRunning(bool newb);
	void CalcTime();
	float GetCurrentTime();
	void DispatchTimeMessage(bool pb);
	float GetOffsetBefore();

};


#undef NEWLINE

#endif // TIMER_H_