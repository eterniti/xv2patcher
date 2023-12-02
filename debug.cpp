#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

#include "xv2patcher.h"
#include "debug.h"

#define DEBUG_BUFFER_SIZE_EXTRA	1024
#define DEBUG_FILE	"../XV2PATCHER/xv2_log.txt"

static int debug_level;
static FILE *file;

extern DWORD initial_tick;

static void local_tick(uint32_t *H, uint32_t *MM, uint32_t *SS, uint32_t *mmm)
{
	uint32_t T =  (uint32_t)(GetTickCount() - initial_tick);
	uint32_t seconds_total = T / 1000;
	uint32_t minutes_total = T / (1000*60);
	
	*mmm = T % 1000;
	*SS = seconds_total % 60;
	*MM = minutes_total % 60;
	*H = T / (1000*60*60);
}

int set_debug_level(int level)
{
	int ret = debug_level;
	debug_level = level;
	return ret;
}

int mod_debug_level(int mod_by)
{
	return set_debug_level(debug_level+mod_by);
}

int __attribute__ ((format (printf, 1, 2))) DebugPrintf(const char* fmt, ...) 
{
	if (debug_level == 0)
		return 0;
	
	char *dbg;
	va_list ap;
	
	size_t max_size = strlen(fmt)+DEBUG_BUFFER_SIZE_EXTRA;

	dbg = (char *)malloc(max_size);
	if (!dbg)
		return 0;
		
	va_start(ap, fmt);
	size_t len = vsnprintf(dbg, max_size, fmt, ap);
	va_end(ap);
	
	if (debug_level == 1)
	{
		OutputDebugStringA(dbg);
	}
	else if (debug_level == 2)
	{
		if (!file)
		{
			std::string file_path = myself_path + DEBUG_FILE;		
			file = fopen(file_path.c_str(), "w");
			if (!file)
				return 0;
		}

		//fputs(dbg, file);
		uint32_t H, MM, SS, mmm;
	
		local_tick(&H, &MM, &SS, &mmm);
		fprintf(file, "%d:%02d:%02d.%03d: %s", H, MM, SS, mmm, dbg);
		fflush(file);
	}
	else
	{
		MessageBoxA(NULL, dbg, "XV2 Patcher", 0);
	}
	
	free(dbg);
	return len;
}

int __attribute__ ((format (printf, 1, 2))) UserPrintf(const char* fmt, ...) 
{
	char *dbg;
	va_list ap;
	
	size_t max_size = strlen(fmt)+DEBUG_BUFFER_SIZE_EXTRA;

	dbg = (char *)malloc(max_size);
	if (!dbg)
		return 0;
		
	va_start(ap, fmt);
	size_t len = vsnprintf(dbg, max_size, fmt, ap);
	va_end(ap);

	MessageBoxA(NULL, dbg, "XV2 Patcher", 0);
	free(dbg);
	return len;
}

int __attribute__ ((format (printf, 1, 2))) FilePrintf(const char* fmt, ...) 
{
	char *dbg;
	va_list ap;
	
	size_t max_size = strlen(fmt)+DEBUG_BUFFER_SIZE_EXTRA;

	dbg = (char *)malloc(max_size);
	if (!dbg)
		return 0;
		
	va_start(ap, fmt);
	size_t len = vsnprintf(dbg, max_size, fmt, ap);
	va_end(ap);
	
	if (!file)
	{
		std::string file_path = myself_path + DEBUG_FILE;		
		file = fopen(file_path.c_str(), "w");
		if (!file)
			return 0;
	}

	//fputs(dbg, file);
	uint32_t H, MM, SS, mmm;
	
	local_tick(&H, &MM, &SS, &mmm);
	fprintf(file, "%d:%02d:%02d.%03d: %s", H, MM, SS, mmm, dbg);	
	fflush(file);
	
	free(dbg);
	return len;
}

extern "C"
{

PUBLIC int __attribute__ ((format (printf, 2, 3))) DprintfPatched(uint32_t unk, const char* fmt, ...) 
{
	if (debug_level == 0)
		return 0;
	
	char *dbg;
	va_list ap;
	
	size_t max_size = strlen(fmt)+DEBUG_BUFFER_SIZE_EXTRA;

	dbg = (char *)malloc(max_size);
	if (!dbg)
		return 0;
		
	va_start(ap, fmt);
	size_t len = vsnprintf(dbg, max_size, fmt, ap);
	va_end(ap);
	
	if (debug_level == 1)
	{
		OutputDebugStringA(dbg);
	}
	else if (debug_level == 2)
	{
		if (!file)
		{
			std::string file_path = myself_path + DEBUG_FILE;		
			file = fopen(file_path.c_str(), "w");
			if (!file)
				return 0;
		}

		//fputs(dbg, file);
		uint32_t H, MM, SS, mmm;
		
		local_tick(&H, &MM, &SS, &mmm);
		fprintf(file, "%d:%02d:%02d.%03d: %s", H, MM, SS, mmm, dbg);	
		fflush(file);
	}
	else
	{
		MessageBoxA(NULL, dbg, "XV2 Patcher", 0);
	}
	
	free(dbg);
	return len;
}

PUBLIC void DprintfPatched2(const char* fmt) 
{
	// Meh, the params were deleted from game (by optimizer?), so we cannot print them
	DPRINTF("%s\n", fmt);
}

} // extern "C"

void iggy_trace_callback(void *, void *, const char *str, size_t)
{
	if (str && strcmp(str, "\n") == 0)
		return;
	
	DPRINTF("IGGY: %s\n", str);
}

void iggy_warning_callback(void *, void *, uint32_t, const char *str)
{
	if (str)
	{
		if (strcmp(str, "\n") == 0 || strstr(str, "Cannot maintain framerate specified in SWF file") != nullptr)
			return;
	}
	
	DPRINTF("%s\n", str);
}

void *get_caller(uint8_t level)
{
	void *addr;
	
	if (CaptureStackBackTrace(level+2, 1, &addr, nullptr) > 0)
	{
		return addr;
	}
	
	return nullptr;
}

void PrintStackTrace(uint8_t level)
{
	for (uint8_t i = 0; i < level; i++)
	{
		void *ra = get_caller(i+1);
		if (!ra)
			break;
		
		HMODULE module;
		
		if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)ra, &module))
		{
			char *fp = (char *)malloc(MAX_PATH);
			GetModuleFileNameA(module, fp, MAX_PATH);
			std::string fn = Utils::GetFileNameString(fp);
			free(fp);			
			
			DPRINTF("Called from %s:0x%x\n", fn.c_str(), Utils::DifPointer(ra, module));
		}
		else
		{
			DPRINTF("Called from %p\n", ra);
		}
	}
}
