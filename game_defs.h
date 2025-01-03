#pragma once

#include "CusFile.h"
#include "BcmFile.h"
#include "PatchUtils.h"

#include "vc_defs.h"

#define MAX_MOBS			14
#define MAX_MOBS_HUD		7
#define MAX_PORTRAITS		64

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

struct CharaResourcePartsetsRequest
{
	uint32_t cms_id; // 00
	uint8_t unk_04[0x10-4]; 
	StdVector<uint32_t> partsets; // 10
	// ...
};
CHECK_FIELD_OFFSET(CharaResourcePartsetsRequest, cms_id, 0);
CHECK_FIELD_OFFSET(CharaResourcePartsetsRequest, partsets, 0x10);

#define CHANGE_PARTSET_VIRTUAL 0xD0

struct CommonChara
{
	void **vtbl; // 0000
	uint8_t unk_08[0x58C-8]; 
	uint32_t current_partset; // 058C
	// ...
	
	inline void ChangePartset(uint32_t partset)
	{
		PatchUtils::InvokeVirtualRegisterFunction(this, CHANGE_PARTSET_VIRTUAL, partset);
	}
};
CHECK_FIELD_OFFSET(CommonChara, current_partset, 0x58C);

// (Game object) XG::Game::Battle::Mob
// Size: 0x2850 (1.08)
// Update 1.20: +4 displacement of multiple fields that had been stable for long time (e.g. hp F8 -> FC)
// Update 1.21: again displacement of multiple fields
struct Battle_Mob
{
	void **vtbl; // 0000
	uint8_t unk_08[0x50-8];
	uint32_t is_cpu; // 0050 - 1 if controlled by AI
	uint8_t unk_54[0xB0-0x54];
	uint8_t flags; // 00B0  Known flag: 1 -> cac 0x10 -> character transformed
	uint8_t unk_B1[0xB4-0xB1];
	uint32_t unk_B4; // Seems cms id as well. Probably one of them is the initial cms, and the other the current one, as skills like boo race transformation can change cms
	uint32_t default_partset; // 00B8 (the initial partset)
	uint32_t cms_id; // 00BC
	uint32_t unk_C0; // Seems the same as initial partset? doesn't change with transforms. It seems to be continiously accessed in battle
	int32_t body; // C4 (filled only for cacs)
	uint8_t unk_C8[0x100-0xC8];
	float hp; // 0100
	float hp_start; // 0104 assume this one is always below the previous one
	uint32_t unk_108;
	float ki;	// 010C
	uint8_t unk_110[0x16C-0x110];
	float stamina; // 016C
	uint8_t unk_16C[0x268-0x170];
	SkillSlot skills[SKILL_NUM]; // 0268
	uint8_t unk_2F8[0x3B0-0x2F8]; 
	int32_t unk_interface_var; // 03B0  Somehow controls the portrait and audio? We need this for the "Take control of ally" functionality
	uint8_t unk_3B4[0x4C8-0x3B4];
	CommonChara *common_chara; // 4C8 - XG::Game::Common::Chara, the one that has the BCS file content inside
	uint64_t unk_4D0;
	Battle_Command *battle_command; // 04D8
	uint8_t unk_4E0[0x2094-0x4E0]; 
	int32_t loaded_var; // 2094 ; if >= 0, char is loaded
	uint8_t unk_2088[0x2248-0x2098];
	uint32_t trans_partset; // 2248
	int32_t trans_control; // 224C
	// ...
	// ...
	
	inline bool IsCac() const
	{
		return (flags & 1);
	}
	
	inline bool IsTransformed1() const
	{
		return (flags & 0x10);
	}
	
	inline bool IsTransformed2() const
	{
		return (trans_control >= 0);
	}
	
	inline bool IsTransformed() const
	{
		return (IsTransformed1() || IsTransformed2());
	}

	inline uint32_t GetCurrentPartset()
	{
		if (!common_chara)
			return default_partset;
		
		return common_chara->current_partset;
	}
	
	inline void ChangePartset(uint32_t partset)
	{
		if (!common_chara)
			return;
		
		common_chara->ChangePartset(partset);
	}
};
CHECK_FIELD_OFFSET(Battle_Mob, is_cpu, 0x50);
CHECK_FIELD_OFFSET(Battle_Mob, flags, 0xB0);
CHECK_FIELD_OFFSET(Battle_Mob, default_partset, 0xB8);
CHECK_FIELD_OFFSET(Battle_Mob, cms_id, 0xBC);
CHECK_FIELD_OFFSET(Battle_Mob, body, 0xC4);
CHECK_FIELD_OFFSET(Battle_Mob, hp, 0x100);
CHECK_FIELD_OFFSET(Battle_Mob, ki, 0x10C);
CHECK_FIELD_OFFSET(Battle_Mob, stamina, 0x16C);
CHECK_FIELD_OFFSET(Battle_Mob, skills, 0x268);
CHECK_FIELD_OFFSET(Battle_Mob, unk_interface_var, 0x3B0);
CHECK_FIELD_OFFSET(Battle_Mob, common_chara, 0x4C8);
CHECK_FIELD_OFFSET(Battle_Mob, battle_command, 0x4D8);
CHECK_FIELD_OFFSET(Battle_Mob, loaded_var, 0x2094);
CHECK_FIELD_OFFSET(Battle_Mob, trans_partset, 0x2248);
CHECK_FIELD_OFFSET(Battle_Mob, trans_control, 0x224C);

// 1.20. size 0x180 -> 0x190
// 1.21 size 0x190 -> 0x1A0
struct Battle_HudCharInfo
{
	uint32_t unk_00;
	uint32_t is_cac; // 004
	int32_t index; // 008
	uint32_t cms_entry; // 00C
	
	StdU16String name; // 10
	
	uint8_t unk_30[0x3C-0x30];
	uint32_t cms_entry2; // 03C  WHY there is another one?
	uint8_t unk_40[0x1A0-0x40];
	
	inline const char16_t *GetName() const
	{
		return name.CStr();
	}
	
};
CHECK_STRUCT_SIZE(Battle_HudCharInfo, 0x1A0);
CHECK_FIELD_OFFSET(Battle_HudCharInfo, is_cac, 4);
CHECK_FIELD_OFFSET(Battle_HudCharInfo, index, 8);
CHECK_FIELD_OFFSET(Battle_HudCharInfo, cms_entry, 0xC);
CHECK_FIELD_OFFSET(Battle_HudCharInfo, name, 0x10);
CHECK_FIELD_OFFSET(Battle_HudCharInfo, cms_entry2, 0x3C);

// (Game object) XG::Game::Battle::HudCockpit
// Size unknown
struct Battle_HudCockpit
{
	uint8_t unk_00[0x580]; 
	Battle_HudCharInfo char_infos[MAX_MOBS]; // 1.16, changed from offset 0x4F0 to 0x540; 1.21: changed from offset 0x540 to 0x580
	uint8_t unk_1C40[0x1EE10-0x1C40];
	int32_t portrait_cms[MAX_PORTRAITS];
	// ...
};
CHECK_FIELD_OFFSET(Battle_HudCockpit, char_infos, 0x580);
CHECK_FIELD_OFFSET(Battle_HudCockpit, portrait_cms, 0x1EE10);

// 1.10 structure grow from 0x1D0 to 0x1D4. team variable went from offset 0x10 to 0x14
// 1.13v2 structure grow from 0x1D4 to 0x1D8
// 1.14 structure grow from 0x1D8 to 0x1DC
// 1.21 structure grow from 0x1DC to 0x1F0
struct PACKED UnkMobStruct
{
	uint8_t unk_00[0x18];
	uint32_t team; //  0x18 -  1 or 2
	uint8_t unk_14[0x1F0-0x1C];
};
CHECK_STRUCT_SIZE(UnkMobStruct, 0x1F0);
CHECK_FIELD_OFFSET(UnkMobStruct, team, 0x18);

// (Game object) XG::Game::Battle::Core::MainSystem
// Size 0x9BF0 (1.10v2)
struct Battle_Core_MainSystem
{
	void **vtbl; // 0000
	uint8_t unk_08[0xE0-8];
	UnkMobStruct unk_mob_data[MAX_MOBS]; // 00E0	
	uint8_t unk_1C00[0x3A58-0x1C00]; 
	Battle_Mob *mobs[MAX_MOBS]; // 3A58
	uint8_t unk_3AC8[0x4358-0x3AC8];
	Battle_HudCockpit *cockpit; // 4358
	//...
};
CHECK_FIELD_OFFSET(Battle_Core_MainSystem, unk_mob_data, 0xE0);
CHECK_FIELD_OFFSET(Battle_Core_MainSystem, mobs, 0x3A58);
CHECK_FIELD_OFFSET(Battle_Core_MainSystem, cockpit, 0x4358);

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
// 1.20.1 mob, 0x10 -> 0x28
// 1.20.1 ai_decision, 0x28 -> 0x40
// 1.20.1 type, 0x154 -> 0x174
struct AIDef
{
	uint8_t unk_00[0x28]; 
	Battle_Mob *mob; // 0028
	uint8_t unk_30[0x40-0x30]; 
	uint32_t ai_decision; // 0040
	uint8_t unk_44[0x174-0x44]; 
	uint32_t type;	// 0x174 - A number between [0-10], both included. Seem to be 2 when long range attack? or maybe means a single-step attack.
	//...
};
CHECK_FIELD_OFFSET(AIDef, mob, 0x28);
CHECK_FIELD_OFFSET(AIDef, ai_decision, 0x40);
CHECK_FIELD_OFFSET(AIDef, type, 0x174);

struct BattleInterface
{
	uint8_t unk_00[0x14];
	uint32_t gbb_mode; // 0014;  tentative name, it may have other uses?
	//...
	
	inline bool IsGbbMode() const
	{
		return (gbb_mode == 1); // Note: specifically checking for 1 instead of "not zero"
	}
};
CHECK_FIELD_OFFSET(BattleInterface, gbb_mode, 0x14);

struct QuestManager
{
	uint8_t unk_00[0xA0];
	uint32_t mode; // 0xA0; tentative name.
	// ...
};
CHECK_FIELD_OFFSET(QuestManager, mode, 0xA0);


// (Game object)
// Size 0x180

