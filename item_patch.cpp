#include "xv2patcher.h"
#include "MsgFile.h"
#include "IdbFile.h"
#include "Xv2StageDefFile.h"

#define GAME_CAC_COSTUME_DESCS_PATH  "data/msg/proper_noun_costume_info_"

#define GAME_TOP_IDB_PATH "data/system/item/costume_top_item.idb"
#define GAME_BOTTOM_IDB_PATH "data/system/item/costume_bottom_item.idb"
#define GAME_GLOVES_IDB_PATH "data/system/item/costume_gloves_item.idb"
#define GAME_SHOES_IDB_PATH "data/system/item/costume_shoes_item.idb"

enum ItemType
{
	ITEM_TOP,
	ITEM_BOTTOM,
	ITEM_GLOVES,
	ITEM_SHOES,
	ITEM_ACCESSORY,
	ITEM_SUPERSOUL,
	ITEM_MATERIAL,
	ITEM_EXTRA,
	ITEM_BATTLE,
	ITEM_QQBANG
};

typedef const char16_t *(* GetItemInfoType)(void *, int, int);

static MsgFile item_msg[1];
static IdbFile item_idb[4];
static bool idb_loaded[4];
static bool msg_loaded[1];

static const char16_t *unknown_item_info;
static int *game_language;
static GetItemInfoType GetItemInfo;

extern "C"
{
	
PUBLIC void OnUnknownItemAddressLocated(void *addr)
{
	unknown_item_info = (const char16_t *)GetAddrFromRel(addr);
}

PUBLIC void OnGameLanguageLocated_Item(void *addr)
{
	game_language = (int *)GetAddrFromRel(addr, 4);
}	

PUBLIC void SetupGetItemInfo(GetItemInfoType orig)
{
	GetItemInfo = orig;
}

PUBLIC const char16_t *GetItemInfoPatched(void *pthis, int type, int item_id)
{
	const char16_t *result = GetItemInfo(pthis, type, item_id);
	
	// Pointer comparison instead of string comparison, for efficiency reasons
	if (result == unknown_item_info && type >= ITEM_TOP && type <= ITEM_SHOES)
	{
		int msg_index = 0;
		
		if (type <= ITEM_SHOES)
		{
			msg_index = 0;
		}
		
		if (!idb_loaded[type])
		{
			std::string idb_path = myself_path + CONTENT_ROOT;
			
			if (type == ITEM_TOP)
				idb_path += GAME_TOP_IDB_PATH;
			else if (type == ITEM_BOTTOM)
				idb_path += GAME_BOTTOM_IDB_PATH;
			else if (type == ITEM_GLOVES)
				idb_path += GAME_GLOVES_IDB_PATH;
			else if (type == ITEM_SHOES)
				idb_path += GAME_SHOES_IDB_PATH;
			
			idb_loaded[type] = true;
			item_idb[type].LoadFromFile(idb_path, false);
		}
		
		if (!msg_loaded[msg_index])
		{
			std::string msg_path = myself_path + CONTENT_ROOT;
			
			if (msg_index == 0)
				msg_path += GAME_CAC_COSTUME_DESCS_PATH;
			
			if (*game_language < 0 || *game_language >= (int)xv2_native_lang_codes.size())
				return result;
			
			msg_path += xv2_native_lang_codes[*game_language];
			msg_path += ".msg";
			
			msg_loaded[msg_index] = true;
			item_msg[msg_index].SetU16Mode(true);
			item_msg[msg_index].LoadFromFile(msg_path, false);
		}
		
		if (item_idb[type].GetNumEntries() > 0 && item_msg[msg_index].GetNumEntries() > 0)
		{
			IdbEntry *idb = item_idb[type].FindEntryByID(item_id);
			
			if (idb && idb->desc_id < item_msg[msg_index].GetNumEntries())
			{
				const MsgEntry &msg = (item_msg[msg_index])[idb->desc_id];
				
				if (msg.u16_lines.size() > 0)
					result = msg.u16_lines[0].c_str();
			}
		}
	}
	
	return result;
}
	
} // extern "C"

