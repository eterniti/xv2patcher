#pragma once

#include "CusFile.h"
#include "BcmFile.h"

#include "vc_defs.h"

#define MAX_MOBS			14
#define MAX_MOBS_HUD		7

enum
{
	SKILL_SUPER1,
	SKILL_SUPER2,
	SKILL_SUPER3,
	SKILL_SUPER4,
	SKILL_ULTIMATE1,
	SKILL_ULTIMATE2,
	SKILL_EVASIVE,
	SKILL_BLAST,
	SKILL_AWAKEN,
	SKILL_NUM
};

#define COMMAND_SUPER1_START	1500
#define COMMAND_SUPER1_END		1599
#define COMMAND_SUPER2_START	1600
#define COMMAND_SUPER2_END		1699
#define COMMAND_SUPER3_START	1700
#define COMMAND_SUPER3_END		1799
#define COMMAND_SUPER4_START	1800
#define COMMAND_SUPER4_END		1899
#define COMMAND_ULTRA1_START	1900
#define COMMAND_ULTRA1_END		1999
#define COMMAND_ULTRA2_START	2000
#define COMMAND_ULTRA2_END		2099
#define COMMAND_EVASIVE_START	2100
#define COMMAND_EVASIVE_END		2199
#define COMMAND_BLAST_START		2200
#define COMMAND_BLAST_END		2299
#define COMMAND_AWAKEN_FIRST	2300
#define COMMAND_AWAKEN_SECOND	2301
#define COMMAND_AWAKEN_THIRD	2302
#define COMMAND_AWAKEN_RETURN	2303

// (Game object)
// Size 0x10
struct SkillSlot
{
	CUSSkill *skill; // as defined in CusFile.h
	uint64_t unk_08;
};
CHECK_STRUCT_SIZE(SkillSlot, 0x10);

// (Game object) XG::Game::Battle::Command
// Size unknown
struct Battle_Command
{
	void **vtbl; // 000
	uint8_t unk_08[0xDC0-8];
	BCMEntry bcm; // 0DC0 - trans_modifier is at 0x64  - Maybe not all the entry is stored
	// ...
};
CHECK_FIELD_OFFSET(Battle_Command, bcm, 0xDC0);

// (Game object) XG::Game::Battle::Mob
// Size: 0x2850 (1.08)
struct Battle_Mob
{
	void **vtbl; // 0000
	uint8_t unk_08[0x50-8];
	uint32_t is_cpu; // 0050 - 1 if controlled by AI
	uint8_t unk_54[0xB0-0x54];
	uint8_t flags; // 00B0  Known flag: 1 -> cac 0x10 -> character transformed?
	uint8_t unk_B1[0xB8-0xB1];
	uint32_t default_partset; // 00B8 (the initial partset)
	uint32_t cms_id; // 00BC
	uint8_t unk_C0[0xF8-0xC0];
	float hp; // 00F8
	uint32_t unk_FC; 
	uint32_t unk_100;
	float ki;	// 0104
	uint8_t unk_108[0x164-0x108];
	float stamina; // 0164
	uint8_t unk_168[0x260-0x168];
	SkillSlot skills[SKILL_NUM]; // 0260
	uint8_t unk_2F0[0x3A0-0x2F0]; 
	int32_t unk_interface_var; // 03A0  Somehow controls the portrait and audio? We need this for the "Take control of ally" functionality
	uint8_t unk_3A4[0x4A8-0x3A4];
	Battle_Command *battle_command; // 04A8
	uint8_t unk_4B0[0x1F64-0x4B0]; 
	int32_t loaded_var; // 1F64 ; if >= 0, char is loaded
	uint8_t unk_1D18[0x2114-0x1F68];
	int32_t trans_control; // 2114
	// ...
	
	inline bool IsCac() const
	{
		return (flags & 1);
	}
	
	inline bool IsTransformed() const
	{
		return (flags & 0x10);
	}
};
CHECK_FIELD_OFFSET(Battle_Mob, is_cpu, 0x50);
CHECK_FIELD_OFFSET(Battle_Mob, flags, 0xB0);
CHECK_FIELD_OFFSET(Battle_Mob, default_partset, 0xB8);
CHECK_FIELD_OFFSET(Battle_Mob, cms_id, 0xBC);
CHECK_FIELD_OFFSET(Battle_Mob, hp, 0xF8);
CHECK_FIELD_OFFSET(Battle_Mob, ki, 0x104);
CHECK_FIELD_OFFSET(Battle_Mob, stamina, 0x164);
CHECK_FIELD_OFFSET(Battle_Mob, skills, 0x260);
CHECK_FIELD_OFFSET(Battle_Mob, unk_interface_var, 0x3A0);
CHECK_FIELD_OFFSET(Battle_Mob, battle_command, 0x4A8);
CHECK_FIELD_OFFSET(Battle_Mob, loaded_var, 0x1F64);
CHECK_FIELD_OFFSET(Battle_Mob, trans_control, 0x2114);

struct Battle_HudCharInfo
{
	uint32_t unk_00;
	uint32_t is_cac; // 004
	int32_t index; // 008
	uint32_t cms_entry; // 00C
	
	StdU16String name; // 10
	
	uint8_t unk_30[0x3C-0x30];
	uint32_t cms_entry2; // 03C  WHY there is another one?
	uint8_t unk_40[0x180-0x40];
	
	inline const char16_t *GetName() const
	{
		return name.CStr();
	}
	
};
CHECK_STRUCT_SIZE(Battle_HudCharInfo, 0x180);
CHECK_FIELD_OFFSET(Battle_HudCharInfo, is_cac, 4);
CHECK_FIELD_OFFSET(Battle_HudCharInfo, index, 8);
CHECK_FIELD_OFFSET(Battle_HudCharInfo, cms_entry, 0xC);
CHECK_FIELD_OFFSET(Battle_HudCharInfo, name, 0x10);
CHECK_FIELD_OFFSET(Battle_HudCharInfo, cms_entry2, 0x3C);

// (Game object) XG::Game::Battle::HudCockpit
// Size unknown
struct Battle_HudCockpit
{
	uint8_t unk_00[0x540]; 
	Battle_HudCharInfo char_infos[MAX_MOBS]; // 1.16, changed from offset 0x4F0 to 0x540
	// ...
};
CHECK_FIELD_OFFSET(Battle_HudCockpit, char_infos, 0x540);

// 1.10 structure grow from 0x1D0 to 0x1D4. team variable went from offset 0x10 to 0x14
// 1.13v2 structure grow from 0x1D4 to 0x1D8
// 1.14 structure grow from 0x1D8 to 0x1DC
struct PACKED UnkMobStruct
{
	uint8_t unk_00[0x18];
	uint32_t team; //  0x18 -  1 or 2
	uint8_t unk_14[0x1DC-0x1C];
};
CHECK_STRUCT_SIZE(UnkMobStruct, 0x1DC);
CHECK_FIELD_OFFSET(UnkMobStruct, team, 0x18);

// (Game object) XG::Game::Battle::Core::MainSystem
// Size 0x9BF0 (1.10v2)
struct Battle_Core_MainSystem
{
	void **vtbl; // 0000
	uint8_t unk_08[0xDC-8];
	UnkMobStruct unk_mob_data[MAX_MOBS]; // 00DC	
	uint8_t unk_1AE4[0x37F8-0x1AE4]; 
	Battle_Mob *mobs[MAX_MOBS]; // 37F8
	uint8_t unk_3868[0x40F8-0x3868];
	Battle_HudCockpit *cockpit; // 40F8
	//...
};
CHECK_FIELD_OFFSET(Battle_Core_MainSystem, unk_mob_data, 0xDC);
CHECK_FIELD_OFFSET(Battle_Core_MainSystem, mobs, 0x37F8);
CHECK_FIELD_OFFSET(Battle_Core_MainSystem, cockpit, 0x40F8);

// (Game object)
// Size unknown
struct SelectCharInfo
{
	uint32_t char_index; // 00
	uint32_t char_index2; // 04 - what is the difference with above?
	uint32_t unk_08; // seen FFFFFFFF
	uint32_t is_cpu; // 0C - 1 if controlled by AI
	uint32_t team; // 10  - 1 or 2
	// ...
};
CHECK_FIELD_OFFSET(SelectCharInfo, is_cpu, 0xC);
CHECK_FIELD_OFFSET(SelectCharInfo, team, 0x10);

// (Game object) XG::Game::Battle::AIBehaviourSpecial
// Size unknown
struct AIBehaviourSpecial
{
	void **vtbl;
	// ...
};

// (Game object)
// Size unknown
struct AIDef
{
	uint8_t unk_00[0x10]; 
	Battle_Mob *mob; // 0010
	uint8_t unk_18[0x28-0x18]; 
	uint32_t ai_decision; // 0028
	uint8_t unk_2C[0x154-0x2C]; 
	uint32_t type;	// 0x154 - A number between [0-10], both included. Seem to be 2 when long range attack? or maybe means a single-step attack.
	//...
};
CHECK_FIELD_OFFSET(AIDef, mob, 0x10);
CHECK_FIELD_OFFSET(AIDef, ai_decision, 0x28);
CHECK_FIELD_OFFSET(AIDef, type, 0x154);

// (Game object)
// Size 0x180

