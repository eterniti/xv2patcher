#ifndef XV2PATCHER
#define XV2PATCHER

#include "IniFile.h"
#include "PatchUtils.h"
#include "game_defs.h"

#define EXPORT 	__declspec(dllexport)
#define PUBLIC	EXPORT

#define PROCESS_NAME "dbxv2.exe"

#define INI_PATH		"../XV2PATCHER/xv2patcher.ini"
#define PATCHES_PATH	"../XV2PATCHER/EPatches"
#define EXE_PATH	"DBXV2.exe"

#define CONTENT_ROOT	"../"

#define MINIMUM_GAME_VERSION	1.21f

#define SLOTS_FILE         		"data/XV2P_SLOTS.x2s"
#define SLOTS_FILE_STAGE		"data/XV2P_SLOTS_STAGE.x2s"
#define SLOTS_FILE_STAGE_LOCAL	"data/XV2P_SLOTS_STAGE_LOCAL.x2s"

#define XV2_PATCHER_VERSION	"4.20"

typedef void (* IGGYSetTraceCallbackType)(void *callback, void *param);
typedef void (* IGGYSetWarningCallbackType)(void *callback, void *param);

extern std::string myself_path;
extern IniFile ini;

extern Battle_Core_MainSystem **pms_singleton;

extern void iggy_trace_callback(void *param, void *unk, const char *str, size_t len);
extern void iggy_warning_callback(void *param, void *unk, uint32_t len, const char *str);

static inline void *GetAddrFromRel(void *addr, uint32_t offset=0)
{
	uint8_t *addr_8 = (uint8_t *)addr;
	return (void *)(addr_8+4+offset + *(int32_t *)addr);
}

static inline void SetRelAddr(void *addr, void *final_addr)
{
	PatchUtils::Write32(addr, Utils::DifPointer(final_addr, addr) - 4);
}

static inline size_t RelAddress(void *addr)
{
	return (size_t)addr - (size_t)GetModuleHandle(nullptr);
}


#endif // XV2PATCHER
