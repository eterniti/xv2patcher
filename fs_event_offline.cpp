#include "PatchUtils.h"
#include "xv2patcher.h"
#include "QxdFile.h"
#include "debug.h"

//#define CHANGE_MEDALS_ADDRESS 0x699D90

enum
{
	EVENT_NON_STARTED,
	EVENT_STARTING,
	EVENT_STARTED,
	EVENT_ENDING,
	EVENT_ENDED
};

typedef void *(* VCControl_Ctor_Type)(void *);
typedef int (* IsDemoEventHappeningType)(void *);
typedef int (* IsFreezerEventEndingType)(void *);
typedef int (* GetStatueTypeType)(void *);
typedef int (* GetResistancePointsType)(void);
typedef void (* AddResistancePointsType)(int);
typedef void (* ChangeMedalsType)(int);
typedef void (* QuestFunctionType)(void *, void *, uint32_t, uint32_t, uint32_t);

static VCControl_Ctor_Type VCControl_Ctor;
static IsDemoEventHappeningType IsDemoEventHappening;
static IsFreezerEventEndingType IsFreezerEventEnding;
static GetStatueTypeType GetStatueType;
static GetResistancePointsType GetResistancePoints;
static AddResistancePointsType AddResistancePoints;
//static ChangeMedalsType ChangeMedals;
static void *QuestFunctionAddress;
static QuestFunctionType QuestFunction;

static int event_state;
static float probability;
static int duration;
static int goal;
static float resistance_multiplier;
static int prize;

static uint8_t orig_ins1[7], orig_ins2[10], orig_ins3[5];
static uint8_t *patch_addr1, *patch_addr2, *patch_addr3;

static void *em_callback;

static DWORD WINAPI EventTimer(LPVOID)
{
	Sleep(duration*60*1000);	
	
	if (event_state < EVENT_ENDING && event_state != EVENT_NON_STARTED)
		event_state = EVENT_ENDING;
	
	return 0;
}

static void QuestFunctionPatched(void *pthis, QXDQuest *quest, uint32_t a3, uint32_t a4, uint32_t a5)
{
	if (event_state == EVENT_STARTED && prize > 0)
	{
		if (quest && memcmp(quest->name, "LEQ_", 4) == 0)
		{
			int resistance_points = quest->resistance_points;
			resistance_points = (int)((float)resistance_points * resistance_multiplier);
			
			// If we are going to meet the reward
			if (resistance_points+GetResistancePoints() >= goal)
			{
				// Set TP medals reward
				quest->tp_medals = prize;
			}
		}
	}
	
	QuestFunction(pthis, quest, a3, a4, a5);
}

extern "C"
{
	
PUBLIC void SetupFsEventOffline(VCControl_Ctor_Type orig)
{
	VCControl_Ctor = orig;
	
	bool offline_mode;
	
	ini.GetBooleanValue("Patches", "offline_mode", &offline_mode, false);
	
	if (!offline_mode)
	{
		UPRINTF("fs_event_offline = true requires offline_mode = true");
		exit(-1);
	}
	
	ini.GetFloatValue("FS_OFFLINE", "probability", &probability, 0.01f);
	ini.GetIntegerValue("FS_OFFLINE", "duration", &duration, 120);
	ini.GetIntegerValue("FS_OFFLINE", "goal", &goal, 5000);
	ini.GetFloatValue("FS_OFFLINE", "resistance_multiplier", &resistance_multiplier, 2.5f);
	ini.GetIntegerValue("FS_OFFLINE", "prize_tp", &prize, 100);
	
	event_state = EVENT_NON_STARTED;
}
	
PUBLIC void *VCControl_Ctor_Patched(void *pthis)
{
	if (event_state == EVENT_NON_STARTED)
	{
		float rnd = Utils::RandomProbability();
		
		if (rnd < probability)
		{
			DPRINTF("Will trigger FS_OFFLINE_EVENT\n");
			event_state = EVENT_STARTING;
		}
		else
		{
			//DPRINTF("Won't trigger FS_OFFLINE_EVENT\n");
		}
	}
	
	return VCControl_Ctor(pthis);
}

PUBLIC void SetupIsDemoEventHappening(IsDemoEventHappeningType orig)
{
	IsDemoEventHappening = orig;
}

PUBLIC int IsDemoEventHappeningPatched(void *pthis)
{
	switch (event_state)
	{
		case EVENT_STARTING: case EVENT_ENDING:
			return 1;
			
		case EVENT_STARTED:
			return 0;
	}
	
	return IsDemoEventHappening(pthis);
}

PUBLIC void SetupIsFreezerEventEnding(IsFreezerEventEndingType orig)
{
	IsFreezerEventEnding = orig;
}

PUBLIC int IsFreezerEventEndingPatched(void *pthis)
{
	static uint8_t patch2[10] = { 0xBB, 0x01, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90 }; // mov ebx, 1; fill with nop; (ORIGINAL CODE: test eax, eax; mov eax, 0; cmovz ebx, eax)
	
	switch (event_state)
	{
		case EVENT_STARTING: 		
			event_state = EVENT_STARTED;
			
			PatchUtils::Write16(patch_addr1, 0x9090);
			PatchUtils::Write16(patch_addr1+5, 0x9090);
			
			PatchUtils::Copy(patch_addr2, patch2, sizeof(patch2));
			SetRelAddr(patch_addr3, em_callback);
			
			PatchUtils::Hook(QuestFunctionAddress, (void **)&QuestFunction, (void *)QuestFunctionPatched);
			
			CreateThread(nullptr, 0, EventTimer, nullptr, 0, nullptr);
			return 0;
			
		case EVENT_ENDING:
			event_state = EVENT_ENDED;
			
			PatchUtils::Copy(patch_addr1, orig_ins1, sizeof(orig_ins1));			
			PatchUtils::Copy(patch_addr2, orig_ins2, sizeof(orig_ins2));
			PatchUtils::Copy(patch_addr3, orig_ins3, sizeof(orig_ins3));	

			PatchUtils::Unhook(QuestFunctionAddress);
			
			return 1;
			
		case EVENT_STARTED: // We shouldn't be never here
			return 0;
	}
	
	return IsFreezerEventEnding(pthis);
}

PUBLIC void OnSetIsFsEventHappeningLocated(uint8_t *addr)
{
	patch_addr1 = addr;
	memcpy(orig_ins1, addr, sizeof(orig_ins1));
}	

PUBLIC void OnPatchLobbyScriptIsOpenedEventLocated(uint8_t *addr)
{
	patch_addr2 = addr;
	memcpy(orig_ins2, addr, sizeof(orig_ins2));
}

PUBLIC void OnPatchOpenQuestCallbackLocated(uint8_t *addr)
{
	em_callback = GetAddrFromRel(addr);
	
	patch_addr3 = addr + 0xC;
	memcpy(orig_ins3, patch_addr3, sizeof(orig_ins3));
}

PUBLIC void SetupGetStatueType(GetStatueTypeType orig)
{
	GetStatueType = orig;
}

PUBLIC int GetStatueTypePatched(void *pthis)
{
	if (event_state == EVENT_STARTED || event_state == EVENT_ENDING)
		return 4; // Freezer statue
	
	return GetStatueType(pthis);
}

PUBLIC void SetupGetResistancePoints(GetResistancePointsType orig)
{
	GetResistancePoints = orig;
}

PUBLIC void SetupAddResistancePoints(AddResistancePointsType orig)
{
	AddResistancePoints = orig;
}

extern void FS_SetEdi(uint32_t value);

PUBLIC void AddResistancePointsPatched(int points)
{
	points = (int)((float)points * resistance_multiplier);
	AddResistancePoints(points);
	
	if (GetResistancePoints() >= goal)
	{
		if (event_state < EVENT_ENDING && event_state != EVENT_NON_STARTED)
		{			
			event_state = EVENT_ENDING;
		}
	}
	
	FS_SetEdi(points);
}

/*PUBLIC void OnChangeMedalsLocated(ChangeMedalsType orig)
{
	ChangeMedals = orig;
	
	// This is to fix the issue of multiple functions having same signature
	if (RelAddress((void *)orig) != CHANGE_MEDALS_ADDRESS)
	{
		UPRINTF("Update the value of CHANGE_MEDALS_ADDRESS\n");
		exit(-1);
	}
}*/

PUBLIC void OnQuestFunctionLocated(void *orig)
{
	QuestFunctionAddress = orig;
}
	
} // extern "C"