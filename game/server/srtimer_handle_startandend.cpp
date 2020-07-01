
// handler for auto start and end

#include "cbase.h"
#include "srtimer_calc.h"

class TimerOperate
{
public:
	static void OperateWithName(const char* name, const char* op);
	static TimerOperate* operate()
	{
		static TimerOperate* operate = new TimerOperate();
		return operate;
	}

};


void TimerOperate::OperateWithName(const char* name, const char* op)
{

	if (Q_strcmp(name, "logic_start_train") == 0 && Q_strcmp(op, "start") == 0)
	{
		SpeedrunTimer::timer()->Start(1);
		return;
	}

	else if (Q_strcmp(name, "Mic_breen_teleport_final") == 0 && Q_strcmp(op, "end") == 0)
	{
		char runtimeprint[15];
		SpeedrunTimer::timer()->Stop(2);
		if (SpeedrunTimer::timer()->runtimeprint)
		{
			Q_strcpy(runtimeprint, SpeedrunTimer::timer()->runtimeprint);
		}
		return;
	}

}
