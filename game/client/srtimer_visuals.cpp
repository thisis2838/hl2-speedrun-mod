#include "cbase.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"

using namespace vgui;

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/AnimationController.h>

#include "vgui_helpers.h"

#define BUFSIZE (sizeof("00:00:00.0000")+1)

static ConVar sr_timer("sr_timer", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Turn the timer display on/off");
static ConVar sr_timer_pb_delta_viewlimit("sr_timer_pb_delta_viewlimit", "30", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "When to show the live delta between current time and PB (if set), in seconds.");

class CHudTimer : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE(CHudTimer, Panel);

public:
	CHudTimer(const char* pElementName);
	virtual void Init();
	virtual bool ShouldDraw()
	{
		return sr_timer.GetBool() && CHudElement::ShouldDraw();
	}
	void MsgFunc_SpeedrunTimer_TimeToBeat(bf_read& msg);
	void MsgFunc_SpeedrunTimer_TimeDelta(bf_read& msg);
	void MsgFunc_SpeedrunTimer_Time(bf_read& msg);
	void MsgFunc_SpeedrunTimer_StateChange(bf_read& msg);

	//int getPos(const char*);

	virtual void Paint();

private:
	int initialTall;
	float m_flSecondsRecord;
	float timedelta;
	float m_flSecondsTime;
	float totalTicks;
	wchar_t m_pwCurrentTime[BUFSIZE];
	wchar_t m_pwPBTime[BUFSIZE];
	wchar_t m_pwPBDelta[BUFSIZE+1];
	char m_pszString[BUFSIZE];
	char m_pszStringpb[BUFSIZE];
	char m_pszStringpbdelta[BUFSIZE+1];
	CUtlMap<const char*, float> map;

protected:
	CPanelAnimationVar(float, m_flBlur, "Blur", "0");
	CPanelAnimationVar(Color, m_TextColor, "TextColor", "FgColor");
	CPanelAnimationVar(Color, m_Ammo2Color, "Ammo2Color", "FgColor");

CPanelAnimationVar(HFont, m_hNumberFont, "NumberFont", "HudNumbers");
CPanelAnimationVar(HFont, m_hNumberGlowFont, "NumberGlowFont",
	"HudNumbersGlow");
CPanelAnimationVar(HFont, m_hSmallNumberFont, "SmallNumberFont",
	"HudNumbersSmall");
CPanelAnimationVar(HFont, m_hTextFont, "TextFont", "Default");

CPanelAnimationVarAliasType(float, text_xpos, "text_xpos", "8",
	"proportional_float");
CPanelAnimationVarAliasType(float, text_ypos, "text_ypos", "20",
	"proportional_float");
CPanelAnimationVarAliasType(float, digit_xpos, "digit_xpos", "50",
	"proportional_float");
CPanelAnimationVarAliasType(float, digit_ypos, "digit_ypos", "2",
	"proportional_float");
CPanelAnimationVarAliasType(float, digit2_xpos, "digit2_xpos", "98",
	"proportional_float");
CPanelAnimationVarAliasType(float, digit2_ypos, "digit2_ypos", "16",
	"proportional_float");

const char* SetFormattedCurrentTime(float input, bool removeexcess);
};

DECLARE_HUDELEMENT(CHudTimer);
DECLARE_HUD_MESSAGE(CHudTimer, SpeedrunTimer_TimeToBeat);
DECLARE_HUD_MESSAGE(CHudTimer, SpeedrunTimer_TimeDelta);
DECLARE_HUD_MESSAGE(CHudTimer, SpeedrunTimer_Time);
DECLARE_HUD_MESSAGE(CHudTimer, SpeedrunTimer_StateChange);

CHudTimer::CHudTimer(const char* pElementName) :
	CHudElement(pElementName), Panel(NULL, "HudTimer")
{
	SetParent(g_pClientMode->GetViewport());
}

void CHudTimer::Init()
{
	HOOK_HUD_MESSAGE(CHudTimer, SpeedrunTimer_TimeToBeat);
	HOOK_HUD_MESSAGE(CHudTimer, SpeedrunTimer_TimeDelta);
	HOOK_HUD_MESSAGE(CHudTimer, SpeedrunTimer_Time);
	HOOK_HUD_MESSAGE(CHudTimer, SpeedrunTimer_StateChange);
	initialTall = 48;
	//Reset();
}

void CHudTimer::MsgFunc_SpeedrunTimer_TimeToBeat(bf_read& msg)
{
	m_flSecondsRecord = msg.ReadFloat();
}

void CHudTimer::MsgFunc_SpeedrunTimer_TimeDelta(bf_read& msg)
{
	timedelta = msg.ReadFloat();
}


void CHudTimer::MsgFunc_SpeedrunTimer_Time(bf_read& msg)
{
	totalTicks = msg.ReadFloat();
}

void CHudTimer::MsgFunc_SpeedrunTimer_StateChange(bf_read& msg)
{
	bool started = msg.ReadOneBit();
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;
}

const char* CHudTimer::SetFormattedCurrentTime(float input, bool removeexcess)
{
	int hours = (int)(input / 3600.0f);
	int minutes = (int)(((input / 3600.0f) - hours) * 60.0f);
	int seconds = (int)(((((input / 3600.0f) - hours) * 60.0f) - minutes) * 60.0f);
	int millis = (int)(((((((input / 3600.0f) - hours) * 60.0f) - minutes) * 60.0f) - seconds) * 10000.0f);

	char printout[15];

	if (!removeexcess)
	{
		Q_snprintf(printout, sizeof(printout), "%02d:%02d:%02d.%04d",
			hours,//hours
			minutes, //minutes
			seconds,//seconds
			millis);
	}
	else
	{
		if ((int)hours != 0)
		{
			Q_snprintf(printout, sizeof(printout), "%02d:%02d:%02d.%04d",
				hours,//hours
				minutes, //minutes
				seconds,//seconds
				millis);
		}
		else if ((int)minutes != 0)
		{
			Q_snprintf(printout, 11, "%02d:%02d.%04d",
				minutes, //minutes
				seconds,//seconds
				millis);
		}
		else
		{
			Q_snprintf(printout, 7, "%02d.%04d",
				seconds,//seconds
				millis);
		}
	}

	return printout;

}

void CHudTimer::Paint(void)
{
	Q_strcpy(m_pszString, SetFormattedCurrentTime(totalTicks * 0.015, false));
	Q_strcpy(m_pszStringpb, SetFormattedCurrentTime(m_flSecondsRecord, false));

// msg.ReadString(m_pszString, sizeof(m_pszString));
	g_pVGuiLocalize->ConvertANSIToUnicode(
		m_pszString, m_pwCurrentTime, sizeof(m_pwCurrentTime));

	g_pVGuiLocalize->ConvertANSIToUnicode(
		m_pszStringpb, m_pwPBTime, sizeof(m_pwPBTime));

	// Draw the text label.
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(GetFgColor());
	//current map can be found with:    g_pGameRules->MapName()

	float actual_text_ypos = (m_flSecondsRecord != 0) ? text_ypos : (text_ypos + surface()->GetFontTall(m_hTextFont) / 2);

	surface()->DrawSetTextPos(text_xpos, actual_text_ypos + 3);
	surface()->DrawPrintText(L"TIME", wcslen(L"TIME"));
	// Draw current time.
	surface()->DrawSetTextFont(surface()->GetFontTall(m_hTextFont));
	surface()->DrawSetTextPos(digit_xpos, actual_text_ypos);
	surface()->DrawPrintText(m_pwCurrentTime, wcslen(m_pwCurrentTime));

	if (m_flSecondsRecord != 0)
	{
		float pb_text_ypos = text_ypos + surface()->GetFontTall(m_hTextFont) + 3;
		float timedelta2 = totalTicks * 0.015 - m_flSecondsRecord;

		surface()->DrawSetTextFont(m_hTextFont);

		if (m_flSecondsRecord != 0 && abs(timedelta2) <= sr_timer_pb_delta_viewlimit.GetFloat())
		{
			char* delta = (timedelta2 < 0) ? "-" : "+";
			surface()->DrawSetTextColor((timedelta2 < 0) ? Color(10, 255, 10, 255) : Color(255, 10, 10, 255));

			char deltatime[16];
			char txt[16];
			Q_strcpy(deltatime, SetFormattedCurrentTime(abs(timedelta2), true));
			Q_snprintf(txt, sizeof(txt), "%s%s", delta, deltatime, true);

			g_pVGuiLocalize->ConvertANSIToUnicode(txt, m_pwPBDelta, sizeof(m_pwPBDelta));

			DevMsg("%s\n", txt); 

			surface()->DrawSetTextPos(text_xpos, pb_text_ypos);
			surface()->DrawPrintText(L"DELTA", wcslen(L"DELTA"));

			surface()->DrawSetTextPos(digit_xpos - 2, pb_text_ypos);
			surface()->DrawPrintText(m_pwPBDelta, wcslen(m_pwPBDelta));
		}
		else
		{
			surface()->DrawSetTextColor(GetFgColor());
			surface()->DrawSetTextPos(text_xpos, pb_text_ypos);
			surface()->DrawPrintText(L"PB", wcslen(L"PB"));
			// Draw PB time
			//surface()->DrawSetTextFont(surface()->GetFontTall(m_hTextFont));
			surface()->DrawSetTextPos(digit_xpos - 2, pb_text_ypos + 1);
			surface()->DrawPrintText(m_pwPBTime, wcslen(m_pwPBTime));
		}
	}
}





