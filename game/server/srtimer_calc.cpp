#include "cbase.h"
#include "srtimer_calc.h"
#include <xlocale>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include "../../lib/public/discord_rpc.h"


void SpeedrunTimer::Init(float offsetAfterLoad)
{
	startTick = offsetAfterLoad;
	SetLevelLoad(false);
	DispatchTimeMessage(true);
}

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

// PURPOSE: takes in input time in float seconds and outputs formatted time as 00:00:00.0000 
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

// PURPOSE: sets the Discord Rich Text Presence status and details
void SetDiscordRPC(const char* input1)
{
	char buffer[256];
	char buffer2[256];

	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));

	Q_snprintf(buffer, sizeof(buffer), "%s", input1);

	discordPresence.details = buffer;
	Discord_UpdatePresence(&discordPresence);
}

// PURPOSE: sets the Pb for that current run
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

static void checkoutputfile(IConVar* var, const char* pOldValue, float fOldValue)
{
	if (Q_strcmp(sr_timer_output_filepath.GetString(), "") == 0)
	{
		Msg("You should set the output file path using the sr_timer_output_filepath command so the game can write run information!");
		return;
	}
}

static ConCommand sr_timer_pb("sr_timer_pb", SetTimeToBeat, "Sets the PB for your run\n", FCVAR_ARCHIVE);
static ConVar sr_timer_timing_method("sr_timer_timing_method", "0", 0, "Which method of timing to use, 0 is with pauses, 1 is without pauses\n");
static ConVar sr_timer_run_count("sr_timer_run_count", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN, "Total run count so far \n");
static ConVar sr_timer_run_count_in_session("sr_timer_run_count_in_session", "0", FCVAR_HIDDEN, "Total run count in this session \n");
static ConVar sr_timer_output_enable("sr_timer_output_enable", "0", FCVAR_ARCHIVE, "Should run information be written to disk? \n", checkoutputfile);

auto start = std::chrono::system_clock::now();
auto end = std::chrono::system_clock::now();

char startmethod[100] = "";
char endmethod[100] = "";
char cheatsmsg[100] = "";
char hl1movement[100] = "";

bool checkforcheats;
bool ischeater;
float timeoffirstcheat;

float pbdelta;
char pbmsg[100] = "";

bool hasstopped = false;

bool writedelta = false;

// PURPOSE: Checks if the current run has a custom map ending
bool IsEndMapSet()
{
	return !(Q_strcmp("", sr_timer_end_map.GetString()) == 0);
}

// PURPOSE: Write run information to file
void WriteToFile(const char* input)
{
	if (!sr_timer_output_enable.GetBool())
		return;

	if (Q_strcmp(sr_timer_output_filepath.GetString(), "") == 0)
	{
		Msg("sr_timer_output_filepath is not set! Not writing.");
		return;
	}

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
		Msg("The file %s does not exist! Not writing.\n", sr_timer_output_filepath.GetString());
		return;
	}

	std::ofstream outfile;
	outfile.open(str.c_str(), std::ios_base::app);
	outfile << input;
	Msg("Finished writing run information to %s! \n", sr_timer_output_filepath.GetString());
}

// PURPOSE: Function to compile information about the run to be sent to WriteToFile()
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
	float pausetime = SpeedrunTimer::timer()->grandtotalpausedticks * gpGlobals->interval_per_tick;

	if (CommandLine()->CheckParm("-hl1movement"))
	{
		Q_strcpy(hl1movement, "\nThis run was done with HL1 Movement\n");
	}

	char print[500];
	int totalruncount = sr_timer_run_count.GetInt();
	int sessionruncount = sr_timer_run_count_in_session.GetInt();

	Q_snprintf(print, sizeof(print),
		"\n\n#%i\n-----RUN INFORMATION BEGIN-----\nRun #%i of total count, run #%i of session\nFinal time was %s\n%s\nTotal amount of time spent pausing was %f seconds\n\n%s%s%s%s\nRun started on %s\nand ended on %s\n-----RUN INFORMATION END-----\n\n",
		totalruncount,		// unique identifier for run information in the file
		totalruncount,		// total run count until that point in time
		sessionruncount,	// total run count in that session
		input,				// final time
		pbmsg,				// if the run is a PB, print a pb message and show by how much
		pausetime,			// total amount of time spent pausing
		cheatsmsg,			// if sv_cheats was enabled during the run, show a note of when
		hl1movement,		// if this run was done with HL1 movement mode
		startmethod,		// method of starting the run
		endmethod,			// method of ending the run
		runstart,			// system time when the run started
		runend				// system time when the run ended
	);

	WriteToFile(print);
}


void SpeedrunTimer::SetOffsetBefore(float newOff) {
	offsetBefore = newOff;
}

// PURPOSE: Operations when starting a run
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

bool SpeedrunTimer::InLevelLoad() {
	return inLevelLoad;
}

// PURPOSE: Operations when stopping a run
void SpeedrunTimer::Stop(int method)
{
	if (engine->IsPaused())
	{
		totalTicks += pausedtickstemp / gpGlobals->interval_per_tick;
		runtime = totalTicks * gpGlobals->interval_per_tick;
		grandtotalpausedticks += pausedtickstemp / gpGlobals->interval_per_tick;
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

	switch (method)
	{
	case 0:
		Q_snprintf(endmethod, sizeof(endmethod), "The timer was manually stopped on %s\n", gpGlobals->mapname.ToCStr());
		break;
	case 1:
		Q_snprintf(endmethod, sizeof(endmethod), "The timer was manually set to automatically stop on %s \n", sr_timer_end_map.GetString());
		break;
	case 2:
		Q_snprintf(endmethod, sizeof(endmethod), "The run was automatically stopped by default full-game run end trigger\n");
		break;
	}

	char buffer[256];
	if (pbdelta != 0 && personalbest != 0)
	{
		char printtime[15];
		Q_strcpy(printtime, SetFormattedCurrentTime(abs(pbdelta)));

		if (pbdelta < 0)
		{
			Q_snprintf(pbmsg, sizeof(pbmsg), "The run was %s ahead of PB", printtime);
			Q_snprintf(buffer, sizeof(buffer), "Run PB'd by %s!", printtime);
		}
		else
		{
			Q_snprintf(pbmsg, sizeof(pbmsg), "The run was %s behind of PB", printtime);
			Q_snprintf(buffer, sizeof(buffer), "Run missed PB by %s...", printtime);
		}
	}
	else
	{
		Q_snprintf(buffer, sizeof(buffer), "Run finished at %s!", SetFormattedCurrentTime(totalTicks * gpGlobals->interval_per_tick));
	}

	SetDiscordRPC(buffer);

	end = std::chrono::system_clock::now();

	m_bIsRunning = false;
	forcestoppausecalc = true;
	SetOffsetBefore(0.0f);

	Q_strcpy(runtimeprint, SetFormattedCurrentTime(runtime));

	Msg("------------------------- \n");
	Msg("Run End! Time was %s \n", runtimeprint);
	Msg("Total amount of pause time was %f seconds \n", grandtotalpausedticks * gpGlobals->interval_per_tick);
	Msg("------------------------- \n");

	ConstructRunInformation(runtimeprint);

	pausedticks = 0;
	settimetoggle = true;
	pausedtickstemp = 0;
}


// PURPOSE: Operations for calculating the run and handling custom map endings
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
			pausedticks += pausedtickstemp / gpGlobals->interval_per_tick;
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
		runtime = totalTicks * gpGlobals->interval_per_tick;
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

// PURPOSE: Returns current time
float SpeedrunTimer::GetCurrentTime()
{
	if (m_bIsRunning) CalcTime();
	return totalTicks;
}

// PURPOSE: transfer information to the hud
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

			char buffer[256];
			Q_snprintf(buffer, sizeof(buffer), "In a run! %s", SetFormattedCurrentTime(totalTicks * gpGlobals->interval_per_tick));
			SetDiscordRPC(buffer);
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
			WRITE_BOOL(true);
			MessageEnd();
			writedelta = false;
		}
	}
}

// DEPRECATED / UNUSED

bool SpeedrunTimer::IsRunning()
{
	return m_bIsRunning;
}

void SpeedrunTimer::SetRunning(bool newb)
{
	m_bIsRunning = newb;
}

void SpeedrunTimer::SetLevelLoad(bool newVal) 
{
	inLevelLoad = newVal;
}


float SpeedrunTimer::GetOffsetBefore()
{
	return offsetBefore;
}