#include <windows.h>
#include <wininet.h>
#include "IniFile.h"
#include "EPatchFile.h"
#include "PatchUtils.h"
#include "Mutex.h"
#include "DBXV2/QsfFile.h"
#include "DBXV2/IdbFile.h"
#include "xv2patcher.h"
#include "chara_patch.h"
#include "stage_patch.h"
#include "ui_patch.h"
#include "autogenportrait_dumper.h"
#include "ai_patch.h"
#include "mob_control.h"
#include "debug.h"

Battle_Core_MainSystem **pms_singleton;

static HMODULE patched_dll;
static Mutex mutex;
static int num_patches_fail;
static bool count_patches_failures;
HMODULE myself;
std::string myself_path;
IniFile ini;
DWORD initial_tick;

static bool in_game_process()
{
	char szPath[MAX_PATH];
	
	GetModuleFileName(NULL, szPath, MAX_PATH);
	_strlwr(szPath);
	
	// A very poor aproach, I know
	return (strstr(szPath, PROCESS_NAME) != NULL);
}

extern "C"
{
	
#ifdef DINPUT

	PUBLIC HRESULT DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, void *punkOuter)
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return E_INVALIDARG;
	}
	
	PUBLIC HRESULT DllCanUnloadNow()
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return -1;
	}
	
	PUBLIC HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return 0x80040111;
	}
	
	PUBLIC HRESULT DllRegisterServer()
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return E_UNEXPECTED;
	}
	
	PUBLIC HRESULT DllUnregisterServer()
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return -1;
	}

#else
	
	PUBLIC DWORD XInputGetState()
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	
	PUBLIC DWORD XInputSetState()
	{
		DPRINTF("%s ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	
	PUBLIC DWORD XInputGetBatteryInformation(DWORD,  BYTE, void *)
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	
	PUBLIC void XInputEnable(BOOL)
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
	}
	
	PUBLIC DWORD XInputGetCapabilities(DWORD, DWORD, void *)
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	
	PUBLIC DWORD XInputGetDSoundAudioDeviceGuids(DWORD, void *, void *)
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	
	PUBLIC DWORD XInputGetKeystroke(DWORD, DWORD, void *)
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	
	PUBLIC DWORD XInputGetStateEx(DWORD, void *)
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	
	PUBLIC DWORD XInputWaitForGuideButton(DWORD, DWORD, void *)
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	
	PUBLIC DWORD XInputCancelGuideButtonWait()
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	
	PUBLIC DWORD XInputPowerOffController()
	{
		DPRINTF("%s: ****************I have been called but I shouldn't!!!\n", FUNCNAME);
		return ERROR_DEVICE_NOT_CONNECTED;
	}
#endif
}

extern "C"
{

static bool xv1_cpk_exists = false;
static bool unlock_bgm1 = false;
static bool unlock_bgm2 = false;
static bool test_mode = false;
static bool mode_6 = true;

typedef int (* Type73)(int arg0, bool arg1);
typedef int (* Type138)(int arg0);
typedef void *(* ChaselCtorType)(void *, uint32_t, uint32_t, void *, void *, void *, void *, void *, void *, void *);
typedef void *(* ChaselDtorType)(void *, uint32_t);
typedef bool (* SetupAlliesType)(void *, int);

static Type73 func73;
static Type138 func138;
static uint32_t *pE74;

static ChaselCtorType ChaselCtor;
static ChaselDtorType ChaselDtor;
static SetupAlliesType SetupAllies;

// Removed in 1.15 due to the patch change
// static uint8_t *cs_num_allies;
static bool patch_num_allies = false; // Added for game 1.15

PUBLIC bool XV1CpkExists()
{
	static uint8_t pw_sha1[20] = {
		0x1A, 0x34, 0xC4, 0xB9, 0xD5, 0x36, 0xAE, 0xF3, 0x2C, 0x5A, 
		0x6C, 0xFA, 0x1A, 0x2D, 0x82, 0x58, 0x08, 0x12, 0xA7, 0x87
	};
	
	std::string pw;
	uint8_t sha1[20];
	ini.GetStringValue("Patches", "test_mode_pw", pw);
	
	if (pw.length() > 0)
	{
		Utils::Sha1(pw.c_str(), pw.length(), sha1);
		test_mode = (memcmp(sha1, pw_sha1, sizeof(sha1)) == 0); 
	}
	
	if (test_mode)	
		xv1_cpk_exists = Utils::FileExists(myself_path + std::string(CONTENT_ROOT) + "cpk/data_d4_5_xv1.cpk");
	
	unlock_bgm1 = Utils::FileExists(myself_path + std::string(CONTENT_ROOT) + "cpk/data_d2_bgm1.cpk");
	unlock_bgm1 = (unlock_bgm1 && test_mode);
	
	unlock_bgm2 = Utils::FileExists(myself_path + std::string(CONTENT_ROOT) + "cpk/data_d7_bgm2.cpk");
	unlock_bgm2 = (unlock_bgm2 && test_mode);
	
	return (xv1_cpk_exists || test_mode || unlock_bgm1 || unlock_bgm2);
}

PUBLIC void SetupFunction73(Type73 orig)
{
	func73 = orig;
}

PUBLIC void OnE74Located(void *addr)
{
	pE74 = (uint32_t *)GetAddrFromRel(addr, 1);
	//DPRINTF("%p\n", pE74);
}

// 1.21: param arg1 is added (purpose: unknown, seems to be true in most/all calls)
PUBLIC int Function73(int arg0, bool arg1)
{
	int ret = func73(arg0, arg1);
	//DPRINTF("Arg0 = 0x%x, arg1=0x%x, ret=0x%x\n", arg0, arg1, ret);	
	
	if (test_mode && pE74)
	{
		static bool done = false;
		
		if (!done)
		{
			done = true;
			
			*pE74 = 0xF;
			return Function73(arg0, arg1);
		}
	}
	
	if (xv1_cpk_exists)
	{
		if (arg0 == 0x15)		
			return 1;
	}
	
	if (unlock_bgm1)
	{	
		if (arg0 == 0x13)
			return 1;
	}
	
	if (unlock_bgm2)
	{
		if (arg0 == 0x14)
			return 1;
	}
	
	if (test_mode)
	{	
		if (arg0 == 0x16 || arg0 == 0x17 || arg0 == 0x1E || arg0 == 0x1F || arg0 == 0x23 || arg0 == 0x24 || arg0 == 0x2A || arg0 == 0x2C || arg0 == 0x2E || arg0 == 0x30 || arg0 == 0x32)
			return 1;
	}
	
	return ret;
}

PUBLIC void SetupFunction138(Type138 orig)
{
	func138 = orig;
}

PUBLIC int Function138(int arg0)
{
	int ret = func138(arg0);
	//DPRINTF("Argg = 0x%x, ret=0x%x\n", arg0, ret);	
	
	if (xv1_cpk_exists)
	{
		if (arg0 == 0x14)		
			return 1;
	}
	
	if (unlock_bgm1)
	{	
		if (arg0 == 0x12)
			return 1;
	}
	
	if (unlock_bgm2)
	{
		if (arg0 == 0x13)
			return 1;
	}
	
	if (test_mode)
	{	
		if (arg0 == 0x15 || arg0 == 0x16 || arg0 == 0x1A || arg0 == 0x1B || arg0 == 0x1C || arg0 == 0x1D || arg0 == 0x1F || arg0 == 0x20 || arg0 == 0x21 || arg0 == 0x22 || arg0 == 0x23)
			return 1;
	}
	
	return ret;
}

PUBLIC void SetupChaselCtor(ChaselCtorType orig)
{
	ChaselCtor = orig;
	
	ini.GetBooleanValue("MULTISELECT_EXPERT", "use_mode_6", &mode_6, true);
}

/* 
 * Removed in 1.15 due to the patch being changed
PUBLIC void OnNumAlliesLocated(uint8_t *ptr)
{
	cs_num_allies = ptr;
}
*/

static void *ChaselDtorHooked(void *pthis, uint32_t dtor_mode)
{
	//DPRINTF("Chasel dtor called, this=%p\n", pthis);
	
	/* Implementation changed in 1.15
	if (*cs_num_allies != 2)
	{
		PatchUtils::Write8(cs_num_allies, 2);
	}
	*/
	// New implementation
	patch_num_allies = false;	
	return ChaselDtor(pthis, dtor_mode);
}


PUBLIC void *ChaselCtorHooked(void *pthis, uint32_t mode, uint32_t r8d, void *r9, void *sp_20, void *sp_28, void *sp_30, void *sp_38, void *sp_40, void *sp_48)
{
	bool patch = false;
	//DPRINTF("Chasel ctor, this=%p\n", pthis);
	
	// Makes chasel appear in expert missions
	if (mode == 0 && r8d == 0)
	{
		if (mode_6)
		{
			mode = 3;  // 3 -> 3vs3   
			patch = true;
		}
		else
		{
			mode = 5; // Single 1-3 team
		}
	}
	
	void *ret = ChaselCtor(pthis, mode, r8d, r9, sp_20, sp_28, sp_30, sp_38, sp_40, sp_48);
	
	if (ret && patch)
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
		
		// Implementation changed in 1.15 due to the patch changed
		//PatchUtils::Write8(cs_num_allies, 5);
		
		// New implementation
		patch_num_allies = true;
	}
	
	return ret;
}

PUBLIC bool SetupAlliesPatched(void *pthis, void *rbp)
{
	int num = 2;
	
	if (patch_num_allies)
	{
		num = 5;
		PatchUtils::WriteData64(rbp, num, -0x28);
	}
	
	return SetupAllies(pthis, num);
}

PUBLIC void SetupSetupAllies(SetupAlliesType orig)
{
	SetupAllies = orig;
}

#define XV2_PATCHER_TAG	0x50325658 /* XV2P */

typedef int (* ExternalAS3CallbackType)(void *custom_arg, void *iggy_obj, const char **pfunc_name);
typedef void *(* IggyPlayerCallbackResultPathType)(void *unk0);
typedef void (* IggyValueSetStringUTF8RSType)(void *arg1, void *unk2, void *unk3, const char *str, size_t length);
typedef void (* IggyValueSetS32RSType)(void *arg1, uint32_t unk2, uint32_t unk3, uint32_t value);
typedef void (* _Battle_Mob_Destructor)(void *);

static ExternalAS3CallbackType ExternalAS3Callback;
static IggyPlayerCallbackResultPathType IggyPlayerCallbackResultPath;
static IggyValueSetStringUTF8RSType IggyValueSetStringUTF8RS;
static IggyValueSetS32RSType IggyValueSetS32RS;
static _Battle_Mob_Destructor Battle_Mob_Destructor;

PUBLIC int ExternalAS3CallbackPatched(void *custom_arg, void *iggy_obj, const char **pfunc_name)
{
	//DPRINTF("ExternalAS3Callback: %s\n", *func_name);
		
	if (pfunc_name && *pfunc_name)
	{
		const char *func_name = *pfunc_name;
		
		if (strlen(func_name) > 4 && *(uint32_t *)func_name == XV2_PATCHER_TAG)
		{		
			//DPRINTF("Calling %s\n", func_name);
			
			func_name += 4;
			
			if (!IggyPlayerCallbackResultPath)
			{
				HMODULE iggy = GetModuleHandle("iggy_w64.dll");
				
				IggyPlayerCallbackResultPath = (IggyPlayerCallbackResultPathType)GetProcAddress(iggy, "IggyPlayerCallbackResultPath");
				IggyValueSetStringUTF8RS = (IggyValueSetStringUTF8RSType)GetProcAddress(iggy, "IggyValueSetStringUTF8RS");
				IggyValueSetS32RS = (IggyValueSetS32RSType)GetProcAddress(iggy, "IggyValueSetS32RS");
			}
			
			if (strcmp(func_name, "ShouldExtendPauseMenu") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				IggyValueSetS32RS(ret, 0, 0, should_extend_pause_menu);			
				return 1;
			}
			
			else if (strcmp(func_name, "GetSlotsData") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				const std::string &slots = GetSlotsData();
				IggyValueSetStringUTF8RS(ret, nullptr, nullptr, slots.c_str(), slots.length());			
				return 1;
			}
			
			else if (strcmp(func_name, "IsBattleUIHidden") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				IggyValueSetS32RS(ret, 0, 0, IsBattleUIHidden());			
				return 1;
			}
			
			else if (strcmp(func_name, "ToggleBattleUI") == 0)
			{
				ToggleBattleUI();
				return 1;
			}
			
			else if (strcmp(func_name, "GetFirstAutoGenPortraitCharName") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				const std::string name = GetFirstAutoGenPortraitCharName();
				IggyValueSetStringUTF8RS(ret, nullptr, nullptr, name.c_str(), name.length());			
				return 1;
			}
			
			else if (strcmp(func_name, "CanDumpAutoGenPortrait") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				IggyValueSetS32RS(ret, 0, 0, CanDumpAutoGenPortrait());			
				return 1;
			}
			
			else if (strcmp(func_name, "DumpAutoGenPortrait") == 0)
			{
				DumpAutoGenPortrait();
				return 1;
			}
			
			else if (strcmp(func_name, "GetNumSsStages") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				IggyValueSetS32RS(ret, 0, 0, GetNumSsStages());			
				return 1;
			}
			
			else if (strcmp(func_name, "GetStagesSlotsData") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				const std::string &slots = GetStagesSlotsData();
				IggyValueSetStringUTF8RS(ret, nullptr, nullptr, slots.c_str(), slots.length());			
				return 1;
			}
			
			else if (strcmp(func_name, "GetLocalStagesSlotsData") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				const std::string &slots = GetLocalStagesSlotsData();
				IggyValueSetStringUTF8RS(ret, nullptr, nullptr, slots.c_str(), slots.length());			
				return 1;
			}
			
			else if (strstr(func_name, "GetStageName") == func_name)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				int ssid = 0;
				sscanf(func_name+strlen("GetStageName"), "%d", &ssid);
				
				const std::string name = GetStageName(ssid);
				
				IggyValueSetStringUTF8RS(ret, nullptr, nullptr, name.c_str(), name.length());			
				return 1;				
			}
			
			else if (strstr(func_name, "GetStageImageString") == func_name)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				int ssid = 0;
				sscanf(func_name+strlen("GetStageImageString"), "%d", &ssid);
				
				const std::string name = GetStageImageString(ssid);
				
				IggyValueSetStringUTF8RS(ret, nullptr, nullptr, name.c_str(), name.length());			
				return 1;				
			}
			
			else if (strstr(func_name, "GetStageIconString") == func_name)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				int ssid = 0;
				sscanf(func_name+strlen("GetStageIconString"), "%d", &ssid);
				
				const std::string name = GetStageIconString(ssid);
				
				IggyValueSetStringUTF8RS(ret, nullptr, nullptr, name.c_str(), name.length());			
				return 1;				
			}
			else if (strstr(func_name, "StageHasBeenSelectedBefore") == func_name)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				int ssid = 0;
				sscanf(func_name+strlen("StageHasBeenSelectedBefore"), "%d", &ssid);
				
				IggyValueSetS32RS(ret, 0, 0, StageHasBeenSelectedBefore(ssid));					
				return 1;				
			}
			
			else if (strcmp(func_name, "GetPlayerAllies") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				const std::string allies = GetPlayerAllies();
				IggyValueSetStringUTF8RS(ret, nullptr, nullptr, allies.c_str(), allies.length());			
				return 1;
			}
			
			else if (strstr(func_name, "GetMobName") == func_name)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				int idx = 0;
				sscanf(func_name+strlen("GetMobName"), "%d", &idx);
				
				const std::string name = GetMobName(idx);
				
				IggyValueSetStringUTF8RS(ret, nullptr, nullptr, name.c_str(), name.length());			
				return 1;	
			}
			else if (strstr(func_name, "TakeControlOfMob") == func_name)
			{
				int idx = 0;
				sscanf(func_name+strlen("TakeControlOfMob"), "%d", &idx);
				
				TakeControlOfMob(idx);
				return 1;
			}
			else if (strcmp(func_name, "IsToggleUIEnabled") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				IggyValueSetS32RS(ret, 0, 0, IsToggleUIEnabled());			
				return 1;
			}

			else if (strcmp(func_name, "HelloWorld") == 0)
			{
				void *ret = IggyPlayerCallbackResultPath(iggy_obj);
				if (!ret)
				{
					DPRINTF("IggyPlayerCallbackResultPath returned NULL\n");
					return 0;
				}
				
				static const char *hello_world = "Hello world from the native side";			
				IggyValueSetStringUTF8RS(ret, nullptr, nullptr, hello_world, strlen(hello_world));			
				return 1;
			}			
		}
	}
	
	return ExternalAS3Callback(custom_arg, iggy_obj, pfunc_name);
}

PUBLIC void SetupExternalAS3Callback(ExternalAS3CallbackType orig)
{
	ExternalAS3Callback = orig;
}

typedef void (* SendToAS3Type)(void *, int32_t, const void *);
SendToAS3Type SendToAS3;

PUBLIC void PatchGameVersionString(void *obj, int32_t code, const char16_t *version_string)
{
	static std::u16string version = version_string;	// THIS VAR MUST BE STATIC TO AVOID OBJECT DESTRUCTION AT THE END OF THE METHOD
	
	// Old code, commented
	/*
	// -_- I have to make the font smaller, otherwise it won't fit, that would require modifying the iggy.
	// For normal game versions like 1.08, 1.09, etc, use font size 11
	// For versions such as 1.09.1 use font size 10
	// For patcher longer versions, use font size = 8
	
	version = u"<font size=\"9\">v" + version.substr(4) + u" (patcher " XV2_PATCHER_VERSION ")</font>";	
	*/
	
	// New aproach since 1.16: remove the build id, and use that space.

	//DPRINTF("version %S\n", version.c_str());
	
	if (version.find(u"patcher") != std::string::npos)
	{
		SendToAS3(obj, code, version.c_str()); 
		return;
	}
	
	size_t pos = version.find(u" bid");
	if (pos != std::string::npos)
	{
		version = version.substr(0, pos);
	}
	
	version = version.substr(4) + u" (patcher " XV2_PATCHER_VERSION ")";
	//DPRINTF("version %S\n", version.c_str());
	
	SendToAS3(obj, code, version.c_str()); 
}

PUBLIC void SetupPatchGameVersionString(SendToAS3Type orig)
{
	SendToAS3 = orig;
}

PUBLIC void SetupBattleCoreMainSystem_singleton(void *address)
{
	pms_singleton = (Battle_Core_MainSystem **)GetAddrFromRel(address);
}

PUBLIC void SetupMobDtor(_Battle_Mob_Destructor orig)
{
	Battle_Mob_Destructor = orig;
}

PUBLIC void MobDtorPatched(Battle_Mob *pthis)
{
	OnDeletedMob_AI(pthis);	
	OnDeletedMob_Control(pthis);
	
	Battle_Mob_Destructor(pthis);
}

// TEST DELETE ME
/*
struct BattleEventMob_Hensin
{
	void *vtbl;
	void *unk_08;	// Initialized in ctor to third parameter. Unused in the transform?
	void *mob_object; // Apparently there is one of this for every character in the battle? Size is 0x2850, cms id is at 0xBC
	uint32_t unk_18;
	uint32_t unk_1C; // Seen value 1
	uint32_t unk_20; // Seen 0x41
	uint32_t unk_24;
	uint32_t unk_28; // Seen 0x65. Tranformation bcs?
	uint32_t unk_2C; // Seen 0x64
	uint32_t unk_30; // Seen value 0, checked against 0. Then on second call it is at 1
	uint32_t unk_34; // It could be pad
};

static_assert(offsetof(BattleEventMob_Hensin, unk_34) == 0x34, "Struct alignment error");
STATIC_ASSERT_STRUCT(BattleEventMob_Hensin, 0x38);

typedef uint32_t ( *AISpecialBehaviourType)(void *, void *, void *);
AISpecialBehaviourType AISpecialBehaviour;

typedef void * (* Battle_Mob_Ctor_Type)(void *, void *, uint32_t);
Battle_Mob_Ctor_Type Battle_Mob_Ctor;

typedef void (* BattleEventMob_Hensin_Type)(BattleEventMob_Hensin *);
BattleEventMob_Hensin_Type BattleEventMob_Hensin_Func;

static void *last_mob;

static void Transform()
{
	uint32_t *mob32 = (uint32_t *)last_mob;
	
	DPRINTF("Attempting transform on char with cms 0x%x.\n", mob32[0xBC/4]);
	BattleEventMob_Hensin hen;
	
	memset(&hen, 0, sizeof(hen));
	hen.mob_object = last_mob;
	hen.unk_1C = 1;
	hen.unk_20 = 0x41;
	hen.unk_28 = 0x64; // Bcs index
	hen.unk_2C = 0x64; // Base?
	hen.unk_30 = 0;
	BattleEventMob_Hensin_Func(&hen);	
	
	memset(&hen, 0, sizeof(hen));
	hen.mob_object = last_mob;
	hen.unk_1C = 1;
	hen.unk_20 = 0x41;
	hen.unk_28 = 0x64; // Bcs index
	hen.unk_2C = 0x64; // Base?
	hen.unk_30 = 1;
	BattleEventMob_Hensin_Func(&hen);
}

// a2+0x10 = MOB object owner 
PUBLIC uint32_t AISpecialBehaviourPatched(void *pthis, void *a1, uint32_t *a2)
{
	static int state = 0;
	static uint32_t used_skill = -1;
	
	if (state != 2)
	{
		bool doit = false;
		
		if (state == 0)
		{
			if (a2[0x154/4] == 2)
			{
				used_skill = a2[0x28/4];
				a2[0x154/4] = 0xFFFFFFFF;
				doit = true;
			}
		}
		else if (state == 1)
		{
			if (a2[0x28/4] == used_skill)
			{
				doit = true;
			}
			else
			{
				state = 0;
			}
		}
		
		if (doit)
		{		
			DPRINTF("Will skip usage of %d\n", a2[0x28/4]);
			state++;
			
			if (state == 2)
			{
				Transform();
			}
			
			return 0;
		}	
	}
	
	return AISpecialBehaviour(pthis, a1, a2);
}

PUBLIC void SetupAISpecialBehaviour(AISpecialBehaviourType orig)
{
	AISpecialBehaviour = orig;
}

PUBLIC void *Battle_Mob_Ctor_Patched(void *pthis, void *a1, uint32_t a2)
{
	last_mob = Battle_Mob_Ctor(pthis, a1, a2);
	return last_mob;
}

PUBLIC void Setup_Battle_Mob_Ctor(Battle_Mob_Ctor_Type orig)
{
	Battle_Mob_Ctor = orig;
}

PUBLIC void Setup_BattleEventMob_Hensin(BattleEventMob_Hensin_Type orig)
{
	BattleEventMob_Hensin_Func = orig;
}*/

/*struct StringObject
{	
	union
	{
		char short_str[16]; // For length2 < 16
		char *long_str; // For length2 >= 16		
	} str; 
	
	size_t length; 
	size_t length2; 
	
	StringObject()
	{
		str.long_str = nullptr;
		length = 0;
		length2 = 0;
	}
	
	inline char *CStr()
	{
		if (length2 >= 16)
		{
			return str.long_str;
		}
		
		return str.short_str;
	}
};
CHECK_STRUCT_SIZE(StringObject, 0x20);

typedef void (* TestFmType)(uint8_t *, uint32_t, uint32_t);
TestFmType TestFmOrig;

PUBLIC void TestFmSetup(TestFmType orig)
{
	TestFmOrig = orig;
}

PUBLIC void TestFm(uint8_t *pthis, uint32_t a2, uint32_t a3)
{
	uint8_t *str = pthis + 0x28;
	StringObject *sstr = (StringObject *)str;
	
	DPRINTF("TESTFM: %s 0x%x 0x%x 0x%x\n", sstr->CStr(), a2, a3, *(uint32_t *)&pthis[0x20]);
	
	//if (strcmp(sstr->CStr(), "NLBYSKY00") != 0)
	//{
	//	DPRINTF("Changing it\n");
		//a2 = 1; a3 = 0;	
	//	static int tick = 0;
		
	//	if (tick == 0)
	//		tick = GetTickCount();
		
	//	if ((GetTickCount() - tick) >= 60000)		
	//		return;
	//}
	
	TestFmOrig(pthis, a2, a3);
}*/

} // extern C

static bool load_dll(bool critical)
{
	static const std::vector<const char *> exports =
	{
#ifdef DINPUT
		"DirectInput8Create",
		"DllCanUnloadNow",
		"DllGetClassObject",
		"DllRegisterServer",
		"DllUnregisterServer"
#else
		"XInputGetState",
		"XInputSetState",
		"XInputGetBatteryInformation",
		"XInputEnable",
		"XInputGetCapabilities",
		"XInputGetDSoundAudioDeviceGuids",
		"XInputGetKeystroke",
		(char *)100,
		(char *)101,
		(char *)102,
		(char *)103
#endif
	};	
	
	static char mod_path[2048];
	static char original_path[256];	
	static bool loaded = false;
	
	MutexLocker lock(&mutex);
	
	if (loaded)
		return true;
	
#ifdef DINPUT
	myself = GetModuleHandle("dinput8.dll");
#else
	myself = GetModuleHandle("xinput1_3.dll");
#endif

	GetModuleFileNameA(myself, mod_path, sizeof(mod_path));
	
	myself_path = Utils::NormalizePath(mod_path);
	myself_path = myself_path.substr(0, myself_path.rfind('/')+1);	
	DPRINTF("Myself path = %s\n", myself_path.c_str());
	
#ifndef DINPUT	
	if (Utils::FileExists(myself_path+"xinput_other.dll"))
	{
		strncpy(original_path, myself_path.c_str(), sizeof(original_path));
		strncat(original_path, "xinput_other.dll", sizeof(original_path));
	}
	else
#endif
	{	
		if (GetSystemDirectory(original_path, sizeof(original_path)) == 0)
			return false;

#ifdef DINPUT
		strncat(original_path, "\\dinput8.dll", sizeof(original_path));
#else
		strncat(original_path, "\\xinput1_3.dll", sizeof(original_path));
#endif
	}
	
	patched_dll = LoadLibrary(original_path);		
	if (!patched_dll)
	{
		if (critical)
			UPRINTF("Cannot load original DLL (%s).\nLoadLibrary failed with error %u\n", original_path, (unsigned int)GetLastError());
				
		return false;
	}
	
	DPRINTF("original DLL path: %s   base=%p\n", original_path, patched_dll);
		
	for (auto &export_name : exports)
	{
		uint64_t ordinal = (uint64_t)export_name;
		
		uint8_t *orig_func = (uint8_t *)GetProcAddress(patched_dll, export_name);
		
		if (!orig_func)
		{
			if (ordinal < 0x1000)			
			{
				DPRINTF("Warning: ordinal function %I64d doesn't exist in this system.\n", ordinal);
				continue;		
			}
			else
			{
				if (Utils::IsWine())
				{
					DPRINTF("Failed to get original function: %s --- ignoring error because running under wine.\n", export_name);
					continue;
				}
				else
				{
					UPRINTF("Failed to get original function: %s\n", export_name);			
					return false;
				}
			}
		}
		
		uint8_t *my_func = (uint8_t *)GetProcAddress(myself, export_name);		
		
		if (!my_func)
		{
			if (ordinal < 0x1000)			
				UPRINTF("Failed to get my function: %I64d\n", ordinal);
			else
				UPRINTF("Failed to get my function: %s\n", export_name);
			
			return false;
		}
		
		if (ordinal < 0x1000)
			DPRINTF("%I64d: address of microsoft: %p, address of mine: %p\n", ordinal, orig_func, my_func);
		else
			DPRINTF("%s: address of microsoft: %p, address of mine: %p\n", export_name, orig_func, my_func);
		
		DPRINTF("Content of microsoft func: %02X%02X%02X%02X%02X\n", orig_func[0], orig_func[1], orig_func[2], orig_func[3], orig_func[4]);
		DPRINTF("Content of my func: %02X%02X%02X%02X%02X\n", my_func[0], my_func[1], my_func[2], my_func[3], my_func[4]);
		
		PatchUtils::Hook(my_func, nullptr, orig_func);
		DPRINTF("Content of my func after patch: %02X%02X%02X%02X%02X\n", my_func[0], my_func[1], my_func[2], my_func[3], my_func[4]);
	}
	
	loaded = true;
	return true;
}

static void unload_dll()
{
	if (patched_dll)
	{
		FreeLibrary((HMODULE)patched_dll);
		patched_dll = nullptr;
	}
}

static bool load_patch_file(const std::string &path, bool is_directory, void *custom_param)
{
	if (!Utils::EndsWith(path, ".xml", false))
		return true;
	
	int num_fails = 0;

#ifdef DINPUT
	EPatchFile epf(myself_path+"dinput8.dll");
#else
	EPatchFile epf(myself_path+"xinput1_3.dll");
#endif
	
	int enabled;
	bool enabled_b;
	std::string setting;
	
	if (!epf.CompileFromFile(path))
	{
		UPRINTF("Failed to compile file \"%s\"\n", path.c_str());
		exit(-1);
	}
	
	typedef bool (* ConditionalFunction)();
	
	//DPRINTF("File %s compiled.\n", path.c_str());		
	if ((enabled = epf.GetEnabled(setting)) < 0)
	{	
		if (Utils::EndsWith(setting, "()"))
		{
			const std::string funcion_name = setting.substr(0, setting.rfind("()"));
			ConditionalFunction condition = (ConditionalFunction) GetProcAddress(myself, funcion_name.c_str());
			
			if (condition && condition())
			{
				enabled = true;
			}
			else
			{
				enabled = false;
			}
		}
		else
		{
			ini.GetBooleanValue("Patches", setting, &enabled_b, false);
			enabled = enabled_b;
		}
	}
	
	if (!enabled)
		return true;
	
	for (EPatch &patch : epf)
	{
		if ((enabled = patch.GetEnabled(setting)) < 0)
		{
			if (Utils::EndsWith(setting, "()"))
			{
				const std::string funcion_name = setting.substr(0, setting.rfind("()"));
				ConditionalFunction condition = (ConditionalFunction) GetProcAddress(myself, funcion_name.c_str());
				
				if (condition && condition())
				{
					enabled = true;
				}
				else
				{
					enabled = false;
				}
			}				
			else
			{
				ini.GetBooleanValue("Patches", setting, &enabled_b, false);
				enabled = enabled_b;
			}
		}
		
		if (!enabled)
			continue;
		
		if (!patch.Apply())
		{
			if (count_patches_failures)
			{
				DPRINTF("Failed to apply patch \"%s:%s\"\n", epf.GetName().c_str(), patch.GetName().c_str());
				num_fails++;
			}
			else
			{
				UPRINTF("Failed to apply patch \"%s:%s\"\n", epf.GetName().c_str(), patch.GetName().c_str());
				exit(-1);
			}
		}
	}

	if (count_patches_failures && num_fails > 0)
	{
		num_patches_fail += num_fails;
		DPRINTF("%s: %d/%Id patches failed.\n", epf.GetName().c_str(), num_fails, epf.GetNumPatches());
	}
	
	return true;
}

static void load_patches()
{
	ini.GetBooleanValue("Debug", "count_patches_failures", &count_patches_failures, false);	
	num_patches_fail = 0;
	
	Utils::VisitDirectory(myself_path+PATCHES_PATH, true, false, false, load_patch_file);
	
	if (count_patches_failures && num_patches_fail > 0)
	{
		UPRINTF("The following number of patches failed:%d\n", num_patches_fail);
		exit(-1);
	}
}

static void qsf_crash_fix()
{
	// This is a fix for a crash happening in 1.17.2+ if there is an older QuestSort.qsf
	const std::string path = myself_path + CONTENT_ROOT + "data/system/QuestSort.qsf";
	if (!Utils::FileExists(path))
		return;
	
	QsfFile qsf;
	if (!qsf.LoadFromFile(path, false))
		return;
	
	if (qsf.GetNumEntries() != 14)
		return; // Fix not needed
	
	QsfQuestEntry qentry;
	QsfEntry entry;
	
	qentry.quests.push_back("CTP_18_00");
	qentry.quests.push_back("CTP_18_10");
	entry.type = "TPQ_FM";
	entry.quest_entries.push_back(qentry);
	
	qsf.AddEntry(entry);
	
	DPRINTF("*****Doing 1.17.2 QSF crash fix. A backup will be saved as QuestSort.qsf.xv2pbak\n");
	Utils::DoCopyFile(path, path + ".xv2pbak");
	
	if (qsf.SaveToFile(path))
	{
		DPRINTF("QSF crash fix success.\n");
	}
	else
	{
		DPRINTF("QSF crash fix failed on saving.\n");
	}
}

static bool idb_check_visitor(const std::string &path, bool, void *)
{
	if (Utils::EndsWith(path, ".idb"))
	{
		size_t size = Utils::GetFileSize(path);
		if (size != (size_t)-1 && size > sizeof(IDBHeader))
		{
			size -= sizeof(IDBHeader);
			
			if ((size % sizeof(IDBEntryNew)) != 0)
			{
				std::string partial_path = path.substr(std::string(myself_path + CONTENT_ROOT).length());
				std::string msg = "The file \"" + partial_path + "\" is not compatible with this version of the game (idb format changed in 1.18).\n\nAbort load? If you press no, a crash or undefined behavior can happen in game.";
				
				if (MessageBoxA(NULL, msg.c_str(), "xv2patcher", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				{
					exit(0);
				}
				
				return false;
			}
		}
	}
	
	return true;
}

static void idb_check()
{
	Utils::VisitDirectory(myself_path + CONTENT_ROOT + "data/system/item/", true, false, false, idb_check_visitor);
}

static void start()
{
#ifdef DINPUT
	const char *my_dll_name = "dinput8.dll";
#else
	const char *my_dll_name = "xinput1_3.dll";
#endif
	
	DPRINTF("XV2PATCHER VERSION " XV2_PATCHER_VERSION ". Exe base = %p. My Dll base = %p. My dll name: %s\n", GetModuleHandle(NULL), GetModuleHandle(my_dll_name), my_dll_name);	
	
	float version = Utils::GetExeVersion(myself_path+EXE_PATH);
	DPRINTF("Running on game version %.3f\n", version);
	
	if (version != 0.0 && version < (MINIMUM_GAME_VERSION - 0.00001))
	{
		UPRINTF("This game version (%.3f) is not compatible with this version of the patcher.\nMin version required is: %.3f\n", version, MINIMUM_GAME_VERSION);
		exit(-1);
	}
	
	/*
	 This was used in version 1.11-1.13 because those tards forgot to update the version in the exe data.
	if (version == 1.10f)
	{
		uint8_t *ptr = (uint8_t *)GetModuleHandle(nullptr) + 0x6F7080;
		if (*ptr != 0x48)
		{
			UPRINTF("This version of the patcher doesn't support this version of the game.\n");
			exit(-1);
		}
	}*/
	
	load_patches();	
	qsf_crash_fix();
	idb_check();
}

static void IggySetTraceCallbackUTF8Patched(void *, void *param)
{
	HMODULE iggy = GetModuleHandle("iggy_w64.dll");
	if (!iggy)
		return;
	
	IGGYSetTraceCallbackType func = (IGGYSetTraceCallbackType)GetProcAddress(iggy, "IggySetTraceCallbackUTF8");
	
	if (func)
		func((void *)iggy_trace_callback, param);
}

static void IggySetWarningCallbackPatched(void *, void *param)
{
	HMODULE iggy = GetModuleHandle("iggy_w64.dll");
	if (!iggy)
		return;
	
	IGGYSetWarningCallbackType func = (IGGYSetWarningCallbackType)GetProcAddress(iggy, "IggySetWarningCallback");
	
	if (func)
		func((void *)iggy_warning_callback, param);
}

static BOOL InternetGetConnectedStatePatched(LPDWORD lpdwFlags, DWORD dwReserved)
{
	//DPRINTF("InternetGetConnectedState called from %p\n", BRA(0));
	
	*lpdwFlags = INTERNET_CONNECTION_OFFLINE;
	return FALSE;
}

extern "C" PUBLIC void BattleTimer(float *time)
{
	float new_time[1];
	
	ini.GetFloatValue("BattleTimer", "time", &new_time[0], 180.0f);
	DPRINTF("Battle timer set to %f seconds.  (1.#INF00 is infinity)\n", new_time[0]);
	
	PatchUtils::Write32(time, *(uint32_t *)&new_time);
}

extern "C" PUBLIC void ModifyGDrawSetting2A(uint32_t *value)
{
	int new_val;
	
	ini.GetIntegerValue("GDraw_Limits", "gdraw_setting_2a", &new_val, 1536);
	
	DPRINTF("GDraw setting 2A changed to %d\n", new_val);
	PatchUtils::Write32(value, new_val);
}

std::vector<std::string> ignore_modules;

static LONG CALLBACK ExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	void *fault_addr;
	void *fault_addr_rel = nullptr;	
	HMODULE fault_mod;
	std::string fault_mod_name;
	char path[MAX_PATH];
	
	fault_addr = (void *)ExceptionInfo->ExceptionRecord->ExceptionAddress;	
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)fault_addr, &fault_mod);	
	
	if (fault_mod)
	{
		fault_addr_rel = (void *)((uintptr_t)fault_addr - (uintptr_t)fault_mod);
		GetModuleFileNameA(fault_mod, path, MAX_PATH);
		fault_mod_name = path;
		
		std::string fn = Utils::GetFileNameString(fault_mod_name);
		
		for (const std::string &mod : ignore_modules)
		{
			if (Utils::ToLowerCase(mod) == Utils::ToLowerCase(fn))
				return EXCEPTION_CONTINUE_SEARCH;
		}
		
	}
	
	DPRINTF("*************************** EXCEPTION CAUGHT ***************************\n");
	
	if (fault_mod)
	{
		DPRINTF("Exception code=0x%x at address %p (%p) Faulting module = %s\n", (unsigned int)ExceptionInfo->ExceptionRecord->ExceptionCode, fault_addr, fault_addr_rel, fault_mod_name.c_str());
	}
	else
	{
		DPRINTF("Exception code=0x%x at address %p\n", (unsigned int)ExceptionInfo->ExceptionRecord->ExceptionCode, fault_addr);
	}

	DPRINTF("RIP = %p, RSP = %p, RAX = %p, RBX = %p\n", (void *)ExceptionInfo->ContextRecord->Rip, (void *)ExceptionInfo->ContextRecord->Rsp, 
														(void *)ExceptionInfo->ContextRecord->Rax, (void *)ExceptionInfo->ContextRecord->Rbx);
														
	DPRINTF("RCX = %p, RDX = %p, R8 = %p,  R9 = %p\n", 	(void *)ExceptionInfo->ContextRecord->Rcx, (void *)ExceptionInfo->ContextRecord->Rdx, 
														(void *)ExceptionInfo->ContextRecord->R8,  (void *)ExceptionInfo->ContextRecord->R9);
														
	DPRINTF("R10 = %p, R11 = %p, R12 = %p, R13 = %p\n", (void *)ExceptionInfo->ContextRecord->R10, (void *)ExceptionInfo->ContextRecord->R11, 
														(void *)ExceptionInfo->ContextRecord->R12, (void *)ExceptionInfo->ContextRecord->R13);
														
	DPRINTF("R14 = %p, R15 = %p, RDI = %p, RSI = %p\n", (void *)ExceptionInfo->ContextRecord->R14, (void *)ExceptionInfo->ContextRecord->R15, 
														(void *)ExceptionInfo->ContextRecord->Rdi, (void *)ExceptionInfo->ContextRecord->Rsi);
														
	DPRINTF("************************************************************************\n");
	
	return EXCEPTION_CONTINUE_SEARCH;
}

VOID WINAPI GetStartupInfoW_Patched(LPSTARTUPINFOW lpStartupInfo)
{
	static bool started = false;
	
	// This function is only called once by the game but... just in case
	if (!started)
	{	
		int default_log_level;
		
		if (!load_dll(true))
			exit(-1);
		
		ini.LoadFromFile(myself_path+INI_PATH);
		ini.GetIntegerValue("General", "default_log_level", &default_log_level, 1);
		ini.AddIntegerConstant("auto", 0);
		set_debug_level(default_log_level);				
		
		bool iggy_trace, iggy_warning, offline_mode, exception_handler;
		
		start();
		started = true;
		
		ini.GetBooleanValue("Patches", "iggy_trace", &iggy_trace, false);
		ini.GetBooleanValue("Patches", "iggy_warning", &iggy_warning, false);
		ini.GetBooleanValue("Patches", "offline_mode", &offline_mode, false);
		ini.GetBooleanValue("Debug", "exception_handler", &exception_handler, false);
		
		if (iggy_trace)
		{
			if (!PatchUtils::HookImport("iggy_w64.dll", "IggySetTraceCallbackUTF8", (void *)IggySetTraceCallbackUTF8Patched))
			{
				DPRINTF("Failed to hook import of IggySetTraceCallbackUTF8.\n");						
			}
		}
		
		if (iggy_warning)
		{
			if (!PatchUtils::HookImport("iggy_w64.dll", "IggySetWarningCallback", (void *)IggySetWarningCallbackPatched))
			{
				DPRINTF("Failed to hook import of IggySetWarningCallback.\n");						
			}
		}
		
		if (offline_mode)
		{
			if (!PatchUtils::HookImport("WININET.DLL", "InternetGetConnectedState", (void *)InternetGetConnectedStatePatched))
			{
				DPRINTF("Failed to hook import of InternetGetConnectedState");
			}
		}
		
		if (exception_handler)
		{
			ini.GetMultipleStringsValues("Debug", "exception_ignore_modules", ignore_modules);			
			AddVectoredExceptionHandler(1, ExceptionHandler);
		}
	}
	
	GetStartupInfoW(lpStartupInfo);
}

DWORD WINAPI StartThread(LPVOID)
{
	load_dll(false);
	return 0;
}

extern "C" BOOL WINAPI EXPORT DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		
			static bool done = false;
			
			if (done)
				return TRUE;
			
			initial_tick = GetTickCount();
			set_debug_level(1); 	

			if (in_game_process())
			{					
				done = true;
				
				CreateMutexA(nullptr, FALSE, "XV2PATCHER_INSTANCE");
				if (GetLastError() == ERROR_ALREADY_EXISTS)
				{
					UPRINTF("An instance of xv2patcher already exists.\n"
							"You probably have mixed xinput1_3 and dinput8!\n");
					exit(-1);
				}
				
				load_dll(false);

				if (!PatchUtils::HookImport("KERNEL32.dll", "GetStartupInfoW", (void *)GetStartupInfoW_Patched))
				{
					UPRINTF("GetStartupInfoW hook failed.\n");
					return TRUE;
				}
				
				//DWORD id;
				//CreateThread(NULL, 0, StartThread, NULL, 0, &id);
			}			
			
		break;
		
		case DLL_PROCESS_DETACH:		
			
			if (!lpvReserved)
				unload_dll();
			
		break;
	}
	
	return TRUE;
}
