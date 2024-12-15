#include <unordered_map>

#include "ai_patch.h"
#include "xv2patcher.h"
#include "debug.h"

typedef void (* _OnBattleCreateChar)(Battle_Core_MainSystem *, SelectCharInfo *, uint32_t);
typedef uint32_t (* _AISpecial)(AIBehaviourSpecial *, void *, AIDef *);
typedef void (* _SkillCommand)(void *, uint32_t cmd);

static _OnBattleCreateChar OnBattleCreateChar;
static _AISpecial AISpecial;
static _SkillCommand SkillCommand;

static std::unordered_map<Battle_Mob *, AICharacter *> ai_characters;
static int chance_ally_T1[NUM_HP_STATES], chance_ally_T2[NUM_HP_STATES], chance_ally_T3[NUM_HP_STATES];
static int chance_enemy_T1[NUM_HP_STATES], chance_enemy_T2[NUM_HP_STATES], chance_enemy_T3[NUM_HP_STATES];

static inline bool IsCmdSuper1(uint32_t cmd)
{
	return (cmd >= COMMAND_SUPER1_START && cmd <= COMMAND_SUPER1_END);
}

static inline bool IsCmdSuper2(uint32_t cmd)
{
	return (cmd >= COMMAND_SUPER2_START && cmd <= COMMAND_SUPER2_END);
}

static inline bool IsCmdSuper3(uint32_t cmd)
{
	return (cmd >= COMMAND_SUPER3_START && cmd <= COMMAND_SUPER3_END);
}

static inline bool IsCmdSuper4(uint32_t cmd)
{
	return (cmd >= COMMAND_SUPER4_START && cmd <= COMMAND_SUPER4_END);
}

static inline bool IsCmdSuper(uint32_t cmd)
{
	return (cmd >= COMMAND_SUPER1_START && cmd <= COMMAND_SUPER4_END);
}

static inline bool IsCmdUltra1(uint32_t cmd)
{
	return (cmd >= COMMAND_ULTRA1_START && cmd <= COMMAND_ULTRA1_END);
}

static inline bool IsCmdUltra2(uint32_t cmd)
{
	return (cmd >= COMMAND_ULTRA2_START && cmd <= COMMAND_ULTRA2_END);
}

static inline bool IsCmdUltra(uint32_t cmd)
{
	return (cmd >= COMMAND_ULTRA1_START && cmd <= COMMAND_ULTRA2_END);
}

static inline bool IsCmdSuperOrUltra(uint32_t cmd)
{
	return (cmd >= COMMAND_SUPER1_START && cmd <= COMMAND_ULTRA2_END);
}

static inline bool IsCmdEvasive(uint32_t cmd)
{
	return (cmd >= COMMAND_EVASIVE_START && cmd <= COMMAND_EVASIVE_END);
}

static inline bool IsCmdBlast(uint32_t cmd)
{
	return (cmd >= COMMAND_BLAST_START && cmd <= COMMAND_BLAST_END);
}

static inline bool IsCmdAwaken(uint32_t cmd)
{
	return (cmd >= COMMAND_AWAKEN_FIRST && cmd <= COMMAND_AWAKEN_RETURN);
}

// This function is an imitation of 0x124C60 (1.09 address)
static inline int AIDecisionToSkillSlot(uint32_t decision)
{
	switch (decision)
	{
		case 8:
			return 0;
			
		case 9:
			return 1;
			
		case 10:
			return 2;
			
		case 11:
			return 3;
			
		case 12:
			return 4;
			
		case 13:
			return 5;
		
		case 5:
			return 6;
			
		case 96:
			return 7;
	}
	
	return -1;
}

extern "C"
{

PUBLIC void SetupAIExtend(_OnBattleCreateChar orig)
{
	OnBattleCreateChar = orig;
	
	ini.GetIntegerValue("AIextend", "chance_ally_T1_100p", &chance_ally_T1[HP_STATE_100P], 30);
	ini.GetIntegerValue("AIextend", "chance_ally_T1_80p", &chance_ally_T1[HP_STATE_80P], 45);
	ini.GetIntegerValue("AIextend", "chance_ally_T1_60p", &chance_ally_T1[HP_STATE_60P], 60);
	ini.GetIntegerValue("AIextend", "chance_ally_T1_40p", &chance_ally_T1[HP_STATE_40P], 75);
	ini.GetIntegerValue("AIextend", "chance_ally_T1_20p", &chance_ally_T1[HP_STATE_20P], 90);
	
	ini.GetIntegerValue("AIextend", "chance_ally_T2_100p", &chance_ally_T2[HP_STATE_100P], 18);
	ini.GetIntegerValue("AIextend", "chance_ally_T2_80p", &chance_ally_T2[HP_STATE_80P], 33);
	ini.GetIntegerValue("AIextend", "chance_ally_T2_60p", &chance_ally_T2[HP_STATE_60P], 48);
	ini.GetIntegerValue("AIextend", "chance_ally_T2_40p", &chance_ally_T2[HP_STATE_40P], 63);
	ini.GetIntegerValue("AIextend", "chance_ally_T2_20p", &chance_ally_T2[HP_STATE_20P], 78);
	
	ini.GetIntegerValue("AIextend", "chance_ally_T3_100p", &chance_ally_T3[HP_STATE_100P], 8);
	ini.GetIntegerValue("AIextend", "chance_ally_T3_80p", &chance_ally_T3[HP_STATE_80P], 23);
	ini.GetIntegerValue("AIextend", "chance_ally_T3_60p", &chance_ally_T3[HP_STATE_60P], 38);
	ini.GetIntegerValue("AIextend", "chance_ally_T3_40p", &chance_ally_T3[HP_STATE_40P], 53);
	ini.GetIntegerValue("AIextend", "chance_ally_T3_20p", &chance_ally_T3[HP_STATE_20P], 68);
	
	ini.GetIntegerValue("AIextend", "chance_enemy_T1_100p", &chance_enemy_T1[HP_STATE_100P], 30);
	ini.GetIntegerValue("AIextend", "chance_enemy_T1_80p", &chance_enemy_T1[HP_STATE_80P], 45);
	ini.GetIntegerValue("AIextend", "chance_enemy_T1_60p", &chance_enemy_T1[HP_STATE_60P], 60);
	ini.GetIntegerValue("AIextend", "chance_enemy_T1_40p", &chance_enemy_T1[HP_STATE_40P], 75);
	ini.GetIntegerValue("AIextend", "chance_enemy_T1_20p", &chance_enemy_T1[HP_STATE_20P], 90);
	
	ini.GetIntegerValue("AIextend", "chance_enemy_T2_100p", &chance_enemy_T2[HP_STATE_100P], 18);
	ini.GetIntegerValue("AIextend", "chance_enemy_T2_80p", &chance_enemy_T2[HP_STATE_80P], 33);
	ini.GetIntegerValue("AIextend", "chance_enemy_T2_60p", &chance_enemy_T2[HP_STATE_60P], 48);
	ini.GetIntegerValue("AIextend", "chance_enemy_T2_40p", &chance_enemy_T2[HP_STATE_40P], 63);
	ini.GetIntegerValue("AIextend", "chance_enemy_T2_20p", &chance_enemy_T2[HP_STATE_20P], 78);
	
	ini.GetIntegerValue("AIextend", "chance_enemy_T3_100p", &chance_enemy_T3[HP_STATE_100P], 8);
	ini.GetIntegerValue("AIextend", "chance_enemy_T3_80p", &chance_enemy_T3[HP_STATE_80P], 23);
	ini.GetIntegerValue("AIextend", "chance_enemy_T3_60p", &chance_enemy_T3[HP_STATE_60P], 38);
	ini.GetIntegerValue("AIextend", "chance_enemy_T3_40p", &chance_enemy_T3[HP_STATE_40P], 53);
	ini.GetIntegerValue("AIextend", "chance_enemy_T3_20p", &chance_enemy_T3[HP_STATE_20P], 68);
	
	/*ini.GetIntegerValue("AIextend", "chance_enemy_T3_100p", &chance_enemy_T3[HP_STATE_100P], 99);
	ini.GetIntegerValue("AIextend", "chance_enemy_T3_80p", &chance_enemy_T3[HP_STATE_80P], 99);
	ini.GetIntegerValue("AIextend", "chance_enemy_T3_60p", &chance_enemy_T3[HP_STATE_60P], 99);
	ini.GetIntegerValue("AIextend", "chance_enemy_T3_40p", &chance_enemy_T3[HP_STATE_40P], 99);
	ini.GetIntegerValue("AIextend", "chance_enemy_T3_20p", &chance_enemy_T3[HP_STATE_20P], 99);*/
}

PUBLIC void SetupAISpecial(_AISpecial orig)
{
	AISpecial = orig;
}

PUBLIC void SetupSkillCommand(_SkillCommand orig)
{
	SkillCommand = orig;
}

PUBLIC void OnBattleCreateCharPatched(Battle_Core_MainSystem *pthis, SelectCharInfo *info, uint32_t arg3)
{
	OnBattleCreateChar(pthis, info, arg3);
	
	if (info->char_index >= MAX_MOBS)
		return;
	
	Battle_Mob *mob = pthis->mobs[info->char_index];	
	//DPRINTF("Mob 0x%x is %p  Team: %d\n", mob->cms_id, mob, info->team);
	
	//if (mob->is_cpu)
	if (1) // we now include player character because of the mob control functionality
	{
		CUSSkill *awaken = mob->skills[SKILL_AWAKEN].skill;
		
		if (!awaken)
		{
			//DPRINTF("Will ignore mob with cms 0x%x because it doesn't have awaken skill.\n", mob->cms_id);
			return;
		}
		
		int16_t num_transforms = (uint16_t)awaken->num_transforms;
		
		if (num_transforms <= 0)
		{
			//DPRINTF("Mob with cms 0x%x will be ignored because num_transforms=%d\n", mob->cms_id, num_transforms);
			return;
		}
		
		// Debug block
		// Beware, the awaken name can be 4 bytes with no null char, so we have to copy it
		/*std::string name;
		for (int i = 0; i < 4; i++)
			if (awaken->name[i] != 0)
				name.push_back(awaken->name[i]);
		
		DPRINTF("Mob %p with cms 0x%x uses skill %s. Partset=%d. Num stages=%d. Team: %d\n", mob, mob->cms_id, name.c_str(), awaken->partset, num_transforms, info->team); */
		//
		
		AICharacter *ai_char = new AICharacter(mob, (info->team == 2));		
		ai_characters[mob] = ai_char;		
	}	
}

void OnDeletedMob_AI(Battle_Mob *mob)
{
	//if (mob->is_cpu)
	if (1)
	{
		auto it = ai_characters.find(mob);
		
		if (it != ai_characters.end())
		{
			//DPRINTF("Deleting mob with cms 0x%x from list.\n", mob->cms_id);
			AICharacter *ai_char = it->second;
			
			ai_characters.erase(it);
			delete ai_char;
			
		}
	}
}

PUBLIC uint32_t AISpecialPatched(AIBehaviourSpecial *pthis, void *unk1, AIDef *def)
{
	if (def->type == 2)
	{	
		if (def->mob && def->mob->is_cpu)
		{
			auto it = ai_characters.find(def->mob);
			
			if (it != ai_characters.end())
			{
				int skill_slot = AIDecisionToSkillSlot(def->ai_decision);
					
				if (skill_slot >= SKILL_SUPER1 && skill_slot <= SKILL_ULTIMATE2)
				{				
					AICharacter *ai_char = it->second;
					
					if (ai_char->CanTransform() && !ai_char->flag_for_awaken_usage)
					{
						//DPRINTF("Flagged for awaken usage (skill slot %d)\n", skill_slot);
						ai_char->flag_for_awaken_usage = true;						
					}
				}
			}
		}
	}
	
	return AISpecial(pthis, unk1, def);
}

PUBLIC void SkillCommandPatched(uint64_t *pthis, uint32_t cmd)
{
	Battle_Mob *mob = (Battle_Mob *)pthis[0];
	
	if (IsCmdSuperOrUltra(cmd))
	{	
		if (mob && mob->is_cpu)
		{
			auto it = ai_characters.find(mob);
			
			if (it != ai_characters.end())
			{
				AICharacter *ai_char = it->second;
				
				if (ai_char->flag_for_awaken_usage)
				{
					uint32_t tick = GetTickCount();
					
					ai_char->flag_for_awaken_usage = false;
					
					if (tick >= (ai_char->last_tick+TRANS_WAIT) && ai_char->CanTransform())
					{
						int rnd = (int) Utils::RandomInt(0, 100);
						int trans;
						int *chance;
						bool decided = false;
						
						int current_transform = ai_char->GetCurrentTransform();
						//DPRINTF("Current transform of 0x%x is %d\n", mob->cms_id, current_transform);
						
						int max_t, ct;
						
						ct = current_transform;
						
						if (ai_char->num_transforms > 3)
						{
							max_t = 3;
							
							if (ct >= 2)
								ct = 1;
						}
						else
						{
							max_t = ai_char->num_transforms;							
						}
						
						for (int t = max_t-1; t > ct && !decided; t--)
						{
							if (t == 2)
							{
								// X -> Trans 3
								chance = (ai_char->is_enemy) ? chance_enemy_T3 : chance_ally_T3;
								trans = 2;
							}
							else if (t == 1)
							{
								// X -> Trans 2
								chance = (ai_char->is_enemy) ? chance_enemy_T2 : chance_ally_T2;
								trans = 1;
							}
							else
							{
								// X -> Trans 1
								chance = (ai_char->is_enemy) ? chance_enemy_T1 : chance_ally_T1;
								trans = 0;
							}

							for (int i = HP_STATE_20P; i <= NUM_HP_STATES; i++)
							{
								if (mob->hp <= ai_char->hp_states[i])
								{
									if (rnd < chance[i])
									{
										if (trans == 2 && ai_char->num_transforms > 3)
										{
											int start = current_transform;
											
											if (start < 2)
												start = 2;
											
											trans = Utils::RandomInt(start, ai_char->num_transforms);
											
											if (trans >= ai_char->num_transforms)
												trans = ai_char->num_transforms-1;
										}
										
										current_transform = trans;
										
										if (current_transform <= 2)
											cmd = COMMAND_AWAKEN_FIRST + current_transform;
										else
											cmd = COMMAND_AWAKEN_THIRD;
										
										BCMEntry *bcm = nullptr;
										
										if (mob->battle_command)
											bcm = &mob->battle_command->bcm;
										
										if (bcm)
										{
											bcm->ki_cost = 0;
											bcm->stamina_cost = 0;
											bcm->ki_required = 0;
											bcm->health_required = 0;
											bcm->trans_modifier = current_transform;
										}

										ai_char->last_tick = tick;
										
										//DPRINTF("0x%x will try to transform to %d\n", mob->cms_id, current_transform);
										decided = true;
									}
									
									break;
								}
							}
						}
					}
				}								
			}
		}
	}
	
	SkillCommand(pthis, cmd);
}

} // extern "C"