#include <windows.h>
#include <math.h>
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include "PatchUtils.h"
#include "PscFile.h"
#include "Xv2StageDefFile.h"
#include "xv2stagedef_default.inc"
#include "xv2patcher.h"
#include "stage_patch.h"
#include "vc_defs.h"
#include "debug.h"

#include "Misc/IggyFile.h"

#define LOOKUP_SIZE	0x1000

// Address of stagegt within XG::Game::Battle::Core::MainSystem 
#define STAGEGT_ADDRESS	0x6558
#define MAINSYSTEM_SIZE	0xEA20

static uint8_t *orig_stage_defs1;
static uint8_t *new_stage_defs1;

static uint8_t *orig_ssid_to_idx;
static uint8_t *new_ssid_to_idx;

static uint8_t *orig_stage_f6;
static uint8_t *new_stage_f6;

static uint8_t *orig_stage_defs2;
static uint8_t *new_stage_defs2;
static uint8_t *orig_stage_defs2_gbb; // 1.21
static uint8_t *new_stage_defs2_gbb; // 1.21

static uint8_t *orig_stage_sounds;
static uint8_t *new_stage_sounds;

static uint8_t *orig_stage_music;
static uint8_t *new_stage_music; 

static Xv2StageDefFile x2sta;
static std::map<std::string, size_t> str_map;
static char *strings;

void StageDef1Breakpoint(void *pc, void *addr)
{
	UPRINTF("Old stage def1 was accessed. Look at log for more info.\n");
	
	DPRINTF("Old stage def1 was accessed at address %p (rel %Ix)\n", pc, RelAddress(pc));
	DPRINTF("Address that was accessed: %p (rel %Ix)\n", addr, RelAddress(addr));
	
	exit(-1);
}

void StageSsidToIdxBreakpoint(void *pc, void *addr)
{
	UPRINTF("Old ssid_to_idx was accessed. Look at log for more info.\n");
	
	DPRINTF("Old ssid_to_idx was accessed at address %p (rel %Ix)\n", pc, RelAddress(pc));
	DPRINTF("Address that was accessed: %p (rel %Ix)\n", addr, RelAddress(addr));
	
	exit(-1);
}

void StageF6Breakpoint(void *pc, void *addr)
{
	UPRINTF("Old stage_f6 was accessed. Look at log for more info.\n");
	
	DPRINTF("Old stage_f6 was accessed at address %p (rel %Ix)\n", pc, RelAddress(pc));
	DPRINTF("Address that was accessed: %p (rel %Ix)\n", addr, RelAddress(addr));
	
	exit(-1);
}

void StageDef2Breakpoint(void *pc, void *addr)
{
	UPRINTF("Old stage def2 was accessed. Look at log for more info.\n");
	
	DPRINTF("Old stage def2 was accessed at address %p (rel %Ix)\n", pc, RelAddress(pc));
	DPRINTF("Address that was accessed: %p (rel %Ix)\n", addr, RelAddress(addr));
	
	exit(-1);
}

void StageGtBreakpoint(void *pc, void *addr)
{
	UPRINTF("Old stagegt was accessed. Look at log for more info.\n");
	
	DPRINTF("Old stagegt was accessed at address %p (rel %Ix)\n", pc, RelAddress(pc));
	DPRINTF("Address that was accessed: %p\n", addr);
	
	exit(-1);
}

extern "C"
{
	
PUBLIC bool IsDebuggingStages()
{
	return false;
}

static bool DebuggingSetBreakPoint()
{
	return false;
}
	
PUBLIC void PatchStageDef1Main(void *buf)
{
	orig_stage_defs1 = (uint8_t *)GetAddrFromRel(buf);
	DPRINTF("Original stage_defs1 located at %p (rel 0x%Ix)\n", orig_stage_defs1, RelAddress(orig_stage_defs1));
	
	const std::string stage_def_path = myself_path + CONTENT_ROOT + "data/xv2_stage_def.xml";
	
	if (!Utils::FileExists(stage_def_path))
	{
		TiXmlDocument doc;
        doc.Parse(default_stagedef);

        if (doc.ErrorId() != 0)
        {
            UPRINTF("%s: Internal error parsing default stage_def (this should never happen)\n", FUNCNAME);            
            exit(-1);
        }

        if (!x2sta.Compile(&doc))
        {
            UPRINTF("%s: Internal error compiling default stage_def (this should never happen)\n", FUNCNAME);            
            exit(-1);
        }
	}
	else
	{	
		if (!x2sta.CompileFromFile(stage_def_path))
		{
			UPRINTF("Failed to compile stage_def file.\nLog may have more info.\n");		
			exit(-1);
		}
	}
	
	size_t strings_size = x2sta.BuildStrings(str_map);
	
	if (strings_size == 0)
	{
		DPRINTF("**********WARNING: strings size is 0.\n");
	}
	
	strings = (char *)GlobalAlloc(GPTR, strings_size);
	if (!strings)
	{
		UPRINTF("Failed to allocate memory for stage strings.\n");
		exit(-1);
	}
	
	for (auto it = str_map.begin(); it != str_map.end(); ++it)
    {
		strcpy(strings+it->second, it->first.c_str());
	}
	
	new_stage_defs1 = (uint8_t *)PatchUtils::AllocateIn32BitsArea(buf, x2sta.GetNumStages()*sizeof(XV2StageDef1));	
	DPRINTF("New stage_defs1 allocated at %p\n", new_stage_defs1);
	
	XV2StageDef1 *defs1 = (XV2StageDef1 *)new_stage_defs1;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		defs1[i].base_dir = strings + str_map[x2sta[i].base_dir];
		defs1[i].code = strings + str_map[x2sta[i].code];
		defs1[i].dir = strings + str_map[x2sta[i].dir];
		defs1[i].str4 = strings + str_map[x2sta[i].str4];
		defs1[i].unk5 = x2sta[i].unk5;
	}
	
	SetRelAddr(buf, new_stage_defs1);
	
	/*** DEBUG ***/	
	if (IsDebuggingStages() && DebuggingSetBreakPoint())
	{
		if (!PatchUtils::SetMemoryBreakpoint(orig_stage_defs1, XV2_ORIGINAL_NUM_STAGES*sizeof(XV2StageDef1), StageDef1Breakpoint))
		{
			DPRINTF("Failed to set memory breakpoint.\n");
			exit(-1);
		}
	}
	/*** ***/
}

PUBLIC void PatchStageDef1Top(void *buf)
{
	SetRelAddr(buf, new_stage_defs1);
}

PUBLIC void PatchStageDef1Off8(void *buf)
{
	SetRelAddr(buf, new_stage_defs1+8);
}

PUBLIC void PatchStageDef1Off8_1(uint8_t *buf)
{
	const int second_component_rel = 0x74; // // The 0x74 relative address for the second component of the address could change when the patch signature breaks	
	
	uint8_t *addr = (uint8_t *)GetAddrFromRel(buf);
	int second_component = *(int32_t *)&buf[second_component_rel];
	addr += second_component;
	//DPRINTF("Off8_1 address %p, rel=%Ix\n", addr, RelAddress(addr));
	
	PatchUtils::Write32(buf+second_component_rel, second_component + Utils::DifPointer(new_stage_defs1+8, addr));	
	
	/*uint8_t *new_addr = (uint8_t *)GetAddrFromRel(buf) + *(int32_t *)&buf[second_component_rel];
	DPRINTF("New Off8_1 address %p\n", new_addr);*/
}

PUBLIC void PatchStageDef1Off8_2(uint8_t *buf)
{
	const int second_component_rel = 0x15;	// The 0x15 relative address for the second component of the address could change when the patch signature breaks
	
	uint8_t *addr = (uint8_t *)GetAddrFromRel(buf);
	int second_component = *(int32_t *)&buf[second_component_rel];
	addr += second_component;
	//DPRINTF("Off8_2 address %p, rel=%Ix\n", addr, RelAddress(addr));
	
	PatchUtils::Write32(buf+second_component_rel, second_component + Utils::DifPointer(new_stage_defs1+8, addr));	
	
	/*uint8_t *new_addr = (uint8_t *)GetAddrFromRel(buf) + *(int32_t *)&buf[second_component_rel];
	DPRINTF("New Off8_2 address %p\n", new_addr);*/
}

PUBLIC void PatchStageDef1Off8_Absolute(uint32_t *buf)
{
	//DPRINTF("Address: %p\n", *buf + (uintptr_t)GetModuleHandle(nullptr));	
	PatchUtils::Write32(buf, Utils::DifPointer(new_stage_defs1+8, GetModuleHandle(nullptr)));
	//DPRINTF("Addruss: %p\n", *buf + (uintptr_t)GetModuleHandle(nullptr));	
}

PUBLIC void PatchStageDef1BFnmsOff8(void *buf)
{
	XV2StageDef1 *defs1 = (XV2StageDef1 *)new_stage_defs1;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		if (strcmp(defs1[i].code, "BFnms") == 0)
		{
			SetRelAddr(buf, &defs1[i].code);
			return;
		}
	}

	UPRINTF("%s: Could not find BFnms.\n", FUNCNAME);
	exit(-1);
}

PUBLIC void PatchStageDef1BFnmcOff8(void *buf)
{
	XV2StageDef1 *defs1 = (XV2StageDef1 *)new_stage_defs1;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		if (strcmp(defs1[i].code, "BFnmc") == 0)
		{
			SetRelAddr(buf, &defs1[i].code);
			return;
		}
	}

	UPRINTF("%s: Could not find BFnmc.\n", FUNCNAME);
	exit(-1);
}

PUBLIC void PatchStageDef1CHR(void *buf)
{
	XV2StageDef1 *defs1 = (XV2StageDef1 *)new_stage_defs1;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		if (strcmp(defs1[i].code, "CHR") == 0)
		{
			SetRelAddr(buf, &defs1[i].base_dir);
			return;
		}
	}

	UPRINTF("%s: Could not find CHR.\n", FUNCNAME);
	exit(-1);
}

PUBLIC void PatchStageDef1CHROff8(void *buf)
{
	XV2StageDef1 *defs1 = (XV2StageDef1 *)new_stage_defs1;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		if (strcmp(defs1[i].code, "CHR") == 0)
		{
			SetRelAddr(buf, &defs1[i].code);
			return;
		}
	}

	UPRINTF("%s: Could not find CHR.\n", FUNCNAME);
	exit(-1);
}

PUBLIC void PatchStageDef1CHROff10(void *buf)
{
	XV2StageDef1 *defs1 = (XV2StageDef1 *)new_stage_defs1;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		if (strcmp(defs1[i].code, "CHR") == 0)
		{
			SetRelAddr(buf, &defs1[i].dir);
			return;
		}
	}

	UPRINTF("%s: Could not find CHR.\n", FUNCNAME);
	exit(-1);
}

PUBLIC void PatchStageDef1BottomOff8(void *buf)
{
	SetRelAddr(buf, new_stage_defs1+8 + (x2sta.GetNumStages()*sizeof(XV2StageDef1)) );
}

PUBLIC void PatchStageSsidToIdxMain(void *buf)
{
	orig_ssid_to_idx = (uint8_t *)GetAddrFromRel(buf);
	DPRINTF("Original ssid_to_idx located at %p (rel 0x%Ix)\n", orig_ssid_to_idx, RelAddress(orig_ssid_to_idx));
	
	new_ssid_to_idx = (uint8_t *)PatchUtils::AllocateIn32BitsArea(buf, x2sta.GetNumSsStages()*sizeof(uint32_t));	
	DPRINTF("New ssid_to_idx allocated at %p\n", new_ssid_to_idx);
	
	x2sta.BuildSsidMap(new_ssid_to_idx);	
	SetRelAddr(buf, new_ssid_to_idx);
	
	/*** DEBUG ***/	
	/*if (!PatchUtils::SetMemoryBreakpoint(orig_ssid_to_idx, XV2_ORIGINAL_NUM_SS_STAGES*sizeof(uint32_t), StageSsidToIdxBreakpoint))
	{
		DPRINTF("Failed to set memory breakpoint.\n");
		exit(-1);
	}*/
	/*** ***/
}

PUBLIC void PatchStageSsidToIdx(void *buf)
{
	SetRelAddr(buf, new_ssid_to_idx);
}

PUBLIC void PatchStageF6(void *buf)
{
	orig_stage_f6 = (uint8_t *)GetAddrFromRel(buf);
	DPRINTF("Original stage_f6 located at %p (rel 0x%Ix)\n", orig_stage_f6, RelAddress(orig_stage_f6));
	
	new_stage_f6 = (uint8_t *)PatchUtils::AllocateIn32BitsArea(buf, x2sta.GetNumStages()*sizeof(float));	
	DPRINTF("New stage_f6 allocated at %p\n", new_stage_f6);
	
	float *f6s = (float *)new_stage_f6;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		f6s[i] = x2sta[i].ki_blast_size_limit;
	}		
	
	SetRelAddr(buf, new_stage_f6);
	
	/*** DEBUG ***/	
	/*if (!PatchUtils::SetMemoryBreakpoint(orig_stage_f6, XV2_ORIGINAL_NUM_STAGES*sizeof(float), StageF6Breakpoint))
	{
		DPRINTF("Failed to set memory breakpoint.\n");
		exit(-1);
	}*/
	/*** ***/
}

PUBLIC void PatchStageDef2Main(void *buf)
{
	orig_stage_defs2 = (uint8_t *)GetAddrFromRel(buf);
	DPRINTF("Original stage_defs2 located at %p (rel 0x%Ix)\n", orig_stage_defs2, RelAddress(orig_stage_defs2));	

	new_stage_defs2 = (uint8_t *)PatchUtils::AllocateIn32BitsArea(buf, x2sta.GetNumStages()*sizeof(XV2StageDef2));	
	DPRINTF("New stage_defs2 allocated at %p\n", new_stage_defs2);
	
	XV2StageDef2 *defs2 = (XV2StageDef2 *)new_stage_defs2;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		for (size_t j = 0; j < XV2_STA_NUM_GATES; j++)
		{
			defs2[i].gates[j].name = strings + str_map[x2sta[i].gates[j].name];
			defs2[i].gates[j].target_stage_idx = x2sta[i].gates[j].target_stage_idx;
			defs2[i].gates[j].unk_10 = x2sta[i].gates[j].unk_10;
			// 1.21
			defs2[i].gates[j].unk_14 = x2sta[i].gates[j].unk_14;
			defs2[i].gates[j].unk_18 = x2sta[i].gates[j].unk_18;
		}
	}	
	
	// We don't longer need to set this, since this patch happens in stagegt_default and we now reimplement it
	//SetRelAddr(buf, new_stage_defs2);
	
	/*** DEBUG ***/	
	if (IsDebuggingStages() && DebuggingSetBreakPoint())
	{
		if (!PatchUtils::SetMemoryBreakpoint(orig_stage_defs2, XV2_ORIGINAL_NUM_STAGES*sizeof(XV2StageDef2), StageDef2Breakpoint))
		{
			DPRINTF("Failed to set memory breakpoint.\n");
			exit(-1);
		}
	}
	/*** ***/
}

// New: 1.21
PUBLIC void PatchStageDef2MainGBB(void *buf)
{
	orig_stage_defs2_gbb = (uint8_t *)GetAddrFromRel(buf);
	DPRINTF("Original stage_defs2_gbb located at %p (rel 0x%Ix)\n", orig_stage_defs2_gbb, RelAddress(orig_stage_defs2_gbb));	

	new_stage_defs2_gbb = (uint8_t *)PatchUtils::AllocateIn32BitsArea(buf, x2sta.GetNumStages()*sizeof(XV2StageDef2));	
	DPRINTF("New stage_defs2_gbb allocated at %p\n", new_stage_defs2_gbb);
	
	XV2StageDef2 *defs2 = (XV2StageDef2 *)new_stage_defs2_gbb;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		for (size_t j = 0; j < XV2_STA_NUM_GATES; j++)
		{
			defs2[i].gates[j].name = strings + str_map[x2sta[i].gates_gbb[j].name];
			defs2[i].gates[j].target_stage_idx = x2sta[i].gates_gbb[j].target_stage_idx;
			defs2[i].gates[j].unk_10 = x2sta[i].gates_gbb[j].unk_10;
			// 1.21
			defs2[i].gates[j].unk_14 = x2sta[i].gates_gbb[j].unk_14;
			defs2[i].gates[j].unk_18 = x2sta[i].gates_gbb[j].unk_18;
		}
	}	
	
	// We don't need to set this, since this patch happens in stagegt_default and is reimplemented
	//SetRelAddr(buf, new_stage_defs2_gbb);
	
	/*** DEBUG ***/	
	if (IsDebuggingStages() && DebuggingSetBreakPoint())
	{
		if (!PatchUtils::SetMemoryBreakpoint(orig_stage_defs2_gbb, XV2_ORIGINAL_NUM_STAGES*sizeof(XV2StageDef2), StageDef2Breakpoint))
		{
			DPRINTF("Failed to set memory breakpoint.\n");
			exit(-1);
		}
	}
	/*** ***/
}

PUBLIC void PatchStageSoundsMain(void *buf)
{
	orig_stage_sounds = (uint8_t *)GetAddrFromRel(buf);
	DPRINTF("Original stage_sounds located at %p (rel 0x%Ix)\n", orig_stage_sounds, RelAddress(orig_stage_sounds));
	
	new_stage_sounds = (uint8_t *)PatchUtils::AllocateIn32BitsArea(buf, x2sta.GetNumStages() * sizeof(void *));
	DPRINTF("New stage_sounds allocated at %p\n", new_stage_sounds);
	
	char **sounds = (char **)new_stage_sounds;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		if (x2sta[i].se.length() == 0)
		{
			sounds[i] = nullptr;
		}
		else
		{
			sounds[i] = strings + str_map[x2sta[i].se];
		}
	}	
	
	
	SetRelAddr(buf, new_stage_sounds);
}

PUBLIC void PatchStageMusicMain(void *buf)
{
	orig_stage_music = (uint8_t *)GetAddrFromRel(buf);
	DPRINTF("Original stage_music located at %p (rel 0x%Ix)\n", orig_stage_music, RelAddress(orig_stage_music));
	
	new_stage_music = (uint8_t *)PatchUtils::AllocateIn32BitsArea(buf, x2sta.GetNumStages() * sizeof(uint32_t));
	DPRINTF("New stage_music allocated at %p\n", new_stage_music);
	
	uint32_t *music = (uint32_t *)new_stage_music;
	
	for (size_t i = 0; i < x2sta.GetNumStages(); i++)
	{
		music[i] = x2sta[i].bgm_cue_id;
	}
	
	// Clear this as we don't need it anymore
	str_map.clear();
	
	
	SetRelAddr(buf, new_stage_music);
}

PUBLIC void PatchStageDef2Top(void *buf)
{
	SetRelAddr(buf, new_stage_defs2);
}

// New in 1.21
PUBLIC void PatchStageDef2TopGBB(void *buf)
{
	SetRelAddr(buf, new_stage_defs2_gbb);
}

// 1.21: not longer in use 
/*PUBLIC void PatchStageDef2Off4(void *buf)
{
	SetRelAddr(buf, new_stage_defs2+4);
}

PUBLIC void PatchStageDef2Off8(void *buf)
{
	SetRelAddr(buf, new_stage_defs2+8);
}*/

// These aren't needed anymore (patcher 4.2: stage limit removal)
/*PUBLIC void PatchStageNum_8(uint8_t *buf)
{
	PatchUtils::Write8(buf, x2sta.GetNumStages());
}

PUBLIC void PatchStageNumM1_8(uint8_t *buf)
{
	PatchUtils::Write8(buf, x2sta.GetNumStages()-1);
}

PUBLIC void PatchStageNum_32(uint32_t *buf)
{
	PatchUtils::Write32(buf, x2sta.GetNumStages());
}

PUBLIC void PatchStageSsNumComplex(int8_t *buf)
{
	uint8_t byte = (uint8_t) (x2sta.GetNumSsStages() - 0x24); // On original, this should return -2, aka 0xFE;
	//DPRINTF("byte %02X\n", byte);
	PatchUtils::Write8(buf, byte);
}*/

typedef int32_t (* StageEveType)(int32_t, StdString *);
typedef void * (* StringCopyType)(StdString *, const char *, size_t);

static StageEveType StageEve;
static StringCopyType StringCopy;

PUBLIC void SetupStageEve(StageEveType orig)
{
	StageEve = orig;
	
	// Note, this function will trigger the memory breakpoint, so disable that when dumping
	if (IsDebuggingStages())
	{
		static bool done = false;
		
		if (!done)
		{
			done = true;
			
			std::string c_array = "{ ";
			
			for (int i = 0; i < XV2_ORIGINAL_NUM_STAGES; i++)
			{
				StdString name;
				
				int32_t ret = StageEve(i, &name);
				
				DPRINTF("%d -> %d %s\n", i, ret, name.CStr());
				c_array.push_back('"');
				c_array += name.CStr();
				c_array.push_back('"');
				
				if (i == XV2_ORIGINAL_NUM_STAGES-1)
				{
					c_array += " };";
				}
				else
				{
					c_array += ", ";
				}
			}
			
			DPRINTF("%s\n", c_array.c_str());
		}
	}
}

PUBLIC void OnStringCopyLocated(void *address)
{
	StringCopy = (StringCopyType)GetAddrFromRel(address);
}

PUBLIC int32_t StageEveReimplemented(int32_t idx, StdString *name)
{
	if (idx < 0 || idx >= (int32_t)x2sta.GetNumStages())
		return 0;
	
	const Xv2Stage &stage = x2sta[idx];
	
	StringCopy(name, stage.eve.c_str(), stage.eve.length());	
	return (stage.eve != "template");
}

PUBLIC void IncreaseCoreMainSystemMemory(uint32_t *psize)
{
	uint32_t additional_size = x2sta.GetNumStages() * sizeof(XV2StageDef2) + XV2_STA_NUM_GATES*x2sta.GetNumStages()*sizeof(uint32_t);
	uint32_t size = *psize;
	
	if (size != MAINSYSTEM_SIZE)
	{
		UPRINTF("Update MAINSYSTEM_SIZE\n");
		exit(-1);
	}
	
	PatchUtils::Write32(psize, size+additional_size);
	DPRINTF("XG::Game::Battle::Core::MainSystem object size increased from 0x%x to 0x%x\n", size, *psize);
}

typedef void *(* MainSystem_CtorType)(void *);
static MainSystem_CtorType MainSystem_Ctor;

PUBLIC void SetupMainSystem_Ctor(MainSystem_CtorType orig)
{
	MainSystem_Ctor = orig;
}

/*** This function is debug only, it only gets enabled on IsDebuggingStages ***/
PUBLIC void *HookedMainSystem_Ctor(uint8_t *pthis)
{
	uint8_t *bp_addr = pthis+STAGEGT_ADDRESS;
	uint32_t bp_size = (XV2_ORIGINAL_NUM_STAGES * sizeof(XV2StageDef2)) + (XV2_STA_NUM_GATES*XV2_ORIGINAL_NUM_STAGES*sizeof(uint32_t));
	
	if (!PatchUtils::SetMemoryBreakpoint(bp_addr, bp_size, StageGtBreakpoint))
	{
		DPRINTF("Failed to set memory breakpoint.\n");
		exit(-1);
	}
	
	DPRINTF("***Memory breakpoint set to %p (size 0x%x)\n", bp_addr, bp_size);
		
	return MainSystem_Ctor(pthis);
}

typedef void (* MainSystem_DtorType)(void *);
static MainSystem_DtorType MainSystem_Dtor;

static void **pmainsystem_singleton;
static BattleInterface **pbattleinterface_singleton;
static QuestManager **pquestmanager_singleton;

PUBLIC void SetupMainSystem_Dtor(MainSystem_DtorType orig)
{
	MainSystem_Dtor = orig;
}

/*** This function is debug only, it only gets enabled on IsDebuggingStages ***/
PUBLIC void HookedMainSystem_Dtor(uint8_t *pthis)
{
	uint8_t *bp_addr = pthis+STAGEGT_ADDRESS;
	uint32_t bp_size = (XV2_ORIGINAL_NUM_STAGES * sizeof(XV2StageDef2)) + (XV2_STA_NUM_GATES*XV2_ORIGINAL_NUM_STAGES*sizeof(uint32_t));
	
	PatchUtils::UnsetMemoryBreakpoint(bp_addr, bp_size);
	DPRINTF("***Unset memory breakpoint in %p (size 0x%x)\n", bp_addr, bp_size);
	
	MainSystem_Dtor(pthis);
}

static inline uint8_t *GetStageGtNewAddress(uint8_t *pthis)
{
	return (pthis - STAGEGT_ADDRESS) + MAINSYSTEM_SIZE;
}

static bool IsGbbMode()
{
	if (!pmainsystem_singleton || !pbattleinterface_singleton || !pquestmanager_singleton)
		return false;
	
	// Note: even if not field of mainsystem is used here, the game actually checks for it to not be NULL 
	if (!*pmainsystem_singleton)
		return false;
	
	// Implementation is divided in two parts
	
	// Part 1: imitation of function 0x7DF210 (1.21)
	// Note: the game didn't do null check here, we add it just in case
	BattleInterface *bis = *pbattleinterface_singleton;	
	if (!bis)
		return false;
	
	if (!bis->IsGbbMode())
		return false;
	
	// Part 2: imitation of the rest of the check. Can be seen in stagegt_default and some other function(s)
	QuestManager *qms = *pquestmanager_singleton;
	if (!qms)
		return false;
	
	// Simplified the code to remove the redundant <= 0x12
	//return (qms->mode <= 0x12 && qms->mode == 0x10)
	return (qms->mode == 0x10);
}

static inline uint8_t *GetNewStageDefs2ForCurrentMode()
{
	//DPRINTF("IsGbbMode: %d\n", IsGbbMode());
	return IsGbbMode() ? new_stage_defs2_gbb : new_stage_defs2;
}

PUBLIC void StageGtClearReimplemented(uint8_t *pthis)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	size_t num = x2sta.GetNumStages();
	
	for (size_t i = 0; i < num; i++)
	{
		for (int j = 0; j < XV2_STA_NUM_GATES; j++)
		{
			defs2[i].gates[j].name = nullptr;
			defs2[i].gates[j].target_stage_idx = 0xFFFFFFFF;
			defs2[i].gates[j].unk_0C = 0;
			defs2[i].gates[j].unk_10 = 1;
			// 1.21
			defs2[i].gates[j].unk_14 = 0xFFFFFFFF;
			defs2[i].gates[j].unk_18 = 0;
		}
	}
}

PUBLIC void StageGtDefaultReimplemented(uint8_t *pthis)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	size_t num = x2sta.GetNumStages();
	
	memcpy(defs2, GetNewStageDefs2ForCurrentMode(), num*sizeof(XV2StageDef2));
	
	uint8_t *ptr = ((uint8_t *)defs2) + num*sizeof(XV2StageDef2);
	memset(ptr, 0, sizeof(uint32_t)*XV2_STA_NUM_GATES*num);
}

PUBLIC XV2StageGate *StageGtFindGateReimplemented(uint8_t *pthis, int32_t stage, int32_t gate)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	
	if (stage < 0 || stage >= (int)x2sta.GetNumStages())
		return nullptr;
	
	for (int i = 0; i < XV2_STA_NUM_GATES; i++)
	{
		int32_t this_target = (int32_t)defs2[stage].gates[i].target_stage_idx;
		if (this_target >= 0 && this_target == gate)
		{
			return &defs2[stage].gates[i];
		}
	}
	
	return nullptr;
}

PUBLIC XV2StageGate *StageGtFindGateByNameReimplemented(uint8_t *pthis, int32_t stage, const char *gate)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	
	// Original function doesn't check stage boundaries, so let's leave it like that
	
	for (int i = 0; i < XV2_STA_NUM_GATES; i++)
	{
		const char *this_name = defs2[stage].gates[i].name;
		if (this_name && strcmp(this_name, gate) == 0)
		{
			return &defs2[stage].gates[i];
		}
	}
	
	return nullptr;
}

PUBLIC XV2StageGate *StageGtGetGateReimplemented(uint8_t *pthis, int32_t stage, uint32_t gate)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	
	if (gate >= XV2_STA_NUM_GATES)
		return nullptr;
	
	return &defs2[stage].gates[gate];
}

PUBLIC uint32_t StageGtGetExtraByTargetReimplemented(uint8_t *pthis, int32_t stage, int32_t gate)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	
	size_t num = x2sta.GetNumStages();
	
	if (stage < 0 || stage >= (int)num)
		return 0;
	
	uint32_t *extra = (uint32_t *) (uint8_t *)(((uint8_t *)defs2) + num*sizeof(XV2StageDef2));
	extra += stage*XV2_STA_NUM_GATES;
	
	for (int i = 0; i < XV2_STA_NUM_GATES; i++)
	{
		int32_t this_target = (int32_t)defs2[stage].gates[i].target_stage_idx;
		if (this_target >= 0 && this_target == gate)
		{
			return extra[i];
		}
	}
	
	return 0;
}

PUBLIC uint32_t StageGtGetExtraByNameReimplemented(uint8_t *pthis, int32_t stage, const char *gate)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	size_t num = x2sta.GetNumStages();	
	
	uint32_t *extra = (uint32_t *) (uint8_t *)(((uint8_t *)defs2) + num*sizeof(XV2StageDef2));
	extra += stage*XV2_STA_NUM_GATES;
	
	// Original function doesn't check stage boundaries, so let's leave it like that
	
	for (int i = 0; i < XV2_STA_NUM_GATES; i++)
	{
		const char *this_name = defs2[stage].gates[i].name;
		if (this_name && strcmp(this_name, gate) == 0)
		{
			return extra[i];
		}
	}
	
	return 0;
}

PUBLIC uint32_t StageGtGetTargetReimplemented(uint8_t *pthis, int32_t stage, uint32_t gate)
{
	if (stage < 0 || stage >= (int)x2sta.GetNumStages() || gate >= XV2_STA_NUM_GATES)
		return 0xFFFFFFFF;
	
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	return defs2[stage].gates[gate].target_stage_idx;
}

PUBLIC void StageGtClearExtraReimplemented(uint8_t *pthis)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	size_t num = x2sta.GetNumStages();	
	
	uint32_t *extra = (uint32_t *) (uint8_t *)(((uint8_t *)defs2) + num*sizeof(XV2StageDef2));
	memset(extra, 0, sizeof(uint32_t)*XV2_STA_NUM_GATES*num);
}

PUBLIC void StageGtSetReimplemented(uint8_t *pthis, int32_t stage, int32_t gate, char *name, uint32_t target, uint32_t s28)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	size_t num = x2sta.GetNumStages();		
	
	if (stage < 0 || stage >= (int)num || gate < 0 || gate >= XV2_STA_NUM_GATES || !name)
		return;
	
	uint32_t *extra = (uint32_t *) (uint8_t *)(((uint8_t *)defs2) + num*sizeof(XV2StageDef2));
	extra += stage*XV2_STA_NUM_GATES;
	extra += gate;	
	
	XV2StageGate *tgate = &defs2[stage].gates[gate];
	
	tgate->name = name;
	tgate->target_stage_idx = target;
	tgate->unk_0C = 0;
	tgate->unk_10 = 1;
	
	*extra = (s28 != 0) + 1;
}

PUBLIC int StageGtSetExtraByTargetReimplemented(uint8_t *pthis, int32_t stage, int32_t gate, uint32_t extra_data)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	size_t num = x2sta.GetNumStages();			
	
	uint32_t *extra = (uint32_t *) (uint8_t *)(((uint8_t *)defs2) + num*sizeof(XV2StageDef2));
	extra += stage*XV2_STA_NUM_GATES;
	
	for (int i = 0; i < XV2_STA_NUM_GATES; i++)
	{
		int32_t this_target = (int32_t)defs2[stage].gates[i].target_stage_idx;		
		if (this_target >= 0 && this_target == gate)
		{
			extra[i] = extra_data;
			return 1;
		}
	}
	
	return 0;
}

PUBLIC int StageGtSetExtraByNameReimplemented(uint8_t *pthis, int32_t stage, const char *gate, uint32_t extra_data)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
	size_t num = x2sta.GetNumStages();			
	
	uint32_t *extra = (uint32_t *) (uint8_t *)(((uint8_t *)defs2) + num*sizeof(XV2StageDef2));
	extra += stage*XV2_STA_NUM_GATES;
	
	for (int i = 0; i < XV2_STA_NUM_GATES; i++)
	{
		const char *this_name = defs2[stage].gates[i].name;	
		if (this_name && strcmp(this_name, gate) == 0)
		{
			extra[i] = extra_data;
			return 1;
		}
	}
	
	return 0;
}

PUBLIC int StageGtSetUnk10ByTargetReimplemented(uint8_t *pthis, int32_t stage, int32_t gate, uint32_t unk_10)
{
	XV2StageDef2 *defs2 = (XV2StageDef2 *)GetStageGtNewAddress(pthis);	
		
	for (int i = 0; i < XV2_STA_NUM_GATES; i++)
	{
		int32_t this_target = (int32_t)defs2[stage].gates[i].target_stage_idx;		
		if (this_target >= 0 && this_target == gate)
		{
			defs2[stage].gates[i].unk_10 = unk_10;
			return 1;
		}
	}
	
	return 0;
}

typedef void (* SetFarClipTypeT1)(void *, float);
typedef void (* SetFarClipTypeT2)(float **);
SetFarClipTypeT1 SetFarClipT1;
SetFarClipTypeT2 SetFarClipT2;

int GetCurrentStage()
{
	if (!pmainsystem_singleton)
		return -1;
	
	uint64_t *mainsystem_singleton = (uint64_t *) (*pmainsystem_singleton);
	if (!mainsystem_singleton)
		return -1;
	
	int *object = (int *)(mainsystem_singleton[0x4368/8]);
	if (!object)
		return -1;
	
	return object[0x928/4];
}

// This one works in early time of loading, unlike above
int GetCurrentStage2()
{
	if (!pbattleinterface_singleton)
		return -1;
	
	int32_t *battleinterface_singleton = (int32_t *) (*pbattleinterface_singleton);
	if (!battleinterface_singleton)
		return -1;
	
	return battleinterface_singleton[(0x10+0xC) / 4];
}

void PrintCurrentStageGates()
{
	int current_stage = GetCurrentStage();
	if (current_stage < 0)
		return;	
	
	DPRINTF("cs = %d\n", current_stage);
	
	uint8_t *mainsystem_singleton = (uint8_t *) (*pmainsystem_singleton);
	XV2StageDef2 *defs2 = (XV2StageDef2 *)(mainsystem_singleton + MAINSYSTEM_SIZE);
	
	uint32_t *extra = (uint32_t *) (uint8_t *)(((uint8_t *)defs2) + x2sta.GetNumStages()*sizeof(XV2StageDef2));
	extra += current_stage*XV2_STA_NUM_GATES;
	
	for (int i = 0; i < XV2_STA_NUM_GATES; i++)
	{
		const XV2StageGate &gate = defs2[current_stage].gates[i];
		
		DPRINTF("--Gate %d\n", i);
		
		DPRINTF("Name: %s\n", gate.name);		
		DPRINTF("Target: %d\n", gate.target_stage_idx);
		DPRINTF("Unk 0C: %d\n", gate.unk_0C);
		DPRINTF("Unk 10: %d\n", gate.unk_10);
		DPRINTF("Unk 14: %d\n", gate.unk_14);
		DPRINTF("Unk 18: %Id\n", gate.unk_18);
		DPRINTF("Extra: %d\n", extra[i]);
	}
}

bool OpenedGateExist(bool *error)
{
	int current_stage = GetCurrentStage();
	if (current_stage < 0)
	{
		if (error)
			*error = true;
		
		return false;
	}
	
	uint8_t *mainsystem_singleton = (uint8_t *) (*pmainsystem_singleton);
	XV2StageDef2 *defs2 = (XV2StageDef2 *)(mainsystem_singleton + MAINSYSTEM_SIZE);
	
	const uint32_t *extra = (uint32_t *) (uint8_t *)(((uint8_t *)defs2) + x2sta.GetNumStages()*sizeof(XV2StageDef2));
	extra += current_stage*XV2_STA_NUM_GATES;
	
	*error = false;
	
	for (int i = 0; i < XV2_STA_NUM_GATES; i++)
	{
		const XV2StageGate &gate = defs2[current_stage].gates[i];
		
		if ((int)gate.target_stage_idx >= 0)
		{
			// 0 -> no gate
			// 1 -> gate showing but closed
			// 2 -> gate open (regardless of wether it is green or not)
			if (extra[i] != 0 && extra[i] != 1)
				return true;
		}
	}
	
	return false;
}

PUBLIC void OnCoreMainSystemLocated_Stages(void *addr)
{
	pmainsystem_singleton = (void **)GetAddrFromRel(addr);
}

PUBLIC void OnBattleInterfaceSingletonLocated(void *addr)
{
	pbattleinterface_singleton = (BattleInterface **)GetAddrFromRel(addr);
}

PUBLIC void OnQuestManagerSingletonLocated(void *addr)
{
	pquestmanager_singleton = (QuestManager **)GetAddrFromRel(addr);
}

PUBLIC void SetupSetFarClipT1(SetFarClipTypeT1 orig)
{
	SetFarClipT1= orig;
}

PUBLIC void SetupSetFarClipT2(SetFarClipTypeT2 orig)
{
	SetFarClipT2 = orig;
}

PUBLIC void OverrideFarClipT1(void *pthis, float value)
{
	int current_stage = GetCurrentStage();
	//DPRINTF("Current stage = %d\n", current_stage);
	
	if (current_stage >= 0 && current_stage < (int)x2sta.GetNumStages())
	{
		float override_clip = x2sta[current_stage].override_far_clip;
		
		if (override_clip != 0.0)
		{
			value = override_clip;
		}
	}
	
	SetFarClipT1(pthis, value);
}

PUBLIC void OverrideFarClipT2(float **obj)
{
	SetFarClipT2(obj);
	
	int current_stage = GetCurrentStage();
	//DPRINTF("Current stage = %d\n", current_stage);
	
	if (current_stage >= 0 && current_stage < (int)x2sta.GetNumStages())
	{
		float override_clip = x2sta[current_stage].override_far_clip;
		
		if (override_clip != 0.0)
		{
			float *obj2 = *obj;			
			obj2[0xD0/4] = override_clip;
		}
	}
}

PUBLIC float GetStageLimit()
{
	int current_stage = GetCurrentStage2();
	
	if (current_stage >= 0 && current_stage < (int)x2sta.GetNumStages())
	{
		//DPRINTF("Setting limit of %s to %f\n", x2sta[current_stage].code.c_str(), x2sta[current_stage].limit);
		return x2sta[current_stage].limit;
	}
	else
	{
		//DPRINTF("Cannot get current stage (%d)\n", current_stage);
	}
	
	return 500.0f;
}

// From here, the part related with stagesele
typedef void *(* GetSelectedBeforeObjectType)(void *, int);
typedef bool (* HasStageBeenSelectedBeforeType)(void *, int);
typedef const char16_t * (* OrigGetStageNameType)(void *, int);

static std::string x2s_raw, x2s_raw_local;
static void **game_data;
static int *game_language;

static GetSelectedBeforeObjectType GetSelectedBeforeObject;
static HasStageBeenSelectedBeforeType HasStageBeenSelectedBefore;
static OrigGetStageNameType OrigGetStageName;

static std::vector<std::vector<std::u16string>> stage_names_ucs2;

static void ReadSlotsFile()
{
	static time_t mtime = 0;
	const std::string path = myself_path + CONTENT_ROOT + SLOTS_FILE_STAGE;
	time_t current_mtime;
	
	if (Utils::GetFileDate(path, &current_mtime) && current_mtime != mtime)
	{
		Utils::ReadTextFile(path, x2s_raw, false);
		mtime = current_mtime;
	}
}

static void ReadSlotsFileLocal()
{
	static time_t mtime = 0;
	const std::string path = myself_path + CONTENT_ROOT + SLOTS_FILE_STAGE_LOCAL;
	time_t current_mtime;
	
	if (Utils::GetFileDate(path, &current_mtime) && current_mtime != mtime)
	{
		Utils::ReadTextFile(path, x2s_raw_local, false);
		mtime = current_mtime;
	}
}

PUBLIC void OnGameLanguageLocated(void *addr)
{
	game_language = (int *)GetAddrFromRel(addr, 4);
}

PUBLIC void SetupGetStageName(OrigGetStageNameType orig)
{
	OrigGetStageName = orig;
	
	if (x2sta.GetNumStages() <= XV2_ORIGINAL_NUM_STAGES)
		return;
	
	size_t num = x2sta.GetNumStages() - XV2_ORIGINAL_NUM_STAGES;
	stage_names_ucs2.resize(num);
	
	for (size_t i = 0; i < num; i++)
	{
		stage_names_ucs2[i].resize(XV2_NATIVE_LANG_NUM);
		
		for (size_t j = 0; j < XV2_NATIVE_LANG_NUM; j++)
		{
			std::string name = x2sta[i+XV2_ORIGINAL_NUM_STAGES].GetNativeName(j);
			stage_names_ucs2[i][j] = Utils::Utf8ToUcs2(name);
		}
	}
}

PUBLIC const char16_t *GetStageNamePatched(void *pthis, int stage_id)
{
	if (stage_id >= XV2_ORIGINAL_NUM_STAGES)
	{
		int idx = stage_id - XV2_ORIGINAL_NUM_STAGES;
		
		if (idx >= (int)stage_names_ucs2.size())
			return u"WTF";
		
		int lang = *game_language;
		
		if (lang < 0 || lang >= XV2_NATIVE_LANG_NUM)
			return u"WTFH";
		
		return stage_names_ucs2[idx][lang].c_str();
	}
	
	return OrigGetStageName(pthis, stage_id);
}

PUBLIC void OnGameDataSingleton_Located(void *addr)
{
	game_data = (void **)GetAddrFromRel(addr);
}

PUBLIC void OnGetSelectedBeforeObject_Located(void *addr)
{
	GetSelectedBeforeObject = (GetSelectedBeforeObjectType)GetAddrFromRel(addr);
}

PUBLIC void OnHasStageBeenSelectedBefore_Located(void *addr)
{
	HasStageBeenSelectedBefore = (HasStageBeenSelectedBeforeType)GetAddrFromRel(addr);
}

// Iggy functions
int GetNumSsStages()
{
	return x2sta.GetNumSsStages();
}

const std::string &GetStagesSlotsData()
{
	ReadSlotsFile();
	return x2s_raw;
}

const std::string &GetLocalStagesSlotsData()
{
	ReadSlotsFileLocal();
	return x2s_raw_local;
}

const std::string GetStageName(int ssid)
{
	uint32_t *ssid_map = (uint32_t *)new_ssid_to_idx;
	
	if (ssid < 0 || ssid >= (int)x2sta.GetNumSsStages() || !ssid_map)
		return "Unknown Stage";	
	
	uint32_t id = ssid_map[ssid];
	int lang = *game_language;	
	
	if (lang < 0 || lang >= XV2_NATIVE_LANG_NUM)
		return "Unknown language";
	
	//DPRINTF("Language: %d. String: %s\n", lang, x2sta[id].GetNativeName(lang).c_str());
	
	return x2sta[id].GetNativeName(lang);
}

const std::string GetStageImageString(int ssid)
{
	uint32_t *ssid_map = (uint32_t *)new_ssid_to_idx;
	
	if (ssid < 0 || ssid >= (int)x2sta.GetNumSsStages() || !ssid_map)
		return "";	
	
	uint32_t id = ssid_map[ssid];	
	const Xv2Stage &stage = x2sta[id];
	
	return std::string("IMG_STAGE02_") + stage.code;
}

const std::string GetStageIconString(int ssid)
{
	uint32_t *ssid_map = (uint32_t *)new_ssid_to_idx;
	
	if (ssid < 0 || ssid >= (int)x2sta.GetNumSsStages() || !ssid_map)
		return "";	
	
	uint32_t id = ssid_map[ssid];	
	const Xv2Stage &stage = x2sta[id];
	
	return std::string("IMG_STAGE02_ICO") + stage.code;
}

int StageHasBeenSelectedBefore(int ssid)
{
	uint32_t *ssid_map = (uint32_t *)new_ssid_to_idx;
	
	if (ssid < 0 || ssid >= (int)x2sta.GetNumSsStages() || !ssid_map)
		return 0;	
	
	uint32_t id = ssid_map[ssid];
	return (int)HasStageBeenSelectedBefore(GetSelectedBeforeObject(*game_data, -1), id);
}

} // extern "C"