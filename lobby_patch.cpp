#include <windows.h>

#include <xbyak.h>

#include "PatchUtils.h"

#include "Xv2LobbyDefFile.h"
#include "xv2lobbydef_default.inc"

#include "xv2patcher.h"
#include "game_defs.h"
#include "vc_defs.h"
#include "debug.h"


struct LobbyCMapFile
{
	uint64_t *vtable;
	//...
};

struct CResourceStruct1
{
	LobbyCMapFile *map;
	uint32_t unk8; // may be something like loaded 
	uint32_t pad;
};
CHECK_STRUCT_SIZE(CResourceStruct1, 0x10);


struct LobbyCResource // XG::Game::Lobby::CResource
{
	uintptr_t *vtable;
	uint8_t unk008[0x20-8];
	CResourceStruct1 maps1[MAX_LOBBY+1]; // 0020   Will only relocate from offset 1
	CResourceStruct1 maps2[MAX_LOBBY]; // 0120   array initialization at 1.24.1::414431	
	uint8_t unk0210[0x430-0x210];
};
CHECK_FIELD_OFFSET(LobbyCResource, maps1, 0x20); 
CHECK_FIELD_OFFSET(LobbyCResource, maps2, 0x120); 
CHECK_STRUCT_SIZE(LobbyCResource, 0x430);

struct LobbyCResourceModded : LobbyCResource
{
	CResourceStruct1 maps1_relocated[MAX_LOBBY_MODDED];
	CResourceStruct1 maps2_relocated[MAX_LOBBY_MODDED];
};


struct LobbyMap // XG::Game::Lobby::CMap
{
	uintptr_t *vtable;
	uint8_t unk008[0x28-8];
	void *v1[MAX_LOBBY]; // 0028   Unknown non-virtual objects created, from same kind as the ones below (a1)
	void *a1[MAX_LOBBY]; // 00A0   Unknown non-virtual objects created at 0x3F33BE (1.24.1)
	uint8_t unk118[0x468-0x118];
};
CHECK_FIELD_OFFSET(LobbyMap, v1, 0x28);
CHECK_FIELD_OFFSET(LobbyMap, a1, 0xA0);
CHECK_STRUCT_SIZE(LobbyMap, 0x468);

struct LobbyMapModded : LobbyMap
{
	void *a1_relocated[MAX_LOBBY_MODDED];
};

typedef void * (* CtorType)(void *);
typedef void * (* DtorType)(void *, void *);

static bool set_memory_bp_resource = false, set_memory_bp_map = false;
static bool set_debug_memory_bp_resource = false, set_debug_memory_bp_map = false; // Don't make both of them true!!!
static bool dump_orig = false;

static CtorType LobbyCResourceCtor, LobbyMapCtor;
static DtorType LobbyCResourceVDtor, LobbyMapVDtor;

static uint8_t *debug_object_top;
static char **lobby_map_list_orig = nullptr, **lobby_map_list_mod=nullptr;
static char **lobby_spm_list_orig = nullptr, **lobby_spm_list_mod=nullptr;
static char **lobby_spmdir_list_orig = nullptr, **lobby_spmdir_list_mod = nullptr;
static int *lobby_integer_orig = nullptr, *lobby_integer_mod=nullptr;

static Xv2LobbyDefFile lobby_file;

static void DebugObjectMemoryBreakpoint(void *pc, void *addr, EXCEPTION_POINTERS *)
{
	static char path[MAX_PATH];
	uintptr_t offs = (uintptr_t)addr - (uintptr_t)debug_object_top;
	HMODULE mod = nullptr;
	GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)pc, &mod);	
	
	if (mod)
	{
		uintptr_t rel = (uintptr_t)pc - (uintptr_t)mod;
		GetModuleFileNameA(mod, path, MAX_PATH);	
		std::string fn = Utils::GetFileNameString(path);
		DPRINTF("Object+0x%Ix accessed at %p (%s+0x%I64x)\n", offs, (void *)addr, fn.c_str(), rel);
	}
	else
	{		
		DPRINTF("Object+0x%Ix accessed at %p\n", offs, addr);			
	}		
}

void Maps1Breakpoint(void *pc, void *addr, EXCEPTION_POINTERS *)
{
	UPRINTF("LobbyCResource::map1 was accessed. Look at log for more info.\n");	
	DPRINTF("maps1 accessed at address %p (rel %Ix)\n", pc, RelAddress(pc));
	exit(-1);
}

void Maps2Breakpoint(void *pc, void *addr, EXCEPTION_POINTERS *)
{
	UPRINTF("LobbyCResource::map2 was accessed. Look at log for more info.\n");	
	DPRINTF("maps2 accessed at address %p (rel %Ix)\n", pc, RelAddress(pc));
	exit(-1);
}

void A1Breakpoint(void *pc, void *addr, EXCEPTION_POINTERS *)
{
	UPRINTF("LobbyMap::a1 was accessed. Look at log for more info.\n");	
	DPRINTF("a1 accessed at address %p (rel %Ix)\n", pc, RelAddress(pc));
	exit(-1);
}

void V1Breakpoint(void *pc, void *addr, EXCEPTION_POINTERS *)
{
	UPRINTF("LobbyMap::a1 was accessed. Look at log for more info.\n");	
	DPRINTF("a1 accessed at address %p (rel %Ix)\n", pc, RelAddress(pc));
	exit(-1);
}

extern "C"
{
	
PUBLIC void OnIncreaseLobbyResourceMemory(void *addr)
{
	uint32_t size = PatchUtils::Read32(addr);
	
	if (size != (uint32_t)sizeof(LobbyCResource))
	{
		UPRINTF("Size of object is not expected (Correct sizeof LobbyCResource!)\n");
		exit(-1);
	}
	
	size = sizeof(LobbyCResourceModded);
	DPRINTF("Size of LobbyCResource object changed from 0x%x to 0x%x\n", (uint32_t)sizeof(LobbyCResource), size);
	PatchUtils::Write32(addr, size);
}

static void *LobbyCResourceVDtor_Patched(LobbyCResourceModded *pthis, void *rdx)
{
	LobbyCResourceVDtor(pthis, rdx);
	
	if (set_memory_bp_resource)
	{
		PatchUtils::UnsetMemoryBreakpoint(&pthis->maps1[1], MAX_LOBBY*sizeof(CResourceStruct1)); 
		PatchUtils::UnsetMemoryBreakpoint(pthis->maps2, sizeof(pthis->maps2));
	}	
	
	return pthis;
}

PUBLIC void LobbyCResourceCtorSetup(CtorType orig)
{
	LobbyCResourceCtor = orig;
}
	
PUBLIC void *LobbyCResourceCtor_Patched(LobbyCResourceModded *pthis)
{
	// These two must go before the original constructor! (or otherwise some common code in the constructor/destructor will crash
	memset(pthis->maps1_relocated, 0, sizeof(pthis->maps1_relocated));
	memset(pthis->maps2_relocated, 0, sizeof(pthis->maps2_relocated));
	
	LobbyCResourceCtor(pthis);	
	
	if (set_memory_bp_resource)
	{
		//PatchUtils::SetGenericDebugMemoryBreakPoint(&pthis->maps1[1], MAX_LOBBY*sizeof(CResourceStruct1), "maps1", true); // Only from offset 1!
		//PatchUtils::SetGenericDebugMemoryBreakPoint(pthis->maps2, sizeof(pthis->maps2), "maps2", true);
		PatchUtils::SetMemoryBreakpoint(&pthis->maps1[1], MAX_LOBBY*sizeof(CResourceStruct1), Maps1Breakpoint); // Only from offset 1!
		PatchUtils::SetMemoryBreakpoint(pthis->maps2, sizeof(pthis->maps2), Maps2Breakpoint);
		
		static bool done = false;
		if (!done)
		{
			PatchUtils::HookVirtual(pthis, 0, (void **)&LobbyCResourceVDtor, (void *)LobbyCResourceVDtor_Patched);
			done = true;
		}
	}
	
	if (set_debug_memory_bp_resource)
	{
		debug_object_top = (uint8_t *)pthis;
		PatchUtils::SetMemoryBreakpoint(pthis, sizeof(LobbyCResourceModded), DebugObjectMemoryBreakpoint);
	}
	
	return pthis;
}

static void SetupLobby()
{
	const std::string lobby_def_path = myself_path + CONTENT_ROOT + "data/lobby_def.xml";
	
	if (!Utils::FileExists(lobby_def_path))
	{
		TiXmlDocument doc;
        doc.Parse(default_lobbydef);

        if (doc.ErrorId() != 0)
        {
            UPRINTF("%s: Internal error parsing default lobby_def (this should never happen)\n", FUNCNAME);            
            exit(-1);
        }

        if (!lobby_file.Compile(&doc))
        {
            UPRINTF("%s: Internal error compiling default lobby_def (this should never happen)\n", FUNCNAME);            
            exit(-1);
        }
	}
	else
	{	
		if (!lobby_file.CompileFromFile(lobby_def_path))
		{
			UPRINTF("Failed to compile lobby_def file.\nLog may have more info.\n");		
			exit(-1);
		}
	}
}

PUBLIC void PatchLobbyMapListRel(uint8_t *buf)
{
	char **list = (char **)GetAddrFromRel(buf);
	
	if (!lobby_map_list_orig)
	{
		SetupLobby();
		lobby_map_list_orig = list;
		DPRINTF("Orig lobby map list at %p\n", lobby_map_list_orig);
		
		// Reminder: only the array has to be in 32 bits area. The strings themselves can be anywhere
		lobby_map_list_mod = (char **)PatchUtils::AllocateIn32BitsArea(buf, lobby_file.GetNumDefs()*sizeof(uintptr_t));	
		DPRINTF("Modded lobby map list at %p\n", lobby_map_list_mod);
		
		for (size_t i = 0; i < lobby_file.GetNumDefs(); i++)
		{
			lobby_map_list_mod[i] = (char *)lobby_file[i].map.c_str();
		}
	}
	else if (list != lobby_map_list_orig)
	{
		UPRINTF("%s: I guess something weird happened.\n", FUNCNAME);
		exit(-1);
	}
	
	SetRelAddr(buf, lobby_map_list_mod);
}

PUBLIC void PatchLobbyMapListAbs(uint32_t *buf)
{
	uint8_t *addr = (uintptr_t)*buf + (uint8_t *)GetModuleHandle(nullptr);
	if (addr != (uint8_t *)lobby_map_list_orig)
	{
		UPRINTF("%s: uuuuuuuggggggjjjjhhhh\n", FUNCNAME);
		exit(-1);
	}
	
	PatchUtils::Write32(buf, Utils::DifPointer(lobby_map_list_mod, GetModuleHandle(nullptr)));
}

PUBLIC void PatchLobbyIntegerRel(uint8_t *buf)
{
	int *list = (int *)GetAddrFromRel(buf);
	
	if (!lobby_integer_orig)
	{
		lobby_integer_orig = list;
		DPRINTF("Orig lobby integer at %p\n", lobby_integer_orig);
		
		lobby_integer_mod = (int *)PatchUtils::AllocateIn32BitsArea(buf, lobby_file.GetNumDefs()*sizeof(int));	
		DPRINTF("Modded lobby map list at %p\n", lobby_integer_mod);
		
		for (size_t i = 0; i < lobby_file.GetNumDefs(); i++)
		{
			lobby_integer_mod[i] = lobby_file[i].integer;
		}
	}
	else if (list != lobby_integer_orig)
	{
		UPRINTF("%s: I guess something weird happened.\n", FUNCNAME);
		exit(-1);
	}
	
	SetRelAddr(buf, lobby_integer_mod);
}

PUBLIC void PatchLobbySpmAbs(uint32_t *buf)
{
	uint8_t *addr = (uintptr_t)*buf + (uint8_t *)GetModuleHandle(nullptr);
	if (!lobby_spm_list_orig)
	{
		lobby_spm_list_orig = (char **)addr;
		DPRINTF("Orig lobby spm at %p\n", lobby_spm_list_orig);
		
		lobby_spm_list_mod = (char **)PatchUtils::AllocateIn32BitsArea(buf, lobby_file.GetNumDefs()*sizeof(uintptr_t));
		
		for (size_t i = 0; i < lobby_file.GetNumDefs(); i++)
		{
			lobby_spm_list_mod[i] = (char *)lobby_file[i].spm.c_str();
		}
	}
	else if (addr != (uint8_t *)lobby_spm_list_orig)
	{
		UPRINTF("%s: I guess something weird happened.\n", FUNCNAME);
		exit(-1);
	}
	
	PatchUtils::Write32(buf, Utils::DifPointer(lobby_spm_list_mod, GetModuleHandle(nullptr)));
}

PUBLIC void PatchLobbySpmDirAbs(uint32_t *buf)
{
	uint8_t *addr = (uintptr_t)*buf + (uint8_t *)GetModuleHandle(nullptr);
	if (!lobby_spmdir_list_orig)
	{
		lobby_spmdir_list_orig = (char **)addr;
		DPRINTF("Orig lobby spm dir at %p\n", lobby_spmdir_list_orig);
		
		lobby_spmdir_list_mod = (char **)PatchUtils::AllocateIn32BitsArea(buf, lobby_file.GetNumDefs()*sizeof(uintptr_t));
		
		for (size_t i = 0; i < lobby_file.GetNumDefs(); i++)
		{
			lobby_spmdir_list_mod[i] = (char *)lobby_file[i].spm_dir.c_str();
		}
		
		if (dump_orig)
		{
			lobby_file.GetFromGame(lobby_map_list_orig, lobby_spm_list_orig, lobby_spmdir_list_orig, lobby_integer_orig);
			lobby_file.DecompileToFile("lobby_def.xml");
		}
	}
	else if (addr != (uint8_t *)lobby_spmdir_list_orig)
	{
		UPRINTF("%s: I guess something weird happened.\n", FUNCNAME);
		exit(-1);
	}
	
	PatchUtils::Write32(buf, Utils::DifPointer(lobby_spmdir_list_mod, GetModuleHandle(nullptr)));
}

PUBLIC void PatchLobbyM1A1(uint8_t *addr, size_t size)
{
	EXECBUFFER(code_buf, addr); 
	uintptr_t return_addr = (uintptr_t)addr + size;
	
	// Because rsi is kept, this code covers m1-a1, m1-a2 and m1-a8
	struct Code : Xbyak::CodeGenerator 
	{
		Code(void *buf, uintptr_t ra) : Xbyak::CodeGenerator(4096, buf)
		{
			// r14 is index of maps1 (-1 in original, but +0 in relocated)
			// Original code would make rsi point to offset of &maps[idx+1] / 8
			// Our code will make rsi point instead to offset of &maps1_relocated[idx] / 8 
			mov(rsi, offsetof(LobbyCResourceModded, maps1_relocated));
			shr(rsi, 3);
			// Add twice (same as rsi = rsi + r14*2)
			add(rsi, r14);
			add(rsi, r14);
			// Original code
			cmp(qword[r13+rsi*8+0], 0);
			// Return
			jmp(ptr[rip]);
			dq(ra);
		}
	};	
	
	Code c(code_buf, return_addr);	
	PatchUtils::HookGenericCode(addr, (void *)c.getCode(), size);
}

PUBLIC void PatchLobbyM1A3(uint8_t *addr, size_t size)
{
	EXECBUFFER(code_buf, addr); 
	uintptr_t return_addr = (uintptr_t)addr + size;
	
	struct Code : Xbyak::CodeGenerator 
	{
		Code(void *buf, uintptr_t ra) : Xbyak::CodeGenerator(4096, buf)
		{
			// r14 is index of maps1 (-1 in original, but +0 in relocated)
			// Original code would will make maps[idx+1].unk8 = 1
			// Our code will make maps1_relocated[idx].unk8 = 1 
			lea(rax, ptr[r14]);
			add(rax, rax);
			mov(dword[r13+rax*8+(offsetof(LobbyCResourceModded, maps1_relocated)+8)], 1);
			// Return
			jmp(ptr[rip]);
			dq(ra);
		}
	};	
	
	Code c(code_buf, return_addr);	
	PatchUtils::HookGenericCode(addr, (void *)c.getCode(), size);
}

PUBLIC void PatchLobbyM1A4(uint8_t *addr)
{
	EXECBUFFER(code_buf, addr); 
	size_t size = 6; //(first two instructions)
	uintptr_t return_addr = (uintptr_t)addr + size;
	
	struct Code : Xbyak::CodeGenerator 
	{
		Code(void *buf, uintptr_t ra) : Xbyak::CodeGenerator(4096, buf)
		{
			// Original code will traverse all elements of maps1
			// On our code, we want maps1[0] to access original, and the remaining index to access maps1_relocated[idx-1]
			// To achieve this, just make rbx point to maps1_relocated[0] at idx=1
			cmp(esi, 1);
			jne("L1");
			lea(rbx, ptr[rbp+offsetof(LobbyCResourceModded, maps1_relocated)]);
			// Original code
			L("L1");
			mov(rcx, ptr[rbx]);
			test(rcx, rcx);
			// Return
			jmp(ptr[rip]);
			dq(ra);
		}
	};	
	
	Code c(code_buf, return_addr);	
	
	if (c.hasUndefinedLabel())
	{
		UPRINTF("%s: CRITICAL, undefined label.\n", FUNCNAME);
		exit(-1);
	}
	
	PatchUtils::HookGenericCode(addr, (void *)c.getCode(), size);
}

PUBLIC LobbyCMapFile *ReimplementLobbyM1A5(LobbyCResourceModded *pthis, int idx)
{
	// The original code allowed for both edge cases (-1 and element above capacity of array) but that seems to be a bug on their end, so I'm correcting it here
	if (idx < 0 || idx >= (int)(lobby_file.GetNumDefs()+1))
		return nullptr;
	
	if (idx == 0)
		return pthis->maps1[0].map;
	
	DPRINTF("M1A5: %s\n", lobby_file.GetLobbyDefs()[idx-1].GetDebugName().c_str());
	
	return pthis->maps1_relocated[idx-1].map;
}	

PUBLIC void PatchLobbyM1A6(uint8_t *addr)
{
	EXECBUFFER(code_buf, addr); 
	size_t size = 9; //(first two instructions)
	uintptr_t return_addr = (uintptr_t)addr + size;
	
	// This will cover both M1-A6 and M1-A7
	struct Code : Xbyak::CodeGenerator 
	{
		Code(void *buf, uintptr_t ra) : Xbyak::CodeGenerator(4096, buf)
		{
			// Original code points to &maps[1], just make it point to &maps1_relocated[0]
			lea(rbx, ptr[rsi+offsetof(LobbyCResourceModded, maps1_relocated)]);
			// Original code
			mov(edi, 1);
			// Return
			jmp(ptr[rip]);
			dq(ra);
		}
	};	
	
	Code c(code_buf, return_addr);		
	PatchUtils::HookGenericCode(addr, (void *)c.getCode(), size);
}

PUBLIC void PatchLobbyM1A9(uint8_t *addr)
{
	EXECBUFFER(code_buf, addr); 
	size_t size = 6; //(first two instructions)
	uintptr_t return_addr = (uintptr_t)addr + size;
	
	struct Code : Xbyak::CodeGenerator 
	{
		Code(void *buf, uintptr_t ra) : Xbyak::CodeGenerator(4096, buf)
		{
			// Original code will traverse all elements of maps1
			// On our code, we want maps1[0] to access original, and the remaining index to access maps1_relocated[idx-1]
			// The hook is in the moment the loop advances, so just watch for the first advance (edi=0 -> edi=1)
			inc(edi); // Original code
			cmp(edi, 1);
			jne("L1");
			lea(rbx, ptr[rsi+offsetof(LobbyCResourceModded, maps1_relocated)]);
			jmp("L2");
			// Remaining original code
			L("L1");
			add(rbx, 0x10);
			L("L2");
			// Return
			jmp(ptr[rip]);
			dq(ra);
		}
	};	
	
	Code c(code_buf, return_addr);	
	
	if (c.hasUndefinedLabel())
	{
		UPRINTF("%s: CRITICAL, undefined label.\n", FUNCNAME);
		exit(-1);
	}
	
	PatchUtils::HookGenericCode(addr, (void *)c.getCode(), size);
}

PUBLIC void PatchLobbyM1A10(uint8_t *addr, size_t size)
{
	EXECBUFFER(code_buf, addr); 
	uintptr_t return_addr = (uintptr_t)addr + size;
	
	struct Code : Xbyak::CodeGenerator 
	{
		Code(void *buf, uintptr_t ra, uint32_t numMapsP1) : Xbyak::CodeGenerator(4096, buf)
		{
			// The original code did a 4*4 loop because the number of elements is 16 (MAX_LOBBY+1)
			// We can't have that when the number of lobbys is unknown, so let's replace the whole loop
			lea(rcx, ptr[rsi+offsetof(LobbyCResourceModded, maps1_relocated)]);
			mov(eax, numMapsP1);
			L("LOOP");
			cmp(ptr[rcx], r8);			
			jnz("ADVANCE_LOOP");
			// Zero the object
			mov(dword[rcx+8], r14d);
			mov(ptr[rcx], r14);
			L("ADVANCE_LOOP");
			add(rcx, 0x10);
			sub(rax, 1);
			jnz("LOOP");
			// Return
			jmp(ptr[rip]);
			dq(ra);
		}
	};	
	
	Code c(code_buf, return_addr, lobby_file.GetNumDefs()+1);	
	
	if (c.hasUndefinedLabel())
	{
		UPRINTF("%s: CRITICAL, undefined label.\n", FUNCNAME);
		exit(-1);
	}
	
	PatchUtils::HookGenericCode(addr, (void *)c.getCode(), size);
}

PUBLIC void PatchLobbyM2Multi(uint32_t *addr)
{
	// Let's verify that the patch is pointing to the correct thing
	if (*addr != offsetof(LobbyCResourceModded, maps2))
	{
		UPRINTF("%s: Seems that patch is not pointing to correct location or maps2 offset is not correct (expected 0x%x, got 0x%x)\n", FUNCNAME, (uint32_t)offsetof(LobbyCResourceModded, maps2), *addr);
		exit(-1);
	}
	
	// For M2-A1, this will cover both M2-A1 and M2-A2
	PatchUtils::Write32(addr, offsetof(LobbyCResourceModded, maps2_relocated));
}

PUBLIC LobbyCMapFile *ReimplementLobbyM2A4(LobbyCResourceModded *pthis, int idx)
{
	// AGAIN, the original code allowed for both edge cases (-1 and element above capacity of array), and definitely looks like an error
	if (idx < 0 || idx >= (int)(lobby_file.GetNumDefs()+1))
		return nullptr;
	
	if (idx == 0)
	{
		DPRINTF("*************************************** %s: I got idx = 0, dunno how to react, so I'm leaving the app ~~~\n", FUNCNAME);
		exit(-1);
	}
	
	DPRINTF("M2A4: %s\n", lobby_file.GetLobbyDefs()[idx-1].GetDebugName().c_str());
	
	return pthis->maps2_relocated[idx-1].map;
}

PUBLIC void PatchNumLobbies(uint8_t *num)
{
	if (*num != MAX_LOBBY)
	{
		UPRINTF("%s: Patch points to incorrect offset or MAX_LOBBY is not correct.\n", FUNCNAME);
		exit(-1);
	}
	
	PatchUtils::Write8(num, lobby_file.GetNumDefs());
}

PUBLIC void PatchNumLobbiesP1(uint8_t *num)
{
	if (*num != MAX_LOBBY+1)
	{
		UPRINTF("%s: Patch points to incorrect offset or MAX_LOBBY is not correct.\n", FUNCNAME);
		exit(-1);
	}
	
	PatchUtils::Write8(num, lobby_file.GetNumDefs()+1);
}

PUBLIC void PatchNumLobbiesM1(uint8_t *num)
{
	if (*num != MAX_LOBBY-1)
	{
		UPRINTF("%s: Patch points to incorrect offset or MAX_LOBBY is not correct.\n", FUNCNAME);
		exit(-1);
	}
	
	PatchUtils::Write8(num, lobby_file.GetNumDefs()-1);
}

PUBLIC void OnIncreaseLobbyMapMemory(void *addr)
{
	uint32_t size = PatchUtils::Read32(addr);
	
	if (size != (uint32_t)sizeof(LobbyMap))
	{
		UPRINTF("Size of object is not expected (Correct sizeof LobbyMap!)\n");
		exit(-1);
	}
	
	size = sizeof(LobbyMapModded);
	DPRINTF("Size of LobbyMap object changed from 0x%x to 0x%x\n", (uint32_t)sizeof(LobbyMap), size);
	PatchUtils::Write32(addr, size);
}

static void *LobbyMapVDtor_Patched(LobbyMapModded *pthis, void *rdx)
{
	LobbyMapVDtor(pthis, rdx);
	
	if (set_memory_bp_map)
	{
		PatchUtils::UnsetMemoryBreakpoint(pthis->v1, sizeof(pthis->v1));
	}	
	
	return pthis;
}

PUBLIC void LobbyMapCtorSetup(CtorType orig)
{
	LobbyMapCtor = orig;
}
	
PUBLIC void *LobbyMapCtor_Patched(LobbyMapModded *pthis)
{
	memset(pthis->a1_relocated, 0, sizeof(pthis->a1_relocated));	
	// TODO: poner lo mismo para el v1_relocated
	
	LobbyMapCtor(pthis);	

	if (set_memory_bp_map)
	{
		//PatchUtils::SetGenericDebugMemoryBreakPoint(pthis->a1, sizeof(pthis->a1), "a1", true);
		//PatchUtils::SetGenericDebugMemoryBreakPoint(pthis->v1, sizeof(pthis->v1), "v1", true);
		PatchUtils::SetMemoryBreakpoint(pthis->a1, sizeof(pthis->a1), A1Breakpoint);
		//PatchUtils::SetMemoryBreakpoint(pthis->v1, sizeof(pthis->v1), V1Breakpoint);
		
		static bool done = false;
		if (!done)
		{
			PatchUtils::HookVirtual(pthis, 0, (void **)&LobbyMapVDtor, (void *)LobbyMapVDtor_Patched);
			done = true;
		}
	}	
	
	if (set_debug_memory_bp_map)
	{
		debug_object_top = (uint8_t *)pthis;
		PatchUtils::SetMemoryBreakpoint(pthis, sizeof(LobbyMapModded), DebugObjectMemoryBreakpoint);
	}
	
	return pthis;
}

PUBLIC bool ReimplementLobbyEFlag(LobbyMapModded *, int id)
{
	size_t idx = (size_t)(id-1);
	if (idx < lobby_file.GetNumDefs())
		return lobby_file.GetLobbyDefs()[idx].eflag;
	
	return false;
}

// 7 bytes single instruction overwritten. [IN] rcx -> LobbyMap(Modded). [OUT] rbx -> A1
PUBLIC void PatchLobbyA1RW1_A1R2A1R3(uint8_t *addr)
{
	EXECBUFFER(code_buf, addr); 
	size_t size = 7; // sizeof first instruction
	uintptr_t return_addr = (uintptr_t)addr + size;
	
	struct Code : Xbyak::CodeGenerator 
	{
		Code(void *buf, uintptr_t ra) : Xbyak::CodeGenerator(4096, buf)
		{
			// Original code points to a1, just make it point to a1_relocated
			lea(rbx, ptr[rcx+offsetof(LobbyMapModded, a1_relocated)]);
			// Return
			jmp(ptr[rip]);
			dq(ra);
		}
	};	
	
	Code c(code_buf, return_addr);		
	PatchUtils::HookGenericCode(addr, (void *)c.getCode(), size);
}

} // extern "C"