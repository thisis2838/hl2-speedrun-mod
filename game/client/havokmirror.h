#ifndef havokmirror_h
#define havokmirror_h
#pragma once

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <vgui_controls/ImagePanel.h>

//-----------------------------------------------------------------------------
// Purpose: HUD Mirror panel
//-----------------------------------------------------------------------------
class CHavokMirror : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHavokMirror, vgui::Panel);

public:
	CHavokMirror(const char* pElementName);

	static bool shouldDrawHavokMirror();

	static bool shouldDrawHavokPos();

protected:
	virtual void Paint();
	virtual bool ShouldDraw();

private:
	vgui::ImagePanel* m_Mirror;
};
#endif