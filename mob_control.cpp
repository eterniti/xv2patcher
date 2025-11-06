#include <unordered_set>

#include "mob_control.h"
#include "xv2patcher.h"
#include "stage_patch.h"
#include "debug.h"

typedef void (* ChangeBodyType)(Battle_Core_MainSystem *, int, int);
typedef Battle_Mob *(* GetMobControlType)(void *);
typedef bool (* UsesMobControlType)(void *);
typedef void *(* GetEventCastComFromIndexType)(int);
typedef void (* MobUntransformType)(Battle_Mob *, int);

static ChangeBodyType ChangeBody;
static GetMobControlType GetMobControl;
static UsesMobControlType UsesMobControl;
static GetEventCastComFromIndexType GetEventCastComFromIndex;
static MobUntransformType MobUntransform;

static std::unordered_set<Battle_Mob *> controlled_mobs;

static Battle_Mob *original_player=nullptr, *controlled_mob=nullptr;
static int original_player_idx=-1;
static bool null_next_event_cast_com=false;
static bool taking_control=false;
static bool allow_control_enemies=false;

static Battle_Mob *GetSinglePlayer(int *index, uint32_t *team)
{
	Battle_Mob *result = nullptr;
	
	if (!pms_singleton || !(*pms_singleton))
		return result;
	
	for (int i = 0; i < MAX_MOBS; i++)
	{
		Battle_Mob *mob = (*pms_singleton)->mobs[i];
		
		if (!mob)
			continue;
		
		if (!mob->is_cpu)
		{
			if (result) // More than one human player
				return nullptr; 
			
			result = mob;
			
			if (index)
				*index = i;
			
			if (team)
				*team = (*pms_singleton)->unk_mob_data[i].team;
		}
	}
	
	return result;
}

extern "C"
{
	
PUBLIC void SetupChangeBody(ChangeBodyType orig)
{
	ChangeBody = orig;
	
	ini.GetBooleanValue("Patches", "take_enemy_control", &allow_control_enemies, false);
}

static inline Battle_Mob *GetMobFromCharData(void *ptr)
{
	uintptr_t mob = (uintptr_t)ptr - offsetof(Battle_Mob, flags);	
	return (Battle_Mob *)mob;
}

PUBLIC void SetupUsesMobControl(UsesMobControlType orig)
{
	UsesMobControl = orig;
}

PUBLIC void SetupGetMobControl(GetMobControlType orig)
{
	GetMobControl = orig;
}

PUBLIC bool UsesMobControlPatched(void *ptr)
{
	bool ret = UsesMobControl(ptr);
	
	if (ret)
	{
		Battle_Mob *mob = GetMobFromCharData(ptr);
		
		if (controlled_mobs.find(mob) != controlled_mobs.end())
			ret = false;
		else
		{
			DPRINTF("Returning true for 0x%x\n", mob->cms_id);
		}
	}
	
	return ret;
}

PUBLIC Battle_Mob *GetMobControlPatched(void *ptr)
{
	Battle_Mob *ret = GetMobControl(ptr);
	
	if (ret)
	{
		Battle_Mob *mob = GetMobFromCharData(ptr);
		
		if (controlled_mobs.find(mob) != controlled_mobs.end())
			ret = nullptr;
	}
	
	return ret;
}

static int IsPlayerEventPatched(uint64_t **pthis)
{
	if (controlled_mob)
	{
		null_next_event_cast_com = true;
		return 0;
	}
	
	// Original method
	typedef int (* IsPlayerEventType)(void *);
	
	uint64_t *vtable = *pthis;
	IsPlayerEventType IsPlayerEvent = (IsPlayerEventType) vtable[0x2E8/8];
	
	return IsPlayerEvent(pthis);
}

PUBLIC void OnDialogueIsPlayerLocated(uint8_t *addr)
{
	PatchUtils::Write16(addr, 0xE890);
	PatchUtils::HookCall(addr+1, nullptr, (void *)IsPlayerEventPatched);
}

PUBLIC void SetupGetEventCastComFromIndex(GetEventCastComFromIndexType orig)
{
	GetEventCastComFromIndex = orig;
}

PUBLIC void *GetEventCastComFromIndexPatched(int index)
{
	if (null_next_event_cast_com)
	{
		null_next_event_cast_com = false;
		return nullptr;
	}
	
	return GetEventCastComFromIndex(index);
}

PUBLIC void SetupMobUntransformForMobControl(MobUntransformType orig)
{
	MobUntransform = orig;
}

PUBLIC void DummyMobUntransformForMobControl(Battle_Mob *pthis, int param)
{
	if (taking_control)
		return;
	
	MobUntransform(pthis, param);
}

std::string GetPlayerAllies()
{
	if (!ChangeBody) // Patch not enabled
		return "";
	
	uint32_t player_team;
	Battle_Mob *player = GetSinglePlayer(nullptr, &player_team);
	
	// Debug
	//DPRINTF("Player is %p\%n", player);
	///////////
	
	if (!player)
		return "";
	
	std::string ret;
	
	for (int i = 0; i < MAX_MOBS; i++)
	{
		Battle_Mob *mob = (*pms_singleton)->mobs[i];
		
		if (!mob || mob == player)
			continue;
		
		/////////////////
		// Debug dump
		/*std::string name = GetMobName(i);
		if (name.length() > 0)
		{
			DPRINTF("%s is %p %p\n", name.c_str(), mob, ((*pms_singleton)->unk_mob_data + i));
			//Utils::WriteFileBool(name + ".bin", (uint8_t *)mob, 0x31D0);
			//Utils::WriteFileBool(name + ".unk", (uint8_t *)((*pms_singleton)->unk_mob_data + i), 0x1D0);
		}*/
		/////////////////
		
		if (((*pms_singleton)->unk_mob_data[i].team == player_team || allow_control_enemies) && mob->loaded_var >= 0)
		{
			if (ret.length() > 0)
				ret.push_back(',');
			
			ret += Utils::ToString(i);
		}
	}
	
	if (ret.length() > 0)
	{
		bool error;
		bool opened_gate = OpenedGateExist(&error);
		
		if (opened_gate || error)
		{
			ret = "!";
		}
	}
	
	return ret;
}

std::string GetMobName(int idx)
{
	if (idx < 0 || idx >= MAX_MOBS)
		return "";
	
	if (!pms_singleton || !(*pms_singleton))
		return "";
	
	if (!(*pms_singleton)->mobs[idx])
		return nullptr;
	
	if (!(*pms_singleton)->cockpit)
		return nullptr;
	
	const Battle_HudCharInfo &info = (*pms_singleton)->cockpit->char_infos[idx];
	if (info.index < 0)
		return "";
	
	const char16_t *name = info.GetName();
	if (!name)
		return "";
	
	return Utils::Ucs2ToUtf8(name);
}

void TakeControlOfMob(int idx)
{
	if (!ChangeBody)
		return;
	
	if (idx < 0 || idx >= MAX_MOBS)
		return;
	
	int player_idx;
	Battle_Mob *player = GetSinglePlayer(&player_idx, nullptr);
	
	if (!player || player_idx == idx)
		return;
	
	Battle_Mob *other_mob = (*pms_singleton)->mobs[idx];
	if (!other_mob)
		return;
	
	if (original_player && original_player_idx >= 0)
	{
		if (idx != original_player_idx && player_idx != original_player_idx)
		{
			ChangeBody(*pms_singleton, player_idx, original_player_idx);
			
			player->unk_interface_var = -1;
			original_player->unk_interface_var = -1;	

			player = original_player;
			player_idx = original_player_idx;
		}
	}
	else
	{
		original_player = player;
		original_player_idx = player_idx;
	}

	taking_control = true;
	ChangeBody(*pms_singleton, player_idx, idx);
	taking_control = false;
	
	if (other_mob == original_player)
		controlled_mob = nullptr;
	else
		controlled_mob = other_mob;
	
	DPRINTF("Took control of %p\n", other_mob);	
	DPRINTF("Mob var: %d %d\n", player_idx, idx);
	
	controlled_mobs.insert(player);
	controlled_mobs.insert(other_mob);
	
	// They solved the problem of main portrait and audio, but causes another problem: in quest, the new char the human is controlling gets an auto-protect
	// We now used other patches instead of this -1 thing
	/*player->unk_interface_var = -1;
	other_mob->unk_interface_var = -1;*/
}

void OnDeletedMob_Control(Battle_Mob *mob)
{
	controlled_mobs.erase(mob);
	
	if (mob == original_player)
	{
		original_player = nullptr;
		original_player_idx = -1;
	}

	if (mob == controlled_mob)
	{
		controlled_mob = nullptr;
	}
}

	
} // extern "C"