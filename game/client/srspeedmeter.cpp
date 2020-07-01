#include "cbase.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "iclientvehicle.h"

#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "vphysics_interface.h"
#include "c_prop_vehicle.h"

#include "shared.h"

using namespace vgui;

//Credit CZF!

static ConVar sr_speedometer("sr_speedometer", "1", (FCVAR_CLIENTDLL | FCVAR_ARCHIVE), "Turn the speedometer on/off");
static ConVar sr_speedometer_hvel("sr_speedometer_hvel", "0", (FCVAR_CLIENTDLL | FCVAR_ARCHIVE), "If set to 1, doesn't take the vertical velocity component into account.");
static ConVar sr_speedometer_vehicle_actual_speed("sr_speedometer_vehicle_actual_speed", "0", (FCVAR_CLIENTDLL | FCVAR_ARCHIVE), "If set to 1, prints the actual speed of the vehicle based on their internal calculations (using eye positions).");

class CHudSpeedMeter : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE(CHudSpeedMeter, CHudNumericDisplay);

public:
	CHudSpeedMeter(const char* pElementName);
	virtual void Init()
	{
		Reset();
	}
	virtual void VidInit()
	{
		Reset();
	}
	virtual void Reset()
	{
		SetLabelText(L"UPS");
		SetDisplayValue(0);
	}
	virtual bool ShouldDraw()
	{
		return sr_speedometer.GetBool() && CHudElement::ShouldDraw();
	}
	virtual void OnThink();
};

DECLARE_HUDELEMENT(CHudSpeedMeter);

CHudSpeedMeter::CHudSpeedMeter(const char* pElementName) :
	CHudElement(pElementName), CHudNumericDisplay(NULL, "HudSpeedMeter")
{
	SetParent(g_pClientMode->GetViewport());
	SetHiddenBits(HIDEHUD_PLAYERDEAD);
}

void CHudSpeedMeter::OnThink()
{
	Vector velocity(0, 0, 0);
	C_BasePlayer* player = C_BasePlayer::GetLocalPlayer();
	if (player) {
		if (player->IsInAVehicle()) 
		{
			C_PropVehicleDriveable* veh = (C_PropVehicleDriveable*)player->GetVehicle();
			if (veh) 
			{
				if (sr_speedometer_vehicle_actual_speed.GetBool())
				{
					velocity = realvehiclespd;
				}
				else
					velocity = veh->GetSpeed();
			}
		}
		else 
		{
			velocity = player->GetLocalVelocity();
		}

		// Remove the vertical component if necessary
		if (sr_speedometer_hvel.GetBool())
		{
			velocity.z = 0;
		}

		SetDisplayValue((int)velocity.Length());
	}
}