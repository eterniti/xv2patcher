#include <windows.h>
#include "PatchUtils.h"
#include "UtfLowLevel.h"
#include "xv2patcher.h"
#include "debug.h"

typedef uint64_t (* LoadTableType)(void *, void *, uint32_t);
typedef int (* CpkGetFileInfoType)(uint8_t *, const char *, uint8_t *, uint32_t *);
typedef int (* CpkMovieFileType)(void *, void *, const char *);

extern "C" 
{
	
static LoadTableType orig_LoadTable;
static CpkGetFileInfoType orig_CpkGetFileInfo;
static CpkMovieFileType orig_CpkMovieFile;

static bool log_all_files;
static bool log_loose_files;

PUBLIC void LoadToc_Setup(LoadTableType orig)
{
	orig_LoadTable = orig;
}

PUBLIC uint64_t LoadToc(void *unk, void *buf, uint32_t size)
{
	UtfFile utf;
	size_t num_files;
	uint32_t num_removed = 0;	
	
	if (!utf.Load((const uint8_t *)buf, size))
	{
		DPRINTF("%s: UtfFile::Load failed!\n", FUNCNAME);
		return orig_LoadTable(unk, buf, size);
	}
	
	if (strcmp(utf.GetTableName(), "CpkTocInfo") != 0)
	{
		DPRINTF("Warning: TOC name is not expected: \"%s\"\n", utf.GetTableName());
	}
	
	num_files = utf.GetNumRows();
	
	for (size_t i = 0; i < num_files; i++)
	{
		char *dir_name = (char *)utf.GetRawData("DirName", i);
		char *file_name = (char *)utf.GetRawData("FileName", i);
		std::string path;
		
		if (!file_name)
			continue;
		
		if (dir_name)
		{
			path = std::string(dir_name) + std::string("/") + std::string(file_name);			
		}
		else
		{
			path = file_name;
		}
		
		path = myself_path + CONTENT_ROOT + path;	
		
		if (Utils::FileExists(path))
		{
			uint32_t pos;
			uint8_t *table = utf.GetPtrTable();
			
			if (dir_name)
			{
				pos = utf.GetPosition("DirName", i);
			}
			else
			{
				pos = utf.GetPosition("FileName", i);				
			}
			
			if (pos == 0 || pos == (uint32_t)-1)
			{
				UPRINTF("%s: Fatal, cannot get position of DirName or FileName, in row 0x%x.\n", FUNCNAME, (uint32_t)i);
				exit(-1);
			}			
			
			*(uint32_t *)(table + pos) = 0;
			num_removed++;
		}
	}
	
	DPRINTF("%d files were removed in ram for cpk with toc_size=0x%x\n", num_removed, size);	
	return orig_LoadTable(unk, buf, size);
}

PUBLIC void CpkGetFileInfo_Setup(CpkGetFileInfoType orig)
{
	orig_CpkGetFileInfo = orig;
	
	ini.GetBooleanValue("LooseFiles", "log_all_files", &log_all_files, false);
	ini.GetBooleanValue("LooseFiles", "log_loose_files", &log_loose_files, false);
}

PUBLIC int CpkGetFileInfo_Patched(uint8_t *cpk_object, const char *path, uint8_t *ret_entry, uint32_t *result)
{
	//DPRINTF("RA = %p\n", (void *)PatchUtils::RelAddress(BRA(0)));
	//PrintStackTrace(3);
	int ret = orig_CpkGetFileInfo(cpk_object, path, ret_entry, result);	
	
	if (!*result)
	{
		if (cpk_object[0x28] != 2)
		{
			DPRINTF("Conditions are not met for file %s\n", path);
			return ret;
		}
		
		size_t file_size = Utils::GetFileSize(myself_path+CONTENT_ROOT+path);		
		if (file_size != (size_t)-1)
		{
			if (log_all_files || log_loose_files)
			{
				DPRINTF("CpkGetfileInfo: %s. This file is loaded from cpk -> no. Size=%d\n", path, (uint32_t)file_size);
			}
			
			*(uint32_t *)&ret_entry[0x20] = (uint32_t)file_size;
			*(uint64_t *)&ret_entry[0x18] = (uint64_t)file_size; // Uncomp size
			*result = true;
			return 0;
		}

		if (log_all_files || log_loose_files)
		{
			DPRINTF("CpkGetFileInfo: %s, File is neither in cpk nor in filesystem.\n", path);
		}
	}
	else
	{
		if (log_all_files)
		{
			DPRINTF("CpkGetfileInfo: %s. This file is loaded from cpk -> yes. Size=%d, Compressed size=%d\n", path, *(uint32_t *)&ret_entry[0x20], *(uint32_t *)&ret_entry[0x18]);
		}
	}
	
	return ret;
}

PUBLIC void SetupLogMovieFiles(CpkMovieFileType orig)
{
	orig_CpkMovieFile = orig;
}

PUBLIC int HookMovieLoad(void *pthis, void *unk, const char *path)
{
	int ret = orig_CpkMovieFile(pthis, unk, path);
	
	DPRINTF("Movie load: %s %p %d\n", path, unk, ret);
	return ret;
}

} // Extern "C"