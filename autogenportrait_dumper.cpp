#include <windows.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <unordered_set>
#include "Utils.h"
#include "PatchUtils.h"
#include "xv2patcher.h"
#include "chara_patch.h"
#include "ui_patch.h"
#include "autogenportrait_dumper.h"
#include "debug.h"

#define DUMP_PATH u"./XV2PATCHER/"

static const uint8_t bmp_header[0x7A] = 
{
	0x42, 0x4D, 0x7A, 0xF6, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x00, 0x00, 0x00, 0x6C, 0x00, 
	0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0xFD, 0xFF, 0xFF, 0x01, 0x00, 0x20, 0x00, 0x03, 0x00, 
	0x00, 0x00, 0x00, 0xF6, 0x54, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0xC4, 0x0E, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

typedef HRESULT (* D3DX11SaveTextureToMemoryType)(ID3D11DeviceContext *pContext, ID3D11Resource *pSrcTexture, int DestFormat, ID3D10Blob **ppDestBuf, UINT flags);
typedef ID3D11View *(* GetTextureBufferType)(void *);

static D3DX11SaveTextureToMemoryType _D3DX11SaveTextureToMemory;
static GetTextureBufferType GetTextureBuffer;

static uint8_t orig_ins[6];
static uint8_t *patch_addr;

extern "C"
{
	
static const char16_t *_GetFirstAutoGenPortraitCharName()
{
	if (!pms_singleton)
		return nullptr;
	
	Battle_Core_MainSystem *mainsystem = *pms_singleton;
	
	if (!mainsystem || !mainsystem->cockpit)
		return nullptr;
	
	for (int i = 0; i < MAX_MOBS_HUD; i++)
	{
		Battle_HudCharInfo *info = &mainsystem->cockpit->char_infos[i];
		
		if (info->index < 0)
			continue;
		
		if (info->is_cac || (info->cms_entry < LOOKUP_SIZE && auto_btl_portrait_lookup[info->cms_entry]))
		{
			return info->GetName();
		}
	}
	
	return nullptr;
}

std::string GetFirstAutoGenPortraitCharName()
{
	std::string ret;
	const char16_t *name = _GetFirstAutoGenPortraitCharName();
	
	if (name)
	{
		ret = Utils::Ucs2ToUtf8(name);
	}
	
	return ret;
}

static ID3D11View *GetTextureBufferHook(void *object)
{
	// This function is only called from one thread and only once at a time, we don't need to worry about thread issues here.
	
	PatchUtils::Copy(patch_addr, orig_ins, sizeof(orig_ins));	
	ID3D11View *view = GetTextureBuffer(object);
	
	if (!view)
		return view;	
	
	if (!_D3DX11SaveTextureToMemory)
	{
		HMODULE hMod = GetModuleHandle("d3dx11_43.dll");
		if (!hMod)
		{
			DPRINTF("Failed get handle to d3dx11_43.dll.\n");
			return view;
		}
		
		_D3DX11SaveTextureToMemory = (D3DX11SaveTextureToMemoryType) GetProcAddress(hMod, "D3DX11SaveTextureToMemory");
		if (!_D3DX11SaveTextureToMemory)
		{
			DPRINTF("Failed to get address of D3DX11SaveTextureToMemory.\n");
			return view;
		}
	}	

	ID3D10Blob *blob = nullptr;		
	ID3D11Device *device = nullptr;
	ID3D11DeviceContext *context = nullptr;
	ID3D11Resource *texture = nullptr;
	
	view->GetDevice(&device);
	
	if (!device)
		return view;
	
	DPRINTF("Device is %p\n", device);
	
	device->GetImmediateContext(&context);
	
	if (!context)
		return view;
	
	view->GetResource(&texture);
	
	if (!texture)
		return view;
	
	DPRINTF("Texture is %p\n", texture);
			
	HRESULT res = _D3DX11SaveTextureToMemory(context, texture, 4, &blob, 0);
	
	if (res == 0 && blob)
	{
		const uint8_t *dds_buf = (const uint8_t *)blob->GetBufferPointer();
		int32_t dds_width, dds_height;
		int32_t bmp_width, bmp_height;
		
		dds_width = *(int32_t *)&dds_buf[0x10];
		dds_height = *(int32_t *)&dds_buf[0xC];
		
		bmp_width = dds_width / 2;
		bmp_height = dds_height / 2;
		
		size_t bmp_size = sizeof(bmp_header) + (bmp_width*bmp_height*4);
		uint8_t *bmp_buf = new uint8_t[bmp_size];			
		memcpy(bmp_buf, bmp_header, sizeof(bmp_header));	

		*(int32_t *)&bmp_buf[0x12] = bmp_width;
		*(int32_t *)&bmp_buf[0x16] = -bmp_height; // Negative height = store the image top-down
		
		for (int y = 0; y < bmp_height; y++)
		{
			const uint32_t *dds_line = ((const uint32_t *)(dds_buf + 0x94)) + y*dds_width;
			uint32_t *bmp_line = ((uint32_t *)(bmp_buf + sizeof(bmp_header))) + y*bmp_width;
			
			for (int x = 0; x < bmp_width; x++)
			{
				bmp_line[x] = dds_line[x];
			}
		}
		
		const char16_t *name = _GetFirstAutoGenPortraitCharName();
		std::u16string out_path = std::u16string(DUMP_PATH) + ((name) ? std::u16string(name) : u"portrait") + u".bmp";
		
		if (Utils::WriteFileBool(out_path, bmp_buf, bmp_size))
		{
			DPRINTF("Autogen portrait dumped succesfully.\n");
		}
		else
		{
			DPRINTF("Failed to save autogen, write function failed.\n");
		}
		
		delete[] bmp_buf;
	}
	else
	{
		DPRINTF("D3DX11SaveTextureToMemory failed with error 0x%x\n", (unsigned int)res);
	}
	
	
	context->Release();
	device->Release();	
	
	
	return view;
}

PUBLIC void SetupAutoGenPortraitDumper(uint8_t *address)
{
	memcpy(orig_ins, address, sizeof(orig_ins));
	patch_addr = address;
	
	HMODULE mod = GetModuleHandle("xgcore.dll");
	if (!mod)
	{
		UPRINTF("FATAL: failed to get handle to xgcore.dll.\n");
		exit(-1);
	}
	
	GetTextureBuffer = (GetTextureBufferType) GetProcAddress(mod, "?GetTextureBuffer@EmCanvas@Eva@XG@@QEAAPEAXXZ");
	if (!GetTextureBuffer)
	{
		UPRINTF("Failed to get address of GetTextureBuffer.\n");
		exit(-1);
	}
}

bool CanDumpAutoGenPortrait()
{
	if (!GetTextureBuffer) // Patch not enabled
		return false;
	
	if (IsBattleUIHidden())
		return false;
	
	return (_GetFirstAutoGenPortraitCharName() != nullptr);
}

void DumpAutoGenPortrait()
{
	if (*patch_addr == 0xFF)
	{
		PatchUtils::Write16(patch_addr, 0xE890);
		PatchUtils::HookCall(patch_addr+1, nullptr, (void *)GetTextureBufferHook);
	}
}	

}