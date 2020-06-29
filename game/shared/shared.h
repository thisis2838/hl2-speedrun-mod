
// interface between server and client

#ifndef shared_h
#define shared_h
#pragma once

#ifndef shared_api
#define shared_api __declspec(dllimport)
#endif


#include "util_shared.h"

// havok viewport
extern shared_api Vector havokpos;
extern shared_api Vector havokspd;
extern bool shared_api crouched;

// hud timer code
extern float shared_api totalTicks;


#endif