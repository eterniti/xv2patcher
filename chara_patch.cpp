#include <windows.h>
#include <math.h>
#include <cmath>
#include <algorithm>
#include "PatchUtils.h"
#include "PscFile.h"
#include "AurFile.h"
#include "Xv2PreBakedFile.h"
#include "Xv2PatcherSlotsFile.h"
#include "xv2patcher.h"
#include "chara_patch.h"
#include "debug.h"

#include "Misc/IggyFile.h"

#include <unordered_set>
#include <unordered_map>

typedef void * (*IggyBaseCtorType)(void *, int32_t, int32_t);
typedef void *(* ChaselDtorType)(void *, uint32_t);
typedef void (* SendToAS3Type)(void *, int32_t, uint64_t);
typedef void (*Func1Type)(void *, uint32_t, uint32_t, void *, void *);
typedef void (*Func2Type)(void *, uint32_t, uint32_t, void *);
typedef int (* CheckUnlockType)(void *, int32_t, int32_t);
typedef uint64_t (* SetBodyShapeType)(void *, int32_t, uint32_t, float);
typedef void (*MobSetDefaultBodyType)(Battle_Mob *, uint32_t);

/*typedef uint64_t (* ResultPortraitsType)(uint64_t arg_rcx, uint64_t arg_rdx, uint32_t is_cac, uint32_t cms_entry, 
									uint64_t arg_20, uint64_t arg_28, uint64_t arg_30, uint64_t arg_38, uint64_t arg_40, uint64_t arg_48, uint64_t arg_50, uint64_t arg_58, uint64_t arg_60, uint64_t arg_68,
									uint64_t arg_70, uint64_t arg_78, uint64_t arg_80, uint64_t arg_88, uint64_t arg_90, uint64_t arg_98, uint64_t arg_A0, uint64_t arg_A8, uint64_t arg_B0, uint64_t arg_B8,
									uint64_t arg_C0, uint64_t arg_C8, uint64_t arg_D0, uint64_t arg_D8, uint64_t arg_E0, uint64_t arg_E8, uint64_t arg_F0, uint64_t arg_F8, uint64_t arg_100, uint64_t arg_108,
									uint64_t arg_110, uint64_t arg_118, uint64_t arg_120, uint64_t arg_128, uint64_t arg_130, uint64_t arg_138, uint64_t arg_140, uint64_t arg_148, uint64_t arg_150, uint64_t arg_158,
									uint64_t arg_160, uint64_t arg_168, uint64_t arg_170, uint64_t arg_178, uint64_t arg_180, uint64_t arg_188, uint64_t arg_190, uint64_t arg_198, uint64_t arg_1A0, uint64_t arg_1A8);*/
									
//typedef uint64_t (* ResultPortraitsType)(uint64_t arg_rcx, uint64_t arg_rdx, uint32_t is_cac, uint32_t cms_entry);
typedef uint64_t (* ResultPortraits2Type)(Battle_HudCockpit *pthis, int idx, void *r8, void *r9, void *arg5, void *arg6, void *arg7, void *arg8);
typedef int (* Behaviour10FuncType)(void *, uint32_t, void *);
typedef void (* ApplyCacMatsType)(uint64_t *, uint32_t, const char *);
typedef void (* AurBpeType)(void *, uint32_t, uint32_t, uint32_t);


// These constants are the ones in CharaSele.as
// I have kept their names despite in native code, receives are actually sends

static constexpr const int PlayerNumFri = 2;
static constexpr const int SkillMax = 8;
static constexpr const int CharaVarIndexNum = 32;
static constexpr const int CharacterMax = 256; // 1.17: 128->256
static constexpr const int CustomListMax = 512; // 1.17: 256->512
static constexpr const int CharacterTableData = 15;  // 1.22: 14->15
static constexpr const int CharacterTableMax = 512; // 1.10: 350->512
static constexpr const int ReceiveType_FlagUseCancel = 0;
static constexpr const int ReceiveType_PlayerFriNum = 1;
static constexpr const int ReceiveType_PlayerEnmNum = 2;
static constexpr const int ReceiveType_PartyNpcNum = 3;
static constexpr const int ReceiveType_FlagSelectAvatar = 4;
static constexpr const int ReceiveType_FlagLocalBattle = 5;
static constexpr const int ReceiveType_Flag2pController = 6;
static constexpr const int ReceiveType_Str2pController = 7;
static constexpr const int ReceiveType_Time = 8;
static constexpr const int ReceiveType_CharaNameStr = 9;
static constexpr const int ReceiveType_NameOption_GK2 = 10;
static constexpr const int ReceiveType_NameOption_CGK = 11; // 1.22: new
static constexpr const int ReceiveType_NameOption_LvText = 12; // 1.10: new
static constexpr const int ReceiveType_NameOption_LvNum = 13; // 1.10: new
static constexpr const int ReceiveType_VariationNameStr = 14;  
static constexpr const int ReceiveType_TarismanHeaderStr = 15; 
static constexpr const int ReceiveType_TarismanNameStr = 16; 
static constexpr const int ReceiveType_ImageStrStart = 17; 
static constexpr const int ReceiveType_ImageStrEnd = ReceiveType_ImageStrStart + CharacterMax - 1; // 0x110
static constexpr const int ReceiveType_UnlockVarStart = ReceiveType_ImageStrEnd + 1; // 0x111 (pre 1.22: 0x110)
static constexpr const int ReceiveType_UnlockVarEnd = ReceiveType_UnlockVarStart + CharaVarIndexNum * CharacterMax - 1; // 0x2110
static constexpr const int ReceiveType_KeyStrL1 = ReceiveType_UnlockVarEnd + 1; // 0x2111 (pre 1.22: 0x2110)
static constexpr const int ReceiveType_KeyStrR1 = ReceiveType_KeyStrL1 + 1; // 0x2112
static constexpr const int ReceiveType_KeyStrL2 = ReceiveType_KeyStrR1 + 1; // 0x2113
static constexpr const int ReceiveType_KeyStrR2 = ReceiveType_KeyStrL2 + 1; // 0x2114
static constexpr const int ReceiveType_KeyStrRU = ReceiveType_KeyStrR2 + 1; // 0x2115
static constexpr const int ReceiveType_KeyStrRD = ReceiveType_KeyStrRU + 1; // 0x2116
static constexpr const int ReceiveType_KeyStrRL = ReceiveType_KeyStrRD + 1; // 0x2117
static constexpr const int ReceiveType_KeyStrRR = ReceiveType_KeyStrRL + 1; // 0x2118
static constexpr const int ReceiveType_KeyStrSingleLS = ReceiveType_KeyStrRR + 1; // 0x2119
static constexpr const int ReceiveType_KeyStrSingleRS = ReceiveType_KeyStrSingleLS + 1; // 0x211A
static constexpr const int ReceiveType_KeyStrSingleU = ReceiveType_KeyStrSingleRS + 1; // 0x211B
static constexpr const int ReceiveType_KeyStrSingleD = ReceiveType_KeyStrSingleU + 1; // 0x211C
static constexpr const int ReceiveType_KeyStrSingleL = ReceiveType_KeyStrSingleD + 1; // 0x211D
static constexpr const int ReceiveType_KeyStrSingleR = ReceiveType_KeyStrSingleL + 1; // 0x211E
static constexpr const int ReceiveType_SkillNameStrStart = ReceiveType_KeyStrSingleR + 1; // 0x211F
static constexpr const int ReceiveType_SkillNameStrEnd = ReceiveType_SkillNameStrStart + SkillMax - 1; // 0x2126
static constexpr const int ReceiveType_ImageStrNpcStart = ReceiveType_SkillNameStrEnd + 1; // 0x2127
static constexpr const int ReceiveType_ImageStrNpcEnd = ReceiveType_ImageStrNpcStart + PlayerNumFri - 1; // 0x2128
static constexpr const int ReceiveType_CharaSelectedStart = ReceiveType_ImageStrNpcEnd + 1; // 0x2129 (pre 1.22: 0x2128)
static constexpr const int ReceiveType_CharaSelectedEnd = ReceiveType_CharaSelectedStart + CharacterMax - 1; // 0x2228
static constexpr const int ReceiveType_CharaVariationStart = ReceiveType_CharaSelectedEnd + 1; // 0x2229
static constexpr const int ReceiveType_CharaVariationEnd = ReceiveType_CharaVariationStart + CharaVarIndexNum - 1; // 0x2248
static constexpr const int ReceiveType_DLCUnlockFlag = ReceiveType_CharaVariationEnd + 1; // 0x2249
static constexpr const int ReceiveType_DLCUnlockFlag2 = ReceiveType_DLCUnlockFlag + 1; // 0x224A
static constexpr const int ReceiveType_JoyConSingleFlag = ReceiveType_DLCUnlockFlag2 + 1; // 0x224B
static constexpr const int ReceiveType_WaitLoadNum = ReceiveType_JoyConSingleFlag; // 0x224B

static constexpr const int ReceiveType_CostumeNum = ReceiveType_JoyConSingleFlag + 1; // 0x224c
static constexpr const int ReceiveType_CharacterTableStart = ReceiveType_CostumeNum + 1; // 0x224D
static constexpr const int ReceiveType_CostumeID = ReceiveType_CharacterTableStart; // 0x224D
static constexpr const int ReceiveType_CID = ReceiveType_CostumeID + 1; // 0x224E
static constexpr const int ReceiveType_MID = ReceiveType_CID + 1; // 0x224F
static constexpr const int ReceiveType_PID = ReceiveType_MID + 1; // 0x2250
static constexpr const int ReceiveType_UnlockNum = ReceiveType_PID + 1; // 0x2251
static constexpr const int ReceiveType_Gokuaku = ReceiveType_UnlockNum + 1; // 0x2252
static constexpr const int ReceiveType_SelectVoice1 = ReceiveType_Gokuaku + 1; // 0x2253
static constexpr const int ReceiveType_SelectVoice2 = ReceiveType_SelectVoice1 + 1; // 0x2254
static constexpr const int ReceiveType_DlcKey = ReceiveType_SelectVoice2 + 1; // 0x2255
static constexpr const int ReceiveType_DlcKey2 = ReceiveType_DlcKey + 1; //  0x2256
static constexpr const int ReceiveType_CustomCostume = ReceiveType_DlcKey2 + 1; // 0x2257
static constexpr const int ReceiveType_AvatarSlotID = ReceiveType_CustomCostume + 1; // 0x2258
static constexpr const int ReceiveType_AfterTU9Order = ReceiveType_AvatarSlotID + 1; // 0x2259
static constexpr const int ReceiveType_CustomCostumeEx = ReceiveType_AfterTU9Order + 1; // 0x225A
static constexpr const int ReceiveType_ChouGokuaku = ReceiveType_CustomCostumeEx + 1; // 0x225B (New in 1.22)
static constexpr const int ReceiveType_CharacterTableEnd = ReceiveType_CharacterTableStart + CharacterTableMax * CharacterTableData; // 0x404d

static constexpr const int ReceiveType_UseCustomList = ReceiveType_CharacterTableEnd + 1; // 0x404E
static constexpr const int ReceiveType_CustomListNum = ReceiveType_UseCustomList + 1; // 0x404F
static constexpr const int ReceiveType_CustomList_CID_Start = ReceiveType_CustomListNum + 1; // 0x4050
static constexpr const int ReceiveType_CustomList_CID_End = ReceiveType_CustomList_CID_Start + CustomListMax - 1; // 0x424f
static constexpr const int ReceiveType_CustomList_MID_Start = ReceiveType_CustomList_CID_End + 1; // 0x4250
static constexpr const int ReceiveType_CustomList_MID_End = ReceiveType_CustomList_MID_Start + CustomListMax - 1; // 0x444f
static constexpr const int ReceiveType_CustomList_PID_Start = ReceiveType_CustomList_MID_End + 1; // 0x4450
static constexpr const int ReceiveType_CustomList_PID_End = ReceiveType_CustomList_PID_Start + CustomListMax - 1; // 0x464f
static constexpr const int ReceiveType_CustomList_PartnerJudge_Start = ReceiveType_CustomList_PID_End + 1; // 0x4650
static constexpr const int ReceiveType_CustomList_PartnerJudge_End = ReceiveType_CustomList_PartnerJudge_Start + CustomListMax - 1; // 0x484f
static constexpr const int ReceiveType_Num = ReceiveType_CustomList_PartnerJudge_End + 1; // 0x4850

static_assert(ReceiveType_Num == 0x4850, "Error (ReceiveType_Num)");

extern "C" 
{
	
static void *charasele_obj;
	
static int n_CharacterMax;
static int n_ReceiveType_ImageStrEnd; 
static int n_ReceiveType_UnlockVarStart;
static int n_ReceiveType_UnlockVarEnd;
static int n_ReceiveType_KeyStrL1;
static int n_ReceiveType_KeyStrR1;
static int n_ReceiveType_KeyStrL2;
static int n_ReceiveType_KeyStrR2;
static int n_ReceiveType_KeyStrRU;
static int n_ReceiveType_KeyStrRD;
static int n_ReceiveType_KeyStrRL;
static int n_ReceiveType_KeyStrRR;
static int n_ReceiveType_KeyStrSingleLS;
static int n_ReceiveType_KeyStrSingleRS;   
static int n_ReceiveType_KeyStrSingleU;
static int n_ReceiveType_KeyStrSingleD;    
static int n_ReceiveType_KeyStrSingleL;
static int n_ReceiveType_KeyStrSingleR;
static int n_ReceiveType_SkillNameStrStart;
static int n_ReceiveType_SkillNameStrEnd;
static int n_ReceiveType_ImageStrNpcStart;
static int n_ReceiveType_ImageStrNpcEnd;
static int n_ReceiveType_CharaSelectedStart;
static int n_ReceiveType_CharaSelectedEnd;
static int n_ReceiveType_CharaVariationStart;
static int n_ReceiveType_CharaVariationEnd;
static int n_ReceiveType_DLCUnlockFlag;
static int n_ReceiveType_DLCUnlockFlag2;
static int n_ReceiveType_JoyConSingleFlag;
static int n_ReceiveType_WaitLoadNum;

static int n_ReceiveType_CostumeNum; 
static int n_ReceiveType_CharacterTableStart;
static int n_ReceiveType_CostumeID;
static int n_ReceiveType_CID;
static int n_ReceiveType_MID;
static int n_ReceiveType_PID;
static int n_ReceiveType_UnlockNum;
static int n_ReceiveType_Gokuaku;
static int n_ReceiveType_SelectVoice1;
static int n_ReceiveType_SelectVoice2;
static int n_ReceiveType_DlcKey;
static int n_ReceiveType_DlcKey2;
static int n_ReceiveType_CustomCostume;
static int n_ReceiveType_AvatarSlotID;
static int n_ReceiveType_AfterTU9Order;
static int n_ReceiveType_CustomCostumeEx;
static int n_ReceiveType_ChouGokuaku;
static int n_ReceiveType_CharacterTableEnd;

static int n_ReceiveType_UseCustomList;
static int n_ReceiveType_CustomListNum;
static int n_ReceiveType_CustomList_CID_Start;
static int n_ReceiveType_CustomList_CID_End;
static int n_ReceiveType_CustomList_MID_Start;
static int n_ReceiveType_CustomList_MID_End;
static int n_ReceiveType_CustomList_PID_Start;
static int n_ReceiveType_CustomList_PID_End;
static int n_ReceiveType_CustomList_PartnerJudge_Start;
static int n_ReceiveType_CustomList_PartnerJudge_End;
static int n_ReceiveType_Num;

static std::string x2s_raw;

static IggyBaseCtorType IggyBaseCtor;
static ChaselDtorType ChaselDtor;
static SendToAS3Type SendToAS3Int;
static SendToAS3Type SendToAS3Flag;
static SendToAS3Type SendToAS3String;
static Func1Type func1;
static Func2Type func2;
static CheckUnlockType check_unlock;
static SetBodyShapeType SetBodyShape;
static MobSetDefaultBodyType MobSetDefaultBody;
//static ResultPortraitsType ResultPortraits;
static ResultPortraits2Type ResultPortraits2;
static Behaviour10FuncType Behaviour10Func;
static ApplyCacMatsType ApplyCacMats;
static AurBpeType AurBpe;

static std::unordered_map<void *, uint64_t> send_map;

static void *bh10_untransform_ra;

static uint8_t *find_blob_string(uint8_t *blob, size_t size, const char *str)
{
	size_t len = strlen(str);
	size_t max_search = size - len;
	
	for (size_t i = 0; i <= max_search; i++)
	{
		if (memcmp(blob+i, str, strlen(str)) == 0)
		{
			return blob+i;
		}
	}
	
	return nullptr;
}

static uint32_t guess_character_max()
{
	const std::string iggy_path = myself_path + CONTENT_ROOT + "data/ui/iggy/CHARASELE.iggy";
	IggyFile iggy;
	
	if (!iggy.LoadFromFile(iggy_path, false))
	{
		DPRINTF("Auto character max assumes %d because there is no custom iggy file. (assuming too the one in cpk is untouched).\n", CharacterMax);
		return CharacterMax;
	}
	
	uint32_t size;
	uint8_t *abc_blob = iggy.GetAbcBlob(&size);
	
	if (!abc_blob)
	{
		DPRINTF("Auto character max assumes %d because it was not able to extract actionscript from iggy.\n", CharacterMax);
		return CharacterMax;
	}
	
	if (!find_blob_string(abc_blob, size, "DlcKey2"))
	{
		UPRINTF("The version of CHARASELE.iggy at data/ui/iggy is not longer compatible with this game version.\n"
				"Delete your old mods install and update your mods/tools to compatible versions.\n");
				
		exit(-1);
	}
	
	uint8_t *ptr = abc_blob;
	for (uint32_t i = 0; i < 0x40; i++, ptr++)
	{
		uint64_t double_val = *(uint64_t *)ptr;
		
		if (double_val == 0x4018000000000000) // 6.0
		{
			// Read next double
			double ImageStrEnd_double = *(double *)(ptr+8);
			
			if (!std::isfinite(ImageStrEnd_double) || ImageStrEnd_double < 23.0 || ImageStrEnd_double > 100013.0)
			{
				DPRINTF("Auto detecting character max failed. We'll asume the default of %d.\n", CharacterMax);
				return CharacterMax;
			}
			
			uint32_t ret = lrint(ImageStrEnd_double) - 16;
			DPRINTF("Auto character max has been estimated: %d\n", ret);
			return ret;
		}
	}
	
	DPRINTF("Auto detecting character max failed (2). We'll asume the default of %d.\n", CharacterMax);
	return CharacterMax;
}

static void ReadSlotsFile()
{
	static time_t mtime = 0;
	const std::string path = myself_path + CONTENT_ROOT + SLOTS_FILE;
	time_t current_mtime;
	
	if (Utils::GetFileDate(path, &current_mtime) && current_mtime != mtime)
	{
		Utils::ReadTextFile(path, x2s_raw, false);
		mtime = current_mtime;
	}
}

PUBLIC void CharaSetup(IggyBaseCtorType orig)
{
	IggyBaseCtor = orig;
	
	ini.GetIntegerValue("NewChara", "CharacterMax", &n_CharacterMax, CharacterMax);
	
	if (n_CharacterMax == 0)
	{
		n_CharacterMax = guess_character_max();
	}		
	
	ReadSlotsFile();

	n_ReceiveType_ImageStrEnd = ReceiveType_ImageStrStart + n_CharacterMax - 1; 
	n_ReceiveType_UnlockVarStart = n_ReceiveType_ImageStrEnd + 1;
	n_ReceiveType_UnlockVarEnd = n_ReceiveType_UnlockVarStart + CharaVarIndexNum * n_CharacterMax - 1;
	n_ReceiveType_KeyStrL1 = n_ReceiveType_UnlockVarEnd + 1;
	n_ReceiveType_KeyStrR1 = n_ReceiveType_KeyStrL1 + 1;
	n_ReceiveType_KeyStrL2 = n_ReceiveType_KeyStrR1 + 1;
	n_ReceiveType_KeyStrR2 = n_ReceiveType_KeyStrL2 + 1;
	n_ReceiveType_KeyStrRU = n_ReceiveType_KeyStrR2 + 1;
	n_ReceiveType_KeyStrRD = n_ReceiveType_KeyStrRU + 1;
	n_ReceiveType_KeyStrRL = n_ReceiveType_KeyStrRD + 1;
	n_ReceiveType_KeyStrRR = n_ReceiveType_KeyStrRL + 1;
	n_ReceiveType_KeyStrSingleLS = n_ReceiveType_KeyStrRR + 1;
	n_ReceiveType_KeyStrSingleRS = n_ReceiveType_KeyStrSingleLS + 1;
	n_ReceiveType_KeyStrSingleU = n_ReceiveType_KeyStrSingleRS + 1;
	n_ReceiveType_KeyStrSingleD = n_ReceiveType_KeyStrSingleU + 1;    
	n_ReceiveType_KeyStrSingleL = n_ReceiveType_KeyStrSingleD + 1;
	n_ReceiveType_KeyStrSingleR = n_ReceiveType_KeyStrSingleL + 1;
	n_ReceiveType_SkillNameStrStart = n_ReceiveType_KeyStrSingleR + 1;
	n_ReceiveType_SkillNameStrEnd = n_ReceiveType_SkillNameStrStart + SkillMax - 1;
	n_ReceiveType_ImageStrNpcStart = n_ReceiveType_SkillNameStrEnd + 1;
	n_ReceiveType_ImageStrNpcEnd = n_ReceiveType_ImageStrNpcStart + PlayerNumFri - 1;
	n_ReceiveType_CharaSelectedStart = n_ReceiveType_ImageStrNpcEnd + 1;
	n_ReceiveType_CharaSelectedEnd = n_ReceiveType_CharaSelectedStart + n_CharacterMax - 1;
	n_ReceiveType_CharaVariationStart = n_ReceiveType_CharaSelectedEnd + 1;
	n_ReceiveType_CharaVariationEnd = n_ReceiveType_CharaVariationStart + CharaVarIndexNum - 1;
	n_ReceiveType_DLCUnlockFlag = n_ReceiveType_CharaVariationEnd + 1;	
	n_ReceiveType_DLCUnlockFlag2 = n_ReceiveType_DLCUnlockFlag + 1;
	n_ReceiveType_JoyConSingleFlag = n_ReceiveType_DLCUnlockFlag2 + 1;
	n_ReceiveType_WaitLoadNum = n_ReceiveType_JoyConSingleFlag; 	
	
	n_ReceiveType_CostumeNum = n_ReceiveType_JoyConSingleFlag + 1; 
	n_ReceiveType_CharacterTableStart = n_ReceiveType_CostumeNum + 1; 
	n_ReceiveType_CostumeID = n_ReceiveType_CharacterTableStart; 
	n_ReceiveType_CID = n_ReceiveType_CostumeID + 1; 
	n_ReceiveType_MID = n_ReceiveType_CID + 1; 
	n_ReceiveType_PID = n_ReceiveType_MID + 1;  
	n_ReceiveType_UnlockNum = n_ReceiveType_PID + 1; 
	n_ReceiveType_Gokuaku = n_ReceiveType_UnlockNum + 1; 
	n_ReceiveType_SelectVoice1 = n_ReceiveType_Gokuaku + 1;
	n_ReceiveType_SelectVoice2 = n_ReceiveType_SelectVoice1 + 1; 
	n_ReceiveType_DlcKey = n_ReceiveType_SelectVoice2 + 1; 
	n_ReceiveType_DlcKey2 = n_ReceiveType_DlcKey + 1; 
	n_ReceiveType_CustomCostume = n_ReceiveType_DlcKey2 + 1; 
	n_ReceiveType_AvatarSlotID = n_ReceiveType_CustomCostume + 1;
	n_ReceiveType_AfterTU9Order = n_ReceiveType_AvatarSlotID + 1;
	n_ReceiveType_CustomCostumeEx = n_ReceiveType_AfterTU9Order + 1;
	n_ReceiveType_ChouGokuaku = n_ReceiveType_CustomCostumeEx + 1;
	n_ReceiveType_CharacterTableEnd = n_ReceiveType_CharacterTableStart + CharacterTableMax * CharacterTableData; 
	
	n_ReceiveType_UseCustomList = n_ReceiveType_CharacterTableEnd + 1;	
	n_ReceiveType_CustomListNum = n_ReceiveType_UseCustomList + 1; 
	n_ReceiveType_CustomList_CID_Start = n_ReceiveType_CustomListNum + 1; 
	n_ReceiveType_CustomList_CID_End = n_ReceiveType_CustomList_CID_Start + CustomListMax - 1; 
	n_ReceiveType_CustomList_MID_Start = n_ReceiveType_CustomList_CID_End + 1; 
	n_ReceiveType_CustomList_MID_End = n_ReceiveType_CustomList_MID_Start + CustomListMax - 1; 
	n_ReceiveType_CustomList_PID_Start = n_ReceiveType_CustomList_MID_End + 1; 
	n_ReceiveType_CustomList_PID_End = n_ReceiveType_CustomList_PID_Start + CustomListMax - 1; 
	n_ReceiveType_CustomList_PartnerJudge_Start = n_ReceiveType_CustomList_PID_End + 1; 
	n_ReceiveType_CustomList_PartnerJudge_End = n_ReceiveType_CustomList_PartnerJudge_Start + CustomListMax - 1; 
	n_ReceiveType_Num = n_ReceiveType_CustomList_PartnerJudge_End + 1; 
	
	//DPRINTF("%x - %x, %x - %x\n", n_ReceiveType_UnlockVarStart, n_ReceiveType_UnlockVarEnd, n_ReceiveType_CharaVariationStart, n_ReceiveType_CharaVariationEnd);	
}

PUBLIC void AS3SendIntSetup(SendToAS3Type orig)
{
	SendToAS3Int = orig;
}

PUBLIC void AS3SendFlagSetup(SendToAS3Type orig)
{
	SendToAS3Flag = orig;
}

PUBLIC void AS3SendStringSetup(SendToAS3Type orig)
{
	SendToAS3String = orig;
}

static void *ChaselDtorHooked(void *pthis, uint32_t dtor_mode)
{
	void *ret = ChaselDtor(pthis, dtor_mode);
	if (pthis == charasele_obj)
	{
		charasele_obj = nullptr;
		//send_map.clear();
	}
		
	return ret;
}

PUBLIC void *ChaselMainPatch(void *pthis, int32_t unk, int32_t num)
{
	charasele_obj = pthis;
	
	if (num != ReceiveType_Num)
	{
		UPRINTF("%s: Unexpected value of num (%d != %d).\n", FUNCNAME, num, ReceiveType_Num);
		exit(-1);
	}
	
	DPRINTF("ChaselMainPatch: %d -> %d\n", num, n_ReceiveType_Num);
	
	num = n_ReceiveType_Num;	
	return IggyBaseCtor(pthis, unk, num);
}

static int32_t ResolveCode(void *pthis, void *ra, int32_t code)
{
	static bool vdtor_patched = false;
	
	if (!vdtor_patched)
	{		
		// Patch virtual destructor
		uint64_t *vtable = *(uint64_t **)pthis;
		ChaselDtor = (ChaselDtorType)vtable[0];
		PatchUtils::Write64(vtable, (uint64_t)ChaselDtorHooked);
		
		vdtor_patched = true;
	}

	auto it = send_map.find(ra);
	if (it == send_map.end())
	{
		if (code < ReceiveType_ImageStrStart)
			return code;
		
		int32_t maped = -1;
		bool special = false;
		
		if (code == ReceiveType_ImageStrStart)
			maped = ReceiveType_ImageStrStart; // Same
		else if (code == ReceiveType_UnlockVarStart)
			maped = n_ReceiveType_UnlockVarStart;
		else if (code == ReceiveType_KeyStrL1)
			maped = n_ReceiveType_KeyStrL1;
		else if (code == ReceiveType_SkillNameStrStart)
			maped = n_ReceiveType_SkillNameStrStart;
		else if (code == ReceiveType_ImageStrNpcStart)
			maped = n_ReceiveType_ImageStrNpcStart;
		else if (code == ReceiveType_CharaSelectedStart)
			maped = n_ReceiveType_CharaSelectedStart;
		else if (code == ReceiveType_CharaVariationStart)
			maped = n_ReceiveType_CharaVariationStart;
		else if (code == ReceiveType_DLCUnlockFlag)
			maped = n_ReceiveType_DLCUnlockFlag;
		else if (code == ReceiveType_DLCUnlockFlag2)
			maped = n_ReceiveType_DLCUnlockFlag2;
		else if (code == ReceiveType_JoyConSingleFlag)
			maped = n_ReceiveType_JoyConSingleFlag;
		else if (code == ReceiveType_CostumeNum)
			maped = n_ReceiveType_CostumeNum;
		else if (code == ReceiveType_CostumeID)
		{
			maped = n_ReceiveType_CostumeID;
			special = true;
		}
		else if (code == ReceiveType_CID)
		{
			maped = n_ReceiveType_CID;
			special = true;
		}
		else if (code == ReceiveType_MID)
		{
			maped = n_ReceiveType_MID;
			special = true;
		}
		else if (code == ReceiveType_PID)
		{
			maped = n_ReceiveType_PID;
			special = true;
		}
		else if (code == ReceiveType_UnlockNum)
		{
			maped = n_ReceiveType_UnlockNum;
			special = true;
		}
		else if (code == ReceiveType_Gokuaku)
		{
			maped = n_ReceiveType_Gokuaku;
			special = true;
		}
		else if (code == ReceiveType_SelectVoice1)
		{
			maped = n_ReceiveType_SelectVoice1;
			special = true;
		}
		else if (code == ReceiveType_SelectVoice2)
		{
			maped = n_ReceiveType_SelectVoice2;
			special = true;
		}
		else if (code == ReceiveType_DlcKey)
		{
			maped = n_ReceiveType_DlcKey;
			special = true;
		}
		else if (code == ReceiveType_DlcKey2)
		{
			maped = n_ReceiveType_DlcKey2;
			special = true;
		}
		else if (code == ReceiveType_CustomCostume)
		{
			maped = n_ReceiveType_CustomCostume;
			special = true;
		}
		else if (code == ReceiveType_AvatarSlotID)
		{
			maped = n_ReceiveType_AvatarSlotID;
			special = true;
		}
		else if (code == ReceiveType_AfterTU9Order)
		{
			maped = n_ReceiveType_AfterTU9Order;
			special = true;
		}
		else if (code == ReceiveType_CustomCostumeEx)
		{
			maped = n_ReceiveType_CustomCostumeEx;
			special = true;
		}
		else if (code == ReceiveType_ChouGokuaku)
		{
			maped = n_ReceiveType_ChouGokuaku;
			special = true;
		}
		else if (code == ReceiveType_UseCustomList)
			maped = n_ReceiveType_UseCustomList;
		else if (code == ReceiveType_CustomList_CID_Start)
			maped = n_ReceiveType_CustomList_CID_Start;
		else if (code == ReceiveType_CustomList_MID_Start)
			maped = n_ReceiveType_CustomList_MID_Start;
		else if (code == ReceiveType_CustomList_PID_Start)
			maped = n_ReceiveType_CustomList_PID_Start;
		else if (code == ReceiveType_CustomList_PartnerJudge_Start)
			maped = n_ReceiveType_CustomList_PartnerJudge_Start;
		
		if (maped == -1)
		{
			UPRINTF("%s: Cannot map value 0x%x. (ra=%I64x).\n", FUNCNAME, code, PatchUtils::RelAddress(ra));
			exit(-1);
		}
		
		uint64_t value = ((uint64_t)code << 32) | maped;
		if (special) value |= 0x8000000000000000ULL;
		send_map[ra] = value;
		//DPRINTF("Ret 1 0x%x -> 0x%x\n", code, maped);
		return maped;
	}
	else
	{
		uint32_t base = (it->second >> 32);
		bool special = ((base&0x80000000) != 0);
		base = base&0x7FFFFFFF;
		uint32_t maped = (it->second & 0xFFFFFFFF);
		
		if (code < (int32_t)base)
		{
			UPRINTF("%s: Unexpected situation (code(0x%x) < base(0x%x) (ra=%I64x)).\n", FUNCNAME, code, base, PatchUtils::RelAddress(ra));
			exit(-1);
		}
		
		int32_t ret;

		if (!special)
		{
			ret = (int32_t) ((code - base) + maped);
		}
		else
		{
			if (((code - base) % CharacterTableData) != 0)
			{
				UPRINTF("%s: unexpected situation, not multiple of 0x%x. Code minus base=0x%x. Ra=%I64x.\n", FUNCNAME, CharacterTableData, code - base, PatchUtils::RelAddress(ra));
				exit(-1);
			}
			
			uint32_t n = (code - base) / CharacterTableData;
			ret = maped + n*CharacterTableData;			
		}
		
		//DPRINTF("Ret 2 0x%x -> 0x%x (base=0x%x)\n", code, ret, base);
		return ret;
	}
	
	return code;
}

PUBLIC void AS3SendInt_Hook(void *pthis, int32_t code, uint64_t data)
{
	if (charasele_obj != nullptr && charasele_obj == pthis)
	{
		//DPRINTF("SendInt 0x%x\n", code);
		code = ResolveCode(pthis, BRA(0), code);
	}	
	
	SendToAS3Int(pthis, code, data);
}

PUBLIC void AS3SendFlag_Hook(void *pthis, int32_t code, uint64_t data)
{
	if (charasele_obj != nullptr && charasele_obj == pthis)
	{
		//DPRINTF("SendFlag 0x%x\n", code);		
		code = ResolveCode(pthis, BRA(0), code);
	}	
	
	SendToAS3Flag(pthis, code, data);
}

PUBLIC void AS3SendString_Hook(void *pthis, int32_t code, uint64_t data)
{
	if (charasele_obj != nullptr && charasele_obj == pthis)
	{
		//DPRINTF("SendString 0x%x\n", code);		
		code = ResolveCode(pthis, BRA(0), code);
	}
	
	SendToAS3String(pthis, code, data);
}

/* Exceptional cases handling */
PUBLIC void PatchReceiveTypeUnlockVar(void *pthis, int32_t code, uint64_t data)
{
	if (code < ReceiveType_UnlockVarStart || code >= (ReceiveType_UnlockVarStart + CharaVarIndexNum * n_CharacterMax))
	{
		DPRINTF("ERROR: unexpected code at PatchReceiveTypeUnlockVar. Code=0x%x\n", code);
	}
	else
	{
		code = (code - ReceiveType_UnlockVarStart) + n_ReceiveType_UnlockVarStart;
	}
	
	SendToAS3Flag(pthis, code, data);
}

PUBLIC void PatchReceiveTypeCharaSelected(void *pthis, int32_t code, uint64_t data)
{
	if (code < ReceiveType_CharaSelectedStart || code >= (ReceiveType_CharaSelectedStart+n_CharacterMax))
	{
		DPRINTF("ERROR: unexpected code at PatchReceiveTypeCharaSelected. Code=0x%x\n", code);
	}
	else
	{
		code = (code - ReceiveType_CharaSelectedStart) + n_ReceiveType_CharaSelectedStart;
	}
	
	SendToAS3Flag(pthis, code, data);
}
/* End of exceptional cases handling */

PUBLIC void IncreaseChaselMemory(uint32_t *psize)
{
	uint32_t old = *psize;
	PatchUtils::Write32(psize, Utils::Align2(0x2910+(n_CharacterMax*32), 0x100));
	
	DPRINTF("Chasel object memory succesfully changed from 0x%x to 0x%x\n", old, *psize);
}

PUBLIC void SetupIncreaseChaselSlotsArray(Func1Type orig)
{
	func1 = orig;
}

/*void TestBreakpoint(void *pc, void *addr)
{
	DPRINTF("Accesed at address %p (rel %Ix)\n", pc, RelAddress(pc));
	DPRINTF("Address that was accesed: %p (rel %Ix)\n", addr, RelAddress(addr));
	
	exit(-1);
}*/

PUBLIC void IncreaseChaselSlotsArray(void *arg1, uint32_t arg2, uint32_t size, void *arg4, void *arg5)
{
	/*uint8_t *arg1_8 = (uint8_t *)arg1;
	arg1_8 = arg1_8 - 0x18B0;
	if (!PatchUtils::SetMemoryBreakpoint(arg1_8+0x5E0, 0x1000, TestBreakpoint))
	{
		DPRINTF("Failed to set memory breakpoint.\n");
		exit(-1);
	}*/
	
	size = n_CharacterMax;	
	func1(arg1, arg2, size, arg4, arg5);
}

PUBLIC void SetupIncreaseChaselSlotsArray2(Func2Type orig)
{
	func2 = orig;
}

PUBLIC void IncreaseChaselSlotsArray2(void *arg1, uint32_t arg2, uint32_t size, void *arg4)
{
	size = n_CharacterMax;
	func2(arg1, arg2, size, arg4);	
}

// This irrgular patch is for 1.10v2 due to the 0x5E0 = 0x2F << 5 optimization. It will likely be gone in next version if the address 0x5E0 changes.
// When it happens, delete this, and the asm code.
extern void ModifyArrayOffset4Asm();
PUBLIC void ModifyArrayOffset4(uint8_t *buf)
{
	PatchUtils::Write16((uint16_t *)buf, 0xB848); // mov rax, XXXXXXXXXXXXXX
	
	uintptr_t asm_addr = (uintptr_t) ModifyArrayOffset4Asm;
	PatchUtils::Write64((uint64_t *)(buf+2), asm_addr);
	PatchUtils::Write16((uint16_t *)(buf+10), 0xE0FF); // jmp rax
	PatchUtils::Write32(buf+12, 0x90909090); // nops  (not really necessary, code wont reach here, but to not destroy asm view in a debugger)
	
	uint64_t *ret_addr = (uint64_t *)(asm_addr+0x1B);
	if (*ret_addr != 0x123456789ABCDEF)
	{
		UPRINTF("Internal error in ModifyArrayOffset4\n");
		exit(-1);
	}
	
	PatchUtils::Write64(ret_addr, (uint64_t)(buf+0x10)); // buf+0x10 -> address to return to
}

static inline bool is_original_playable_char(int32_t chara)
{
	// Mira final form (0.41 fix)
	if (chara == 0x7D)
		return false;
	
	// between goku and  pan
	if (chara >= 0 && chara <= 0x2A)
		return true;
	
	// between san-shinron and satan
	if (chara >= 0x30 && chara <= 0x34)
		return true;
	
	// between buu small and raspberry
	if (chara >= 0x3A && chara <= 0x3C)
		return true;
	
	// between mira and towa
	if (chara >= 0x3E && chara <= 0x3F)
		return true;
	
	// whis or jaco or cell first form
	if (chara == 0x41 || chara == 0x43 || chara == 0x4B)
		return true;
	
	// between Goku SSGSS and Janemba
	if (chara >= 0x50 && chara <= 0x58)
		return true;
	
	// Races
	if (chara >= 0x64 && chara <= 0x6C)
		return true;
	
	// Gogeta - Hit
	if (chara >= 0x78 && chara <= 0x7E)
		return true;
	
	// Janemba or Goku black or future trunks
	if (chara >= 0x82 && chara <= 0x84)
		return true;
	
	return false;
}

PUBLIC void SetupCheckUnlockFunction(CheckUnlockType orig)
{
	check_unlock = orig;
}

PUBLIC int UnlockCharaMods(void *pthis, int32_t chara, int32_t var)
{
	if (is_original_playable_char(chara))
		return check_unlock(pthis, chara, var);
	
	//DPRINTF("Not original, returning 1 for 0x%x\n", chara);
	
	return 1;
}

// This function is an imitation of the DIMPS one, but without the 16 bits calculation overflow.
// 1.08: this is not longer needed, commenting it
/*
PUBLIC void PscInitReplacement(uint8_t *psc_buf)
{
	if (*psc_buf == 0x21) // Already processed
		return;
		
	if (memcmp(psc_buf+1, "PSC", 3) != 0)
		return;
	
	PSCHeader *hdr = (PSCHeader *)psc_buf;
	
	if (hdr->endianess_check != 0xFFFE)
		return;
	
	PSCEntry *file_entries = (PSCEntry *)(psc_buf+hdr->header_size);
    PSCSpecEntry *file_specs = (PSCSpecEntry *)(file_entries+hdr->num_entries);
	
	for (uint32_t i = 0; i < hdr->num_entries; i++)
	{
		file_entries[i].unk_08 = Utils::DifPointer(file_specs, &file_entries[i]);
		file_specs += file_entries[i].num_specs;
	}
	
	*psc_buf = 0x21; // Set as processed
}*/

const std::string &GetSlotsData()
{
	ReadSlotsFile();
	return x2s_raw;
}

// XV2Patcher extensions

static std::vector<std::string> ozarus = { "OSV", "OSB", "OBB", "OSG", "OSN" }; // The original list, as it is in game (including non-existing OSG)
static std::vector<std::string> cell_maxes = { "SIM", "SIH" }; // The original list as in the game
static uint8_t body_shapes_lookup[LOOKUP_SIZE];
uint8_t auto_btl_portrait_lookup[LOOKUP_SIZE]; // lookup table, we need O(1) access. 

uint16_t cus_aura_lookup[LOOKUP_SIZE]; // This one must be accesible by chara_patch_asm.S
uint8_t cus_aura_bh11_lookup[LOOKUP_SIZE]; // This one must be accesible by chara_patch_asm.S
static uint32_t cus_aura_int2_lookup[LOOKUP_SIZE];
static uint8_t cus_aura_bh10_lookup[LOOKUP_SIZE];
static uint32_t cus_aura_int3_lookup[LOOKUP_SIZE];
uint8_t force_teleport[LOOKUP_SIZE]; // This one must be accesible by chara_patch_asm.S
static uint8_t cus_aura_bh13_lookup[LOOKUP_SIZE];
uint8_t cus_aura_bh66_lookup[LOOKUP_SIZE]; // This one must be accesible by chara_patch_asm.S
static uint32_t bcs_hair_colors[LOOKUP_SIZE];
static uint32_t bcs_eyes_colors[LOOKUP_SIZE];
static std::unordered_map<uint32_t, std::string> bcs_additional_colors;
static uint8_t remove_hair_accessories_lookup[LOOKUP_SIZE];
uint8_t any_dual_skill_lookup[LOOKUP_SIZE]; // This one must be accessible by chara_patch_asm.S
int32_t aur_bpe_map[LOOKUP_SIZE]; // This one must be accessible by chara_patch_asm.S
static bool aur_bpe_flag1[LOOKUP_SIZE];
static bool aur_bpe_flag2[LOOKUP_SIZE];
uint8_t cus_aura_bh64_lookup[LOOKUP_SIZE]; // This one must be accesible by chara_patch_asm.S
static bool cus_aura_gfs_bh[LOOKUP_SIZE];

// Aliases
static std::unordered_map<std::string, std::string> ttc_files_map;

// Pseudo-cacs
std::unordered_map<uint32_t, BcsColorsMap> pcac_colors;

extern void GetRealAura();
extern void GetCABehaviour11();
extern void ForceTeleport();
extern void GetCABehaviour64();

PUBLIC void PreBakeSetup(size_t)
{
	// Set up lookup tables first
	memset(body_shapes_lookup, 0xFF, sizeof(body_shapes_lookup));	
	memset(auto_btl_portrait_lookup, 0, sizeof(auto_btl_portrait_lookup));	
	memset(cus_aura_lookup, 0xFF, sizeof(cus_aura_lookup));
	memset(cus_aura_bh11_lookup, 0xFF, sizeof(cus_aura_bh11_lookup));
	memset(cus_aura_int2_lookup, 0, sizeof(cus_aura_int2_lookup));
	memset(cus_aura_bh10_lookup, 0xFF, sizeof(cus_aura_bh10_lookup));
	memset(cus_aura_int3_lookup, 0, sizeof(cus_aura_int3_lookup));
	memset(force_teleport, 0, sizeof(force_teleport));
	memset(cus_aura_bh13_lookup, 0xFF, sizeof(cus_aura_bh13_lookup));
	memset(cus_aura_bh66_lookup, 0xFF, sizeof(cus_aura_bh66_lookup));
	memset(bcs_hair_colors, 0xFF, sizeof(bcs_hair_colors));
	memset(bcs_eyes_colors, 0xFF, sizeof(bcs_eyes_colors));
	memset(remove_hair_accessories_lookup, 0xFF, sizeof(remove_hair_accessories_lookup));
	memset(any_dual_skill_lookup, 0, sizeof(any_dual_skill_lookup));
	memset(aur_bpe_map, 0xFF, sizeof(aur_bpe_map));
	memset(aur_bpe_flag1, 0, sizeof(aur_bpe_flag1));
	memset(aur_bpe_flag2, 0, sizeof(aur_bpe_flag1));
	memset(cus_aura_bh64_lookup, 0xFF, sizeof(cus_aura_bh64_lookup));
	memset(cus_aura_gfs_bh, 0, sizeof(cus_aura_gfs_bh));
		
	// Original aura mapping
	cus_aura_lookup[0] = 5;
	cus_aura_lookup[1] = 6;
	cus_aura_lookup[2] = 7;
	cus_aura_lookup[3] = cus_aura_lookup[10] = 0xF;
	cus_aura_lookup[4] = cus_aura_lookup[5] = cus_aura_lookup[6] = 0xD;
	cus_aura_lookup[7] = 0xE;
	cus_aura_lookup[8] = 0x12;
	cus_aura_lookup[9] = 0x13;
	cus_aura_lookup[11] = 0x18;
	cus_aura_lookup[12] = 0x19;
	cus_aura_lookup[13] = 0x14;
	cus_aura_lookup[15] = cus_aura_lookup[17] = cus_aura_lookup[18] = 0x16;
	cus_aura_lookup[19] = 0x1E;
	cus_aura_lookup[16] = 0x17;
	cus_aura_lookup[20] = cus_aura_lookup[21] = 0x21;
	cus_aura_lookup[24] = 0x15;
	cus_aura_lookup[25] = 0x27;
	cus_aura_lookup[26] = 0x30;
	cus_aura_lookup[27] = 0x34;
	cus_aura_lookup[28] = 0x35;
	cus_aura_lookup[29] = 0x37;
	cus_aura_lookup[30] = 0x24;
	cus_aura_lookup[31] = 0x38;
	cus_aura_lookup[32] = 0x3A;
	cus_aura_lookup[33] = 0x3B;
	// Original behaviour_11 values: nothing (the 0xFF init will ensure that)
	// Original int 2 values (only non zero)
	cus_aura_int2_lookup[1] = cus_aura_int2_lookup[5] = cus_aura_int2_lookup[21] = cus_aura_int2_lookup[23] = 1; 
	cus_aura_int2_lookup[2] = cus_aura_int2_lookup[3] = cus_aura_int2_lookup[6] = 2;
	// Original behaviour_10 values: nothing (the 0xFF init will ensure that)
	// Original int 3 values (only non zero)
	cus_aura_int3_lookup[0] = 1;
	for (int i = 11; i <= 19; i++)
		cus_aura_int3_lookup[i] = 1;
	
	cus_aura_int3_lookup[1] = cus_aura_int3_lookup[7] = cus_aura_int3_lookup[8] = cus_aura_int3_lookup[9] = 2;
	cus_aura_int3_lookup[2] = cus_aura_int3_lookup[3] = 3;
	// Default force_teleport are all 0
	// Original behaviour_13 values: nothing (the 0xFF init will ensure that)
	// Original cus_aura_bh66_lookup values: nothing (the 0xFF init will ensure that)
	// Original bcs hair colors: only ssj blue, evolution and god, the 0xFF memset ensure rest get nothing.
	bcs_hair_colors[24] = 60; // SSJ Blue original
	bcs_hair_colors[25] = 67; // SSJ Blue Evolution original
	bcs_hair_colors[26] = 10; // SS God
	// Original bcs eyes colors: only ssj blue and evolution, the 0xFF memset ensure rest get nothing.
	bcs_eyes_colors[24] = 60; // SSJ Blue original
	bcs_eyes_colors[25] = 67; // SSJ Blue Evolution original
	bcs_eyes_colors[26] = 11; // SS God
	// Original remove_hair_accessories_lookup: nothing (the 0xFF init will ensure that)
	// Original bpe thing
	aur_bpe_map[26] = aur_bpe_map[27] = aur_bpe_map[29] = aur_bpe_map[31] = aur_bpe_map[50] = 83;
	aur_bpe_map[36] = 257;
	aur_bpe_map[39] = 260;
	aur_bpe_map[45] = 265;
	aur_bpe_map[46] = 273;
	aur_bpe_map[51] = 280;
	aur_bpe_map[52] = 281;
	aur_bpe_map[53] = 302;
	aur_bpe_map[57] = aur_bpe_map[58] = aur_bpe_map[59] = 320;
	aur_bpe_flag1[39] = aur_bpe_flag1[52] = aur_bpe_flag1[53] = true;
	aur_bpe_flag2[36] = aur_bpe_flag2[39] = aur_bpe_flag2[52] = aur_bpe_flag2[53] = true;
	// Original Golden Freezer Skin behaviour
	cus_aura_gfs_bh[13] = true;
	
	Xv2PreBakedFile pbk;
	const std::string pbk_path = myself_path + CONTENT_ROOT + "data/pre-baked.xml";
	
	if (!pbk.CompileFromFile(pbk_path, false))
	{
		if (Utils::FileExists(pbk_path))
		{
			UPRINTF("Compilation of \"%s\" failed.\n", pbk_path.c_str());
			exit(-1);
		}
		
		return;
	}
	
	std::vector<std::string> &additional_ozarus = pbk.GetOzarus();	
	for (const std::string &ozaru : additional_ozarus)
		ozarus.push_back(ozaru);
	
	std::vector<std::string> &additional_cm = pbk.GetCellMaxes();	
	for (const std::string &cm : additional_cm)
		cell_maxes.push_back(cm);
	
	const std::vector<BodyShape> &body_shapes = pbk.GetBodyShapes();	
	for (const BodyShape &shape : body_shapes)
	{
		if (shape.cms_entry < LOOKUP_SIZE)
			body_shapes_lookup[shape.cms_entry] = (uint8_t)shape.body_shape;
	}
	
	
	const std::vector<uint32_t> &auto_btl_portraits_list = pbk.GetAutoBtlPortraitList();		
	for (uint32_t cms_entry : auto_btl_portraits_list)
	{
		if (cms_entry < LOOKUP_SIZE)
			auto_btl_portrait_lookup[cms_entry] = 1;
	}
	
	const std::vector<CusAuraData> &cus_aura_datas = pbk.GetCusAuraDatas();
	for (const CusAuraData &data : cus_aura_datas)
	{
		if (data.cus_aura_id < LOOKUP_SIZE)
		{
			cus_aura_lookup[data.cus_aura_id] = data.aur_aura_id;
			cus_aura_bh11_lookup[data.cus_aura_id] = data.behaviour_11;
			cus_aura_int2_lookup[data.cus_aura_id] = data.integer_2;
			cus_aura_bh10_lookup[data.cus_aura_id] = data.behaviour_10;
			cus_aura_int3_lookup[data.cus_aura_id] = data.integer_3;
			force_teleport[data.cus_aura_id] = data.force_teleport;
			cus_aura_bh13_lookup[data.cus_aura_id] = data.behaviour_13;
			cus_aura_bh66_lookup[data.cus_aura_id] = data.behaviour_66;
			remove_hair_accessories_lookup[data.cus_aura_id] = data.remove_hair_accessories;
			bcs_hair_colors[data.cus_aura_id] = data.bcs_hair_color;
			bcs_eyes_colors[data.cus_aura_id] = data.bcs_eyes_color;	
			cus_aura_bh64_lookup[data.cus_aura_id] = data.behaviour_64;
			cus_aura_gfs_bh[data.cus_aura_id] = data.golden_freezer_skin_bh;

			if (data.bcs_additional_colors.length() > 0)
			{
				bcs_additional_colors[data.cus_aura_id] = data.bcs_additional_colors;
			}
		}
	}
	
	const std::vector<PreBakedAlias> &aliases = pbk.GetAliases();	
	for (const PreBakedAlias &alias : aliases)
	{
		ttc_files_map[alias.cms_name] = alias.ttc_files;
	}
	
	const std::vector<uint32_t> &any_dual_skills = pbk.GetAnyDualSkillList();
	for (uint32_t cms_entry : any_dual_skills)
	{
		if (cms_entry < LOOKUP_SIZE)
			any_dual_skill_lookup[cms_entry] = 1;
	}
	
	pcac_colors = pbk.GetColorsMap();
	
	for (auto &it : pbk.GetAuraExtraMap())
	{
		uint32_t aur_id = (uint32_t)it.first;		
		
		if (aur_id < LOOKUP_SIZE)
		{
			aur_bpe_map[aur_id] = it.second.bpe_id;
			aur_bpe_flag1[aur_id] = it.second.flag1;
			aur_bpe_flag2[aur_id] = it.second.flag2;
		}
	}
}

// Old implementation that simply skipped call, (not longer works in 1.10)
/*PUBLIC void ApplyCacMatsPatched(uint64_t *object, uint32_t arg2, const char *mat)
{
	uint32_t *object2 = (uint32_t *)object[0x30/8];
	// Don' the call in mods chars.
	bool do_call = true;
	
	//DPRINTF("Apply Cacs Mats. Cms 0x%x. arg2=0x%x, arg3=%s, called from %p\n", object2[0x90/4], arg2, mat, BRA(0));
	
	if (object2)
	{
		uint32_t cms_entry = object2[0x90/4];	
		
		if (cms_entry >= 0x9C)
		{
			do_call = false;
		}
	}
	
	if (do_call)
		ApplyCacMats(object, arg2, mat);
}*/

static const std::unordered_map<std::string, uint16_t> bcs_part_to_index =
{
	{ "SKIN_", 0 },
	{ "SKIN_A_", 0}, // Same as above
	{ "SKIN_B_", 1 },
	{ "SKIN_C_", 2 },
	{ "SKIN_D_", 3 },
	{ "HAIR_", 4 },
	{ "eye_", 5 },
	{ "CC00_BUST_", 6 },
	{ "CC01_BUST_", 7 },
	{ "CC02_BUST_", 8 },
	{ "CC03_BUST_", 9 },
	{ "CC00_PANTS_", 10 },
	{ "CC01_PANTS_", 11 },
	{ "CC02_PANTS_", 12 },
	{ "CC03_PANTS_", 13 },
	{ "CC00_RIST_", 14 },
	{ "CC01_RIST_", 15 },
	{ "CC02_RIST_", 16 },
	{ "CC03_RIST_", 17 },
	{ "CC00_BOOTS_", 18 },
	{ "CC01_BOOTS_", 19 },
	{ "CC02_BOOTS_", 20 },
	{ "CC03_BOOTS_", 21 },
	{ "PAINT_A_", 22 },
	{ "PAINT_B_", 23 },
	{ "PAINT_C_", 24 }
};

PUBLIC void ApplyCacMatsPatched(uint64_t *object, uint32_t arg2, const char *mat)
{
	uint32_t *object2 = (uint32_t *)object[0x30/8];
	// Don' the call in mods chars.
	bool do_call = true;
	
	//DPRINTF("Apply Cacs Mats. Object = %p, object2 = %p, Cms 0x%x, costume 0x%x. arg2=0x%x, arg3=%s, called from %p\n", object, object2, object2[0xD0/4], object2[0xD4/4], arg2, mat, BRA(0));
	
	if (object2)
	{
		uint32_t cms_entry = object2[0xD0/4];	
		uint32_t costume = object2[0xD4/4] & 0xFFFF;
		uint16_t *colors = (uint16_t *)(object2 + 0x120/4);
		
		uint32_t key = (cms_entry << 16) | costume;
		
		auto it = pcac_colors.find(key);
		if (it != pcac_colors.end())
		{
			do_call = true;		
			memset(colors, 0xFF, 25*sizeof(uint16_t));
			
			const BcsColorsMap &map = it->second;			
			
			for (auto &it2 : map.map)
			{
				auto it3 = bcs_part_to_index.find(it2.first);
				if (it3 != bcs_part_to_index.end())
				{
					colors[it3->second] = it2.second;
				}
			}
		}
		
		else if (cms_entry >= 0x9F) // Don't do the call for mods, otherwise some mods like Towifla break
		{
			do_call = false;
		}
	}
	
	if (do_call)
		ApplyCacMats(object, arg2, mat);
}

PUBLIC void ApplyCacMats_Setup(ApplyCacMatsType orig)
{
	ApplyCacMats = orig;
}

PUBLIC int IsOzaruReplacement(const char *char_code)
{
	for (const std::string &ozaru : ozarus)
	{
		if (strncmp(ozaru.c_str(), char_code, 3) == 0)
			return 1;
	}
		
	return 0;	
}

PUBLIC int IsCellMaxReplacement(const char *char_code)
{
	for (const std::string &cm : cell_maxes)
	{
		if (strncmp(cm.c_str(), char_code, 3) == 0)
			return 1;
	}
		
	return 0;	
}

PUBLIC void SetBodyShape_Setup(SetBodyShapeType orig)
{
	SetBodyShape = orig;
}

PUBLIC uint64_t SetBodyShape_Patched(void *object, int32_t body_shape, int32_t arg3, float arg4)
{
	// Note, if object offsets of this function change, change the ones in ApplyCacMatsPatched too 
	if (body_shape < 0 && object)
	{
		uint64_t *object64 = (uint64_t *)object;
		uint32_t *object2 = (uint32_t *)object64[0x30/8];
		
		if (object2)
		{
			uint32_t cms_entry = object2[0xD0/4];		

			//DPRINTF("*********** CMS ENTRY = 0x%x\n", cms_entry);
			
			if (cms_entry < LOOKUP_SIZE && body_shapes_lookup[cms_entry] != 0xFF)
				body_shape = body_shapes_lookup[cms_entry];
		}
	}
	
	return SetBodyShape(object, body_shape, arg3, arg4);
}

PUBLIC void MobSetDefaultBody_Setup(MobSetDefaultBodyType orig)
{
	MobSetDefaultBody = orig;
}

PUBLIC void MobSetDefaultBody_Patched(Battle_Mob *pthis, int32_t edx)
{
	int32_t backup = pthis->body;
	if (backup < 0)
	{
		int32_t set_body = 0;
		if (pthis->cms_id < LOOKUP_SIZE && body_shapes_lookup[pthis->cms_id] != 0xFF)
		{
			set_body = body_shapes_lookup[pthis->cms_id];
		}
		pthis->body = set_body;
	}
	
	MobSetDefaultBody(pthis, edx);
	
	if (backup < 0)
	{
		pthis->body = backup;
	}
}

PUBLIC int UseAutobattlePortrait(int32_t cms_entry)
{
	if (cms_entry >= 0 && cms_entry < LOOKUP_SIZE && auto_btl_portrait_lookup[cms_entry])
		return 1;
	
	// Otherwise, do the same as game implementation	
	if (cms_entry >= 0x64 && cms_entry <= 0x6D) // Since 1.14 they changed the implementation and the cac range is now 0x64-0x6D, and not 0x64-0x77 like before
		return 1;
	
	return 0;
}

PUBLIC void ResultPortraits2Setup(ResultPortraits2Type orig)
{
	ResultPortraits2 = orig;
}

PUBLIC uint64_t ResultPortraits2Patched(Battle_HudCockpit *pthis, int idx, void *r8, void *r9, void *arg5, void *arg6, void *arg7, void *arg8)
{
	uint32_t *undo = nullptr;
	
	if (idx >= 0 && idx < 0xE)
	{
		Battle_HudCharInfo *info = &pthis->char_infos[idx];
		uint32_t cms_entry = info->cms_entry;
		
		//DPRINTF("-----------Cms entry is 0x%x 0x%x\n", cms_entry, info->cms_entry2);
				
		if (cms_entry < LOOKUP_SIZE && auto_btl_portrait_lookup[cms_entry])
		{
			undo = &info->is_cac;
			
			if (*undo == 0)
				*undo = 1;
			else
				undo = nullptr;
		}
	}
	
	uint64_t ret = ResultPortraits2(pthis, idx, r8, r9, arg5, arg6, arg7, arg8);
	
	if (undo)
		*undo = 0;
	
	return ret;
}

// This patch is very sensitive. On any change in patch signature, it MUST BE REDONE
PUBLIC void CusAuraMapPatch(uint8_t *buf)
{
	// movsxd  rcx, eax
	PatchUtils::Write8(buf, 0x48); PatchUtils::Write8(buf+1, 0x63); PatchUtils::Write8(buf+2, 0xC8);	
	PatchUtils::Write16((uint16_t *)(buf+3), 0xBA48); // mov rdx, XXXXXXXXXXXXXX
	
	uint64_t gra_addr = (uint64_t)GetRealAura;
	PatchUtils::Write64((uint64_t *)(buf+5), gra_addr);
	PatchUtils::Write16((uint16_t *)(buf+13), 0xE2FF); // jmp rdx
	
	uint64_t *ret_addr = (uint64_t *)(gra_addr+0x1E);
	if (*ret_addr != 0x123456789ABCDEF)
	{
		UPRINTF("Internal error in CusAuraMapPatch\n");
		exit(-1);
	}
	
	PatchUtils::Write64(ret_addr, (uint64_t)(buf+0x135)); // buf+0x135 -> the address of end of switch	
}

// This patch is very sensitive. On any change in patch signature, it MUST BE REDONE
PUBLIC void CusAuraPatchBH11(uint8_t *buf)
{
	PatchUtils::Write16((uint16_t *)buf, 0xB948); // mov rcx, XXXXXXXXXXXXXX
	
	uint64_t g11_addr = (uint64_t)GetCABehaviour11;
	PatchUtils::Write64((uint64_t *)(buf+2), g11_addr);
	PatchUtils::Write16((uint16_t *)(buf+10), 0xE1FF); // jmp rcx
	
	uint64_t *ret_addr = (uint64_t *)(g11_addr+0x32);
	if (*ret_addr != 0x123456789ABCDEF)
	{
		UPRINTF("Internal error in CusAuraPatchBH11\n");
		exit(-1);
	}
	
	PatchUtils::Write64(ret_addr, (uint64_t)(buf+0xC)); // buf+0xC -> address to return to
}

// This patch is very sensitive. On any change in patch signature, it MUST BE REDONE
PUBLIC uint32_t CusAuraPatchInt2(Battle_Mob *pthis)
{
	if (pthis->IsTransformed()) // if awaken skill? needs confirm...
	{
		uint32_t cus_aura = (uint32_t)pthis->trans_control;
		
		if (cus_aura == 0xFFFFFFFF)
			return 0xFFFFFFFF;
		
		if (cus_aura < LOOKUP_SIZE)
			return cus_aura_int2_lookup[cus_aura];
		
		return 0;
	}
	
	return 0xFFFFFFFF;
}

PUBLIC uint32_t CusAuraPatchInt3(Battle_Mob *pthis)
{
	if (pthis->IsTransformed()) 
	{
		uint32_t cus_aura = (uint32_t) pthis->trans_control;
		
		if (cus_aura < LOOKUP_SIZE)
			return cus_aura_int3_lookup[cus_aura];
	}
	
	return 0;
}

// This patch is very sensitive. On any change in patch signature, it MUST BE REDONE
PUBLIC void CusAuraPatchTeleport(uint8_t *buf)
{
	PatchUtils::Write16((uint16_t *)buf, 0xB948); // mov rcx, XXXXXXXXXXXXXX
	
	uint64_t ft_addr = (uint64_t)ForceTeleport;
	PatchUtils::Write64((uint64_t *)(buf+2), ft_addr);
	PatchUtils::Write16((uint16_t *)(buf+10), 0xE1FF); // jmp rcx
	
	uint64_t *ret_addr1 = (uint64_t *)(ft_addr+0x42);
	if (*ret_addr1 != 0x123456789ABCDEF)
	{
		UPRINTF("Internal error in CusAuraPatchTeleport\n");
		exit(-1);
	}
	
	PatchUtils::Write64(ret_addr1, (uint64_t)(buf+0x12)); // buf+0x12 -> address return for teleport
	
	uint64_t *ret_addr2 = (uint64_t *)(ft_addr+0x4E);
	if (*ret_addr2 != 0xFEDCBA987654321)
	{
		UPRINTF("Internal error in CusAuraPatchTeleport (2)\n");
		exit(-1);
	}
	
	PatchUtils::Write64(ret_addr2, (uint64_t)(buf+0x999)); // buf+0x999 -> address return for no teleport   
}

PUBLIC uint32_t Behaviour13(Battle_Mob *pthis)
{
	uint32_t cus_aura = (uint32_t) pthis->trans_control;
	
	if (cus_aura < LOOKUP_SIZE && cus_aura_bh13_lookup[cus_aura] <= BEHAVIOUR_MAX)
		cus_aura = cus_aura_bh13_lookup[cus_aura];

	return cus_aura;
}

typedef void (* SetBcsColorType)(void *, int, uint32_t, const char *);
typedef void (* SaveBcsType)(void *, uint64_t, const char *);

void SetBcsColor(void *pthis, int color, int transform, const char *part, bool save)
{	
	uint64_t *vtable = (uint64_t *) *(uint64_t *)pthis;
	SetBcsColorType SetBcsColorV = (SetBcsColorType)vtable[0x408/8];
	SaveBcsType Save1 = (SaveBcsType)vtable[0x3F8/8];
	SaveBcsType Save2 = (SaveBcsType)vtable[0x400/8];
	
	//DPRINTF("Setting %s:%d (transform=%d,save=%d)\n", part, color, transform, save);
	
	if (transform && save)
	{
		Save1(pthis, 0, part);
		Save2(pthis, 0, part);
	}
	
	SetBcsColorV(pthis, color, transform, part);
	
	if (!transform && save)
	{
		Save1(pthis, 1, part);
		Save2(pthis, 1, part);
	}
}	

void SetBcsColors(void *pthis, Battle_Mob *mob, int transform, bool from_bh10_fail)
{
	int hair_color = -1;
	int eyes_color = -1;
	
	if (mob && mob->trans_control >= 0 && mob->trans_control < LOOKUP_SIZE)
	{
		hair_color = (int)bcs_hair_colors[mob->trans_control];
		eyes_color = (int)bcs_eyes_colors[mob->trans_control];
	}	
	
	if (hair_color >= 0)
	{
		SetBcsColor(pthis, hair_color, transform, "HAIR_", from_bh10_fail);		
	}
	
	if (eyes_color >= 0)
	{
		SetBcsColor(pthis, eyes_color, transform, "eye_", from_bh10_fail);
	}	
	
	// Additional colors (4.2)
	if (mob && mob->trans_control >= 0)
	{
		auto it = bcs_additional_colors.find(mob->trans_control);
		if (it != bcs_additional_colors.end())
		{
			std::vector<std::string> parts;
			
			if (Utils::GetMultipleStrings(it->second, parts) > 0)
			{
				for (const std::string &part : parts)
				{
					std::vector<std::string> lr;
					if (Utils::GetMultipleStrings(part, lr, ':') == 2)
					{
						uint32_t color = Utils::GetUnsigned(lr[1], 0xDEADC0DE);
						if (color != 0xDEADC0DE)
						{							
							static std::unordered_set<std::string> not_save = { "HAIR_", "eye_", "SKIN_" }; // These are already saved by the game (if bh10) or by our previous code (if not bh10)
							bool save = (not_save.find(part) == not_save.end());
							
							SetBcsColor(pthis, color, transform, lr[0].c_str(), save);
						}
						else
						{
							DPRINTF("%s: Bad number string \"%s\" in \"%s\"\n", FUNCNAME, lr[1].c_str(), part.c_str());
						}
					}
					else
					{
						DPRINTF("%s: Unrecognized additional color string \"%s\"\n", FUNCNAME, part.c_str());
					}
				}
			}
		}
	}
}

void SetBcsColorsTransform(void *pthis, Battle_Mob *mob, uint64_t, const char *)
{
	SetBcsColors(pthis, mob, 1, false);
}

void SetBcsColorsUntransform(void *pthis, Battle_Mob *mob, uint64_t, const char *)
{
	SetBcsColors(pthis, mob, 0, false);
}

PUBLIC void ApplyBcsHairColorPatchUntransform(uint8_t *addr)
{
	// First patch, destroy the conditional
	PatchUtils::Write16(addr+7, 0x9090);
	
	// Second patch, replace edx=60 by rdx=rdi=Battle_Mob ptr 
	PatchUtils::Write32(addr+0x1D, 0x90FA8948); // mov rdx, rdi; nop
		
	// Third patch, hook the method 0x400	
	PatchUtils::Write16(addr+0x21, 0xE890);
	PatchUtils::HookCall(addr+0x21+1, nullptr, (void *)SetBcsColorsUntransform);
	
	// Fourth patch, skip eyes call
	PatchUtils::Nop(addr+0x3F, 6);
	
	// Fifth patch, skip SSJ blue evolution code 
	PatchUtils::Write8(addr+0x4C, 0xEB); // jne to jmp	
}

PUBLIC void ApplyBcsHairColorPatchTransform(uint8_t *addr)
{
	// First patch, destroy the conditional
	PatchUtils::Write16(addr+7, 0x9090);
	
	// Second patch, replace edx=60 by rdx=rbx=Battle_Mob ptr 
	PatchUtils::Write32(addr+0x1A, 0x90DA8948); // mov rdx, rbx; nop
	PatchUtils::Write8(addr+0x1A+4, 0x90);
		
	// Third patch, hook the method 0x400	
	PatchUtils::Write16(addr+0x23, 0xE890);
	PatchUtils::HookCall(addr+0x23+1, nullptr, (void *)SetBcsColorsTransform);
	
	// Fourth patch, skip eyes call
	PatchUtils::Nop(addr+0x43, 6);
	
	// Fifth patch, skip SSJ blue evolution code 
	PatchUtils::Write8(addr+0x50, 0xEB); // jne to jmp	
}

PUBLIC void Behaviour10Setup(Behaviour10FuncType orig)
{
	Behaviour10Func = orig;
}

PUBLIC int Behaviour10FuncPatched(Battle_Mob *pthis, uint32_t cus_aura, uint32_t *unk)
{
	if (cus_aura < LOOKUP_SIZE && cus_aura_bh10_lookup[cus_aura] <= BEHAVIOUR_MAX)
		cus_aura = cus_aura_bh10_lookup[cus_aura];
	
	int ret = Behaviour10Func(pthis, cus_aura, unk);
	if (ret <= 0)
	{
		bool transform = (BRA(0) != bh10_untransform_ra);
		SetBcsColors(pthis->common_chara, pthis, transform, true);
	}
	
	return ret;
}

PUBLIC void OnBH10UntransformCallLocated(uint8_t *addr)
{
	bh10_untransform_ra = (void *)(addr + 5);
}

typedef void (* RemoveAccessoriesType)(void *, int);

void RemoveAccessoriesPatched(void *pthis, Battle_Mob *mob)
{
	bool do_call = false;
	
	if (mob && mob->trans_control >= 0 && mob->trans_control < LOOKUP_SIZE)
	{
		uint8_t remove = remove_hair_accessories_lookup[mob->trans_control];
		
		if (remove == 0)
		{
			// Nothing, do_call is already false
		}
		else if (remove == 1)
		{
			do_call = true;
		}
		else
		{
			// Default game behaviour, restoring here the destroyed conditionals			
			if (mob->trans_control == 2)
			{
				if (mob->IsCac())
				{
					do_call = true;
				}
			}
		}
	}
	
	//DPRINTF("Remove hair accesory: 0x%x %d\n", mob->trans_control, do_call);
	
	if (do_call)
	{
		uint64_t *vtable = (uint64_t *) *(uint64_t *)pthis;
		RemoveAccessoriesType RemoveAccessories = (RemoveAccessoriesType) vtable[0x3A0/8];
		
		RemoveAccessories(pthis, 5); // 5 = hair part
	}
}

PUBLIC void ApplyConditionalRemoveHairAccessories(uint8_t *addr)
{
	// First patch, destroy first conditional
	PatchUtils::Write16(addr+7, 0x9090);
	
	// Second patch, destroy the second conditional
	PatchUtils::Write16(addr+16, 0x9090);
	
	// Third patch, replace edx=5 by rdx=rbx=Battle_Mob ptr
	PatchUtils::Write32(addr+28, 0x90DA8948); // mov rdx, rbx; nop
	PatchUtils::Write8(addr+28+4, 0x90);
	
	// Fourth patch, hook the method +0x398
	PatchUtils::Write16(addr+33, 0xE890);
	PatchUtils::HookCall(addr+33+1, nullptr, (void *)RemoveAccessoriesPatched);
}

typedef void (* ChangePartsetType)(Battle_Mob *, int, int, int);
static ChangePartsetType ChangePartset;

bool test_144 = false;

PUBLIC void SetupChangePartset(ChangePartsetType orig)
{
	test_144 = true;
	ChangePartset = orig;
}

PUBLIC void ChangePartsetPatched(Battle_Mob *pthis, int partset, int unk1, int trans_control)
{
	//DPRINTF("Change partset: cms 0x%x partset=%d unk1=%d trans_control=0x%x default=%d\n", pthis->cms_id, partset, unk1, trans_control, pthis->default_partset);
	if (partset == 272 && unk1 == 1 && (trans_control == 0x32 || trans_control == 0x33))
	{
		pthis->default_partset = 273;
	}
	else if (partset == 280 && unk1 == 1 && trans_control == 0x36)
	{
		pthis->default_partset = 281;
	}
	
	ChangePartset(pthis, partset, unk1, trans_control);
}

typedef void (* UntransformType)(Battle_Mob *, int);
static UntransformType Untransform;

static bool untransform_exclude_lookup[LOOKUP_SIZE];
static bool apply_to_roster;

PUBLIC void SetupUntransform(UntransformType orig)
{
	Untransform = orig;
	
	std::vector<int> exclude_trans;
	ini.GetMultipleIntegersValues("UNTRANSFORM_CUT_SCENES", "exclude_trans", exclude_trans);
	
	memset(untransform_exclude_lookup, 0, sizeof(untransform_exclude_lookup));
	
	for (int trans : exclude_trans)
	{
		if (trans >= 0 && trans < LOOKUP_SIZE)
			untransform_exclude_lookup[trans] = true;
	}
	
	ini.GetBooleanValue("UNTRANSFORM_CUT_SCENES", "apply_to_roster", &apply_to_roster, false);
}

PUBLIC void PatchUntransform(Battle_Mob *pthis, int unk1)
{
	//DPRINTF("Untransform cut scene: 0x%x\n", pthis->cms_id);
	if (apply_to_roster || (pthis->cms_id >= 0x64 && pthis->cms_id <= 0x6D))
	{
		bool exclude = false;
		
		if (pthis->trans_control >= 0 && pthis->trans_control < LOOKUP_SIZE && untransform_exclude_lookup[pthis->trans_control])
			exclude = true;
		
		if (!exclude)
			return;
	}	
	
	Untransform(pthis, unk1);
}

typedef StdString * (* StringAppendType)(StdString *, const char *, size_t);
static StringAppendType StringAppend;

PUBLIC void SetupStringAppend(StringAppendType orig)
{
	StringAppend = orig;
}

PUBLIC StdString *TtcFilesAlias(StdString *path, const char *cms, size_t append_len)
{
	if (cms)
	{
		auto it = ttc_files_map.find(cms);
		
		if (it != ttc_files_map.end())
		{
			cms = it->second.c_str();
		}
	}
	
	return StringAppend(path, cms, append_len);
}

extern void CanUseAnyDualSkill();

PUBLIC void AnyDualSkillPatch(uint8_t *buf)
{
	PatchUtils::Write16((uint16_t *)buf, 0xB848); // mov rax, XXXXXXXXXXXXXX
	
	uintptr_t cds_addr = (uintptr_t) CanUseAnyDualSkill;
	PatchUtils::Write64((uint64_t *)(buf+2), cds_addr);
	PatchUtils::Write16((uint16_t *)(buf+10), 0xE0FF); // jmp rax
	PatchUtils::Write8(buf+12, 0x90); // nop  (not really necessary, code wont reach here, but to not destroy asm view in a debugger)
	
	uint64_t *ret_addr1 = (uint64_t *)(cds_addr+0x28);
	if (*ret_addr1 != 0x123456789ABCDEF)
	{
		UPRINTF("Internal error in AnyDualSkillPatch\n");
		exit(-1);
	}
	
	PatchUtils::Write64(ret_addr1, (uint64_t)(buf+0xD)); // buf+0xD -> address return for default dual skill usage
	
	uint64_t *ret_addr2 = (uint64_t *)(cds_addr+0x34);
	if (*ret_addr2 != 0xFEDCBA987654321)
	{
		UPRINTF("Internal error in AnyDualSkillPatch (2)\n");
		exit(-1);
	}
	
	PatchUtils::Write64(ret_addr2, (uint64_t)(buf+0xBD)); // buf+0xBD -> address return for any dual skill
}

extern void AurToBpeMap();

PUBLIC void AurToBpePatch(uint8_t *buf) // Reminder: buf points to the instruction with the "ja"
{
	PatchUtils::Write16(buf, 0xE990); // nop + change conditional to inconditional
	
	if (!PatchUtils::HookCall((void *)(buf+1), nullptr, (void *)AurToBpeMap))
	{
		UPRINTF("Internal error in AurToBpePatch (HookCall failed.\n");
		exit(-1);
	}
	
	uintptr_t abm_addr = (uintptr_t)AurToBpeMap;
	uint64_t *ifbpe_ra = (uint64_t *)(abm_addr+0x19);
	if (*ifbpe_ra != 0x123456789ABCDEF)
	{
		UPRINTF("Internal error in AurToBpeMap\n");
		exit(-1);
	}
	
	PatchUtils::Write64(ifbpe_ra, (uint64_t)(buf+0x60)); // buf+0x60, address to return to if bpe mapping is done
	
	uint64_t *ifnotbpe_ra = (uint64_t *)(abm_addr+0x25);
	if (*ifnotbpe_ra != 0xFEDCBA987654321)
	{
		UPRINTF("Internal error in AurToBpeMap (2)\n");
		exit(-1);
	}
	
	PatchUtils::Write64(ifnotbpe_ra, (uint64_t)(buf+0xDA)); // buf+0xDA, address of original default case
}

static void AurBpeCase1(void *pthis, uint32_t aur, uint32_t flags, uint32_t unk)
{
	if (aur < LOOKUP_SIZE && aur_bpe_flag1[aur])
		AurBpe(pthis, aur, flags, unk);
	// else nothing
}

PUBLIC void AurBpeFlag1Patch(uint8_t *buf)
{
	// First patch, bye to the conditional
	PatchUtils::Nop(buf+0xB, 6);
	// Second patch hook call to the function
	if (!PatchUtils::HookCall((void *)(buf+0x2A), (void **)&AurBpe, (void *)AurBpeCase1))
	{
		UPRINTF("Internal error in AurBpeFlag1Patch.\n");
		exit(-1);
	}
}

PUBLIC void AurBpeCase2(void *pthis, uint32_t aur, uint32_t flags, uint32_t unk)
{
	if (aur < LOOKUP_SIZE && aur_bpe_flag2[aur] != flags)
		flags = aur_bpe_flag2[aur];
	
	AurBpe(pthis, aur, flags, unk);
}

// This patch is very sensitive. On any change in patch signature, it MUST BE REDONE
PUBLIC void CusAuraPatchBH64(uint8_t *buf)
{
	PatchUtils::Write16((uint16_t *)buf, 0xB948); // mov rcx, XXXXXXXXXXXXXX
	
	uint64_t g64_addr = (uint64_t)GetCABehaviour64;
	PatchUtils::Write64((uint64_t *)(buf+2), g64_addr);
	PatchUtils::Write16((uint16_t *)(buf+10), 0xE1FF); // jmp rcx
	
	uint64_t *ret_addr = (uint64_t *)(g64_addr+0x27);
	if (*ret_addr != 0x123456789ABCDEF)
	{
		UPRINTF("Internal error in CusAuraPatchBH64\n");
		exit(-1);
	}
	
	PatchUtils::Write64(ret_addr, (uint64_t)(buf+0xC)); // buf+0xC -> address to return to
}

// EEPK section

typedef void (* LoadEEPKEntryType)(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef AURHeader * (* GetAuraFileType)(void *);
typedef int32_t (* AuraFunc1Type)(void *);
typedef void (* LoadEEPKType)(void *, void *, uint32_t, uint32_t, void *, void *, void *);

static LoadEEPKEntryType LoadEEPKEntry;
static GetAuraFileType GetAuraFile;
static AuraFunc1Type AuraFunc1;
static LoadEEPKType LoadEEPK;

void **srm_singleton;

PUBLIC void SetupLoadEEPKEntry(void *addr)
{
	LoadEEPKEntry = (LoadEEPKEntryType)addr;
}

PUBLIC void LoadEEPKEntryPatched(void *pthis, uint32_t edx, uint32_t idx, uint32_t r9d, uint32_t sp20, uint32_t sp28)
{
	LoadEEPKEntry(pthis, edx, idx, r9d, sp20, sp28);
	
	if (idx == 6) // Last call in the loop
	{
		for (idx=80; idx < 90; idx++)
		{
			//DPRINTF("Loading eepk entry %d\n", idx);
			LoadEEPKEntry(pthis, edx, idx, r9d, sp20, sp28);
		}
	}
}

PUBLIC void SetupSystemResourceManager_singleton(void *address)
{
	srm_singleton = (void **)GetAddrFromRel(address);
}

PUBLIC void SetupGetAuraFile(void *address)
{
	GetAuraFile = (GetAuraFileType)GetAddrFromRel(address);
}

PUBLIC void SetupAuraFunc1(void *address)
{
	AuraFunc1 = (AuraFunc1Type)GetAddrFromRel(address);
}

PUBLIC void SetupLoadEEPK(void *addr)
{
	LoadEEPK = (LoadEEPKType)addr;
}

PUBLIC void LoadEEPKPatched(void *pthis, void *rdx, uint32_t r8d, void *rbx, void *sp20, void *sp28, void *sp30)
{
	//DPRINTF("LoadEEPK\n");
	uint32_t eepk = 1;
	
	if (srm_singleton && *srm_singleton)
	{
		AURHeader *hdr = GetAuraFile(*srm_singleton);
		if (hdr)
		{
			int32_t aur1 = AuraFunc1((void *)PatchUtils::Read64(rbx, 0));
			//int32_t aur2 = (int32_t)PatchUtils::Read32(rbx, 0xC);
			//DPRINTF("Aur 1 = %d, Aur2 = %d\n", aur1, aur2);
			//if (aur1 == aur2 && aur1 >= 0)
			if (true)
			{
				AURAura *auras = (AURAura *)(((uint8_t *)hdr) + hdr->auras_offset);
				
				if (auras[aur1].unk_04 != 0)
				{
					eepk = auras[aur1].unk_04;
					DPRINTF("EEPK changed to %d.\n", eepk);
				}
			}
			
		}
	}
	
	LoadEEPK(pthis, rdx, r8d, eepk, sp20, sp28, sp30);
}

void GoldenFreezerSkinBehaviour(void *common_chara, Battle_Mob *mob)
{
	if (mob->trans_control >= 0 && mob->trans_control < LOOKUP_SIZE && cus_aura_gfs_bh[mob->trans_control])
	{
		PatchUtils::InvokeVirtualRegisterFunction(common_chara, 0x3F8, 0, (uintptr_t)"SKIN_");
		PatchUtils::InvokeVirtualRegisterFunction(common_chara, 0x400, 0, (uintptr_t)"SKIN_");
	}
}

PUBLIC void OnGoldenFreezerSkinBehaviourLocated(uint8_t *addr, size_t size)
{
	// mov rcx, [rbx+4C8h]; nop
	PatchUtils::Write64(addr, 0x90000004C88B8B48); addr += 8; size -= 8;
	// mov rdx, rbx; nop
	PatchUtils::Write32(addr, 0x90DA8948); addr += 4; size -= 4;
	// Set call to my code
	PatchUtils::Write8(addr, 0xE8);
	PatchUtils::HookCall(addr, nullptr, (void *)GoldenFreezerSkinBehaviour); addr += 5; size -= 5;
	// Nop the remaining code
	//DPRINTF("Noped size: %Id\n", size);
	PatchUtils::Nop(addr, size);
}

} // extern "C"