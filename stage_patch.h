#pragma once

extern "C"
{

int GetNumSsStages();
const std::string &GetStagesSlotsData();	
const std::string &GetLocalStagesSlotsData();
const std::string GetStageName(int ssid);
const std::string GetStageImageString(int ssid);
const std::string GetStageIconString(int ssid);
int StageHasBeenSelectedBefore(int ssid);

int GetCurrentStage();
void PrintCurrentStageGates();
bool OpenedGateExist(bool *error);
	
}
