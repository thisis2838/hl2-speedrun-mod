#include "cbase.h"
#include "srtimer_calc.h"
#include <xlocale>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>

const char *lowercase(const char* input)
{
	int i;
	char* loweredchar;
	for (i = 0; i < strlen(input); i++)
	{
		loweredchar[i] = tolower(input[i]);
	}
	return loweredchar;

}

const char* SpeedrunTimer::SetFormattedCurrentTime(float input)
{
	int hours = (int)(input / 3600.0f);
	int minutes = (int)(((input / 3600.0f) - hours) * 60.0f);
	int seconds = (int)(((((input / 3600.0f) - hours) * 60.0f) - minutes) * 60.0f);
	int millis = (int)(((((((input / 3600.0f) - hours) * 60.0f) - minutes) * 60.0f) - seconds) * 10000.0f);

	char printout[15];

	Q_snprintf(printout, sizeof(printout), "%02d:%02d:%02d.%04d",
		hours,//hours
		minutes, //minutes
		seconds,//seconds
		millis);

	return printout;
}



static void SetTimeToBeat(const CCommand& args)
{
	if (args.ArgC() < 5)
	{
		Msg("Format: sr_timer_pb <hour> <mins> <secs> <milisecs>\n");
		return;
	}
	else
	{
		SpeedrunTimer::timer()->personalbest = atoi(args[1]) * 3600 + atoi(args[2]) * 60 + atoi(args[3]) + atof(args[4]) * 0.001;
		Msg("Personal best time set to %s! \n", SpeedrunTimer::timer()->SetFormattedCurrentTime(SpeedrunTimer::timer()->personalbest));
	}

	if (gpGlobals->tickcount != 0) //2838: ONLY fire when the game is active, else we run into read access violations
	{
		SpeedrunTimer::timer()->DispatchTimeMessage(true);
	}
}


static ConVar sr_timer_end_map("sr_timer_end_map", "", 0, "Defines a custom end map for your run, leave blank to disable\n");
static ConVar sr_timer_output_filepath("sr_timer_output_filepath", "", FCVAR_ARCHIVE, "Defines the filepath of the file to put run information in.\n");
static ConCommand sr_timer_pb("sr_timer_pb", SetTimeToBeat, "Sets the PB for your run\n", FCVAR_ARCHIVE);
static ConVar sr_timer_timing_method("sr_timer_timing_method", "0", 0, "Which method of timing to use, 0 is with pauses, 1 is without pauses\n");
static ConVar sr_timer_run_count("sr_timer_run_count", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN, "Total run count so far \n");
static ConVar sr_timer_run_count_in_session("sr_timer_run_count_in_session", "0", FCVAR_HIDDEN, "Total run count in this session \n");

auto start = std::chrono::system_clock::now();
auto end = std::chrono::system_clock::now();

char startmethod[100] = "";
char endmethod[100] = "";
char cheatsmsg[100] = "";

bool checkforcheats;
bool ischeater;
float timeoffirstcheat;

float pbdelta;
char pbmsg[100] = "";

bool writedelta = false;

bool IsEndMapSet()
{
	return !(Q_strcmp("", sr_timer_end_map.GetString()) == 0);
}

void WriteToFile(const char* input)
{
	if (sr_timer_output_filepath.GetString() == "")
		return;

	// replace \ with \\ to comply with c++ weird requirements
	std::string str = sr_timer_output_filepath.GetString();
	for (std::string::size_type offs = str.find("\\"); offs != std::string::npos; offs = str.find("\\", offs + 2))
	{
		str.replace(offs, 1, "\\\\");
	}

	std::fstream fs;
	fs.open(str.c_str());
	
	if (fs.fail())
	{
		Msg("The file %s does not exist! Creating one... \n", sr_timer_output_filepath.GetString());
		std::ofstream outfile(str.c_str());
		outfile.close();
	}

	std::ofstream outfile;
	outfile.open(str.c_str(), std::ios_base::app);
	outfile << input;
	Msg("Finished writing run information to %s! \n", sr_timer_output_filepath.GetString());
}

void ConstructRunInformation(const char* input)
{
	std::time_t start_c = std::chrono::system_clock::to_time_t(start);
	const std::tm start_tm = *std::localtime(&start_c);
	char runstart[50];
	strftime(runstart, sizeof(runstart), "%A %c", &start_tm);

	std::time_t end_c = std::chrono::system_clock::to_time_t(end);
	const std::tm end_tm = *std::localtime(&end_c);
	char runend[50];
	strftime(runend, sizeof(runstart), "%A %c", &end_tm);

	const char* runtime = input;
	float pausetime = SpeedrunTimer::timer()->grandtotalpausedticks * 0.015;


	char print[500];
	int totalruncount = sr_timer_run_count.GetInt();
	int sessionruncount = sr_timer_run_count_in_session.GetInt();

	Q_snprintf(print, sizeof(print),
		"\n\n#%i\n-----RUN INFORMATION BEGIN-----\nRun #%i of total count, run #%i of session\nFinal time was %s\n%s\nTotal amount of time spent pausing was %f seconds\n\n%s%s%s\nRun started on %s\nand ended on %s\n-----RUN INFORMATION END-----\n\n",
		totalruncount,
		totalruncount,
		sessionruncount,
		input,
		pbmsg,
		pausetime,
		cheatsmsg,
		startmethod,
		endmethod,
		runstart, 
		runend
	);

	WriteToFile(print);
}

void SpeedrunTimer::Init(float offsetAfterLoad)
{
	startTick = offsetAfterLoad;
	SetLevelLoad(false);
	DispatchTimeMessage(true);
}

void SpeedrunTimer::SetOffsetBefore(float newOff) {
	offsetBefore = newOff;
}

void SpeedrunTimer::Start(int method)
{
	forcestoppausecalc = false;
	totalTicks = 0;
	pausedticks = 0;
	startTick = gpGlobals->tickcount;
	settimetoggle = true;

	ischeater = false;
	checkforcheats = true;

	Msg("------------------------- \n");
	Msg("Run Start! \n");
	Msg("------------------------- \n");

	if (method == 0)
	{
		Q_snprintf(startmethod, sizeof(startmethod), "The timer was manually started on %s\n", gpGlobals->mapname.ToCStr());
	}
	else if (method == 1)
	{
		Q_snprintf(startmethod, sizeof(startmethod), "The timer was automatically started by default full-game run start trigger\n");
	}

	start = std::chrono::system_clock::now();

	SetRunning(true);
	custmapendtrig = false;
	grandtotalpausedticks = 0;
}

void SpeedrunTimer::SetLevelLoad(bool newVal) {
	inLevelLoad = newVal;
}

bool SpeedrunTimer::InLevelLoad() {
	return inLevelLoad;
}

void SpeedrunTimer::Stop(int method)
{
	if (engine->IsPaused())
	{
		totalTicks += pausedtickstemp / 0.015;
		runtime = totalTicks * 0.015;
		grandtotalpausedticks += pausedtickstemp / 0.015;
	}

	if (totalTicks != 0)
	{
		sr_timer_run_count.SetValue(sr_timer_run_count.GetInt() + 1);
		sr_timer_run_count_in_session.SetValue(sr_timer_run_count_in_session.GetInt() + 1);
	}

	if (ischeater)
	{
		char cheaterprinttime[15];
		Q_strcpy(cheaterprinttime, SetFormattedCurrentTime(timeoffirstcheat));
		
		if (timeoffirstcheat == 0)
		{
			Q_snprintf(cheatsmsg, sizeof(cheatsmsg), "!!! sv_cheats was enabled before or at run start !!!\n");
		}
		else
		{
			Q_snprintf(cheatsmsg, sizeof(cheatsmsg), "!!! sv_cheats was enabled at %s !!!\n", cheaterprinttime);
		}
	}


	if (method == 0)
	{
		Q_snprintf(endmethod, sizeof(endmethod), "The timer was manually stopped on %s\n", gpGlobals->mapname.ToCStr());
	}
	if (method == 1)
	{
		Q_snprintf(endmethod, sizeof(endmethod), "The timer was manually set to automatically stop on %s \n", sr_timer_end_map.GetString());
	}
	else if (method == 2)
	{
		Q_snprintf(endmethod, sizeof(endmethod), "The run was automatically stopped by default full-game run end trigger\n");
	}

	if (pbdelta != 0)
	{
		char printtime[15];
		Q_strcpy(printtime, SetFormattedCurrentTime(abs(pbdelta)));

		if (pbdelta < 0)
		{
			Q_snprintf(pbmsg, sizeof(pbmsg), "The run was %s ahead of PB", printtime);
		}
		else
		{
			Q_snprintf(pbmsg, sizeof(pbmsg), "The run was %s behind of PB", printtime);
		}
	}

	end = std::chrono::system_clock::now();

	m_bIsRunning = false;
	forcestoppausecalc = true;
	SetOffsetBefore(0.0f);

	Q_strcpy(runtimeprint, SetFormattedCurrentTime(runtime));

	Msg("------------------------- \n");
	Msg("Run End! Time was %s \n", runtimeprint);
	Msg("Total amount of pause time was %f seconds \n", grandtotalpausedticks * 0.015);
	Msg("------------------------- \n");

	ConstructRunInformation(runtimeprint);

	pausedticks = 0;

	//refresh hud timer so that both are synced
	if (personalbest != 0)
	{
		writedelta = true;
		DispatchTimeMessage(true);
	}
	else
	{
		DispatchTimeMessage(false);
	}


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
	gamepaused = engine->IsPaused();

	if (!forcestoppausecalc && sr_timer_timing_method.GetInt() == 0)
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
			grandtotalpausedticks += pausedticks;
			DevMsg("Paused measured, added %f seconds to time. \n", pausedtickstemp);
			pausedtickstemp = 0;
			settimetoggle = true;
		}
		else
		{
			settimetoggle = true;
		}
	}

	if (Q_strcmp(gpGlobals->mapname.ToCStr(), lowercase(sr_timer_end_map.GetString())) == 0 && IsEndMapSet())
	{
		//whats this?
		//if we call stop() here then it will call dispatchtimemessage() in the middle of its previous call
		//resulting in a exit to desktop. better to queue up the next dispatchtimemessage() call to end the run
		custmapendtrig = true;
	}
	else
	{
		curTime = (float)gpGlobals->tickcount + pausedticks;
		totalTicks += curTime - startTick;
		startTick = curTime;
		runtime = totalTicks * 0.015;
	}

	pbdelta = runtime - personalbest;

	if (sv_cheats->GetBool() && checkforcheats)
	{
		ischeater = true;
		timeoffirstcheat = (gamepaused) ? runtime + pausedtickstemp : runtime;
		checkforcheats = false;
	}

	//DevMsg("%f %f %f %f %f\n", pausedticks, curTime, startTick, totalTicks, pausedtickstemp);
}

float SpeedrunTimer::GetCurrentTime()
{
	if (m_bIsRunning) CalcTime();
	return totalTicks;
}

void SpeedrunTimer::DispatchTimeMessage(bool pb)
{
	if (!pb)
	{
		if (!custmapendtrig)
		{
			CSingleUserRecipientFilter user(UTIL_GetLocalPlayer());
			user.MakeReliable();
			UserMessageBegin(user, "SpeedrunTimer_Time");
			WRITE_FLOAT(GetCurrentTime());
			MessageEnd();
		}
		else
		{
			custmapendtrig = false;
			Stop(1);
			sr_timer_end_map.SetValue("");
		}
	}
	else
	{
		CSingleUserRecipientFilter user(UTIL_GetLocalPlayer());
		user.MakeReliable();
		if (!writedelta)
		{
			UserMessageBegin(user, "SpeedrunTimer_TimeToBeat");
			WRITE_FLOAT(personalbest);
			MessageEnd();
		}
		else
		{
			UserMessageBegin(user, "SpeedrunTimer_TimeDelta");
			WRITE_FLOAT(pbdelta);
			MessageEnd();
			writedelta = false;
		}
	}
}


float SpeedrunTimer::GetOffsetBefore()
{
	return offsetBefore;
}