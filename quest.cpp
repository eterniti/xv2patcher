#include <unordered_set>

#include "xv2patcher.h"
#include "debug.h"
#include "PatchUtils.h"
#include "QedFile.h"

static const std::unordered_set<std::string> vanilla_hlq_quests =
{
	"HLQ_0100",
	"HLQ_0200",
	"HLQ_0300",
	"HLQ_0400",
	"HLQ_0500",
	"HLQ_0600",
	"HLQ_0700",
	"HLQ_0800",
	"HLQ_1100",
	"HLQ_1200",
	"HLQ_1400",
	"HLQ_1500",
	"HLQ_1800",
	"HLQ_2000",
	"HLQ_2100",
	"HLQ_2600",
	"HLQ_2700",
	"HLQ_2800",
	"HLQ_2900",
	"HLQ_3000"
};

extern "C"
{
	
// This is a single quest status call hooked, the one used to include the quest in the global orb or not.
int GetQuestStatusPatched(uint8_t *pthis)
{
	const char *quest_name = (const char *)pthis+0x20;
	
	if (quest_name && memcmp(quest_name, "HLQ_", 4) == 0)
	{
		if (vanilla_hlq_quests.find(quest_name) == vanilla_hlq_quests.end())			
			return 3;
	}
	
	typedef int (* GetQuestStatusType)(void *);
	uint64_t *vtable = (uint64_t *) *(uint64_t *)pthis;
	GetQuestStatusType GetQuestStatus = (GetQuestStatusType) vtable[0x18/8];
	
	return GetQuestStatus(pthis);
}


PUBLIC void OnModHlqPatchLocated(uint8_t *addr)
{
	// We have an storage of 9 bytes to: move the "mov rcx, rbx", and then write a regular call to our code
	// One instruction is destroyed, but that instruction is replicated by the hooked function
	
	PatchUtils::Write32(addr, 0xCB8B4890); // Nop + mov rcx, rbx
	PatchUtils::Write8(addr+4, 0xE8); // To make PatchUtils::HookCall happy :)
	PatchUtils::HookCall(addr+4, nullptr, (void *)GetQuestStatusPatched);
}

typedef int (* QedCondType)(void *, void *);
typedef void *(* QmlIdToEventCastType)(int32_t);

static QedCondType QedCond;
static QmlIdToEventCastType QmlIdToEventCast;

PUBLIC int QedCondHooked(uint64_t *pthis, uint32_t *param2)
{
	int ret = QedCond(pthis, param2);
	
	uint32_t opcode = *param2;
	
	if (opcode >= QED_COND_EXT_IS_AVATAR && opcode < QED_COND_EXT_LIMIT)
	{
		int32_t *params_int = (int32_t *)(param2[4/4] + pthis[0x10/8]);
		
		switch (opcode)
		{
			case QED_COND_EXT_IS_AVATAR:
				
				uint32_t *event_cast = (uint32_t *)QmlIdToEventCast(params_int[0]);
				if (!event_cast)
					return 0;
				
				uint32_t cms_id = event_cast[0x4C/4]; 
				
				return (cms_id >= 0x64 && cms_id <= 0x6D);
				
			break;
		}
		
		return 0;
	}
	
	return ret;
}

PUBLIC void SetupQedCond(QedCondType orig)
{
	QedCond = orig;
}

PUBLIC void OnQmlIdToEventCastLocated(QmlIdToEventCastType orig)
{
	QmlIdToEventCast = orig;
}
	
} // extern "C"