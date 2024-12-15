#pragma once

#include "xv2patcher.h"
#include "chara_patch.h"

enum
{
	HP_STATE_20P,
	HP_STATE_40P,
	HP_STATE_60P,
	HP_STATE_80P,
	HP_STATE_100P,
	NUM_HP_STATES
};

#define TRANS_WAIT	3800 /* Min time between transformation attemps in milisecs, to avoid spam */

extern "C" void OnDeletedMob_AI(Battle_Mob *);

// (Not game object)
struct AICharacter
{
	Battle_Mob *mob;
	int base_trans_control;
	int num_transforms;
	bool flag_for_awaken_usage;
	bool is_enemy;
	uint32_t last_tick;
			
	float max_hp, max_stamina, max_ki;
	
	float hp_states[NUM_HP_STATES];
	
	AICharacter(Battle_Mob *mob, bool is_enemy) : mob(mob), is_enemy(is_enemy)
	{
		CUSSkill *awaken = mob->skills[SKILL_AWAKEN].skill;
		
		if (awaken)
		{
			base_trans_control = (int16_t)awaken->aura;
			
			if (base_trans_control == 0xFFFF)
				base_trans_control = -1;
			
			int16_t num_transforms = (uint16_t)awaken->num_transforms; // Only lower part is num transforms, beware!
			this->num_transforms = num_transforms;
			
			if (this->num_transforms > 10)
				this->num_transforms = 10;
		}
		else
		{
			base_trans_control = -1;
			num_transforms = 0;
		}		

		flag_for_awaken_usage = false;
		last_tick = 0;
		max_hp = mob->hp;
		max_stamina = mob->stamina;
		max_ki = mob->ki;
		
		hp_states[HP_STATE_20P] = max_hp*0.2f;
		hp_states[HP_STATE_40P] = max_hp*0.4f;
		hp_states[HP_STATE_60P] = max_hp*0.6f;
		hp_states[HP_STATE_80P] = max_hp*0.8f;
		hp_states[HP_STATE_100P] = max_hp;
	}
	
	inline int GetCurrentTransform()
	{
		if (mob->trans_control < 0 || base_trans_control < 0)
			return -1;
		
		return (mob->trans_control - base_trans_control);
	}
	
	inline bool CanTransform()
	{
		return (mob->is_cpu && num_transforms > 0 && GetCurrentTransform() < (num_transforms-1));
	}
};
