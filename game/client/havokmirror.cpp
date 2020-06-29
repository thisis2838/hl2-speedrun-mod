#include "cbase.h"
#include "HavokMirror.h"

using namespace vgui;


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//.vmt location + filename
#define IMAGE_MIRROR "mirror/mirror"

static ConVar drawHavokPos("sr_draw_havok_pos", "0");

static void checkCheats(IConVar* var, const char* pOldValue, float fOldValue) {
	drawHavokPos.SetValue(1);
	if (!var) return;
	if (!((ConVar*)var)->GetString()) return;
	int toCheck = ((ConVar*)var)->GetInt();
	if (toCheck == (int)fOldValue) return;

}
static ConVar drawMirror("sr_draw_havok_viewport", "0", FCVAR_CLIENTDLL | FCVAR_REPLICATED | FCVAR_CHEAT, "(Cheat Protected) Draws a rear view mirror. 0 = off, 1 = on.", checkCheats);
extern CHud gHUD;

DECLARE_HUDELEMENT(CHavokMirror);

bool CHavokMirror::shouldDrawHavokMirror()
{
	return drawMirror.GetBool();
}

bool CHavokMirror::shouldDrawHavokPos()
{
	return drawHavokPos.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHavokMirror::CHavokMirror(const char* pElementName) : CHudElement(pElementName), vgui::Panel(NULL, "HavokMirror")
{
	vgui::Panel* pParent = g_pClientMode->GetViewport();
	SetParent(pParent);	//Our parent is the screen itself


	m_Mirror = new ImagePanel(this, "RearViewMirror");
	m_Mirror->SetImage(IMAGE_MIRROR);

	SetPaintBackgroundEnabled(false);
}

bool CHavokMirror::ShouldDraw() {

	return CHudElement::ShouldDraw() && shouldDrawHavokMirror();
}



//-----------------------------------------------------------------------------
// Purpose: Paint
//-----------------------------------------------------------------------------
void CHavokMirror::Paint()
{
	//Set position Top Right corner
	SetPos(ScreenWidth() - 300, 60);

	//Set Mirror to 256x128 pixels
	m_Mirror->SetSize(256, 144);

	//DevMsg("thing %i \n", scrwidth / 2);
	m_Mirror->SetVisible(true);
}