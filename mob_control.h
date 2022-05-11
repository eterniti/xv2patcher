#pragma once

#include "Utils.h"
#include "xv2patcher.h"

extern "C"
{
	
std::string GetPlayerAllies();
std::string GetMobName(int idx);
void TakeControlOfMob(int idx);

void OnDeletedMob_Control(Battle_Mob *);
	
}