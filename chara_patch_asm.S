	.intel_syntax noprefix
	.globl	GetRealAura
	.def	GetRealAura;	.scl	2;	.type	32;	.endef
	
GetRealAura:

	cmp	rcx, 8191
	ja	ret1
	lea	rdx, cus_aura_lookup[rip]
	movzx	eax, WORD PTR [rdx+rcx*2]
	cmp	ax, -1
	je	ret1
	mov ebx, eax
	
ret1:

	mov rdx, 0x123456789ABCDEF
	jmp rdx
	.p2align 4,,15
	
	.global GetCABehaviour11
	.def	GetCABehaviour11;	.scl	2;	.type	32;	.endef
	
GetCABehaviour11:	
	mov edx, [rbx+0x223C]
	# AFTER HERE, EDX MUST NOT BE OVERWRITTEN OR IT WILL OVERWRITE BEHAVIOUR10
	cmp edx, 8191
	ja orig2
	lea rcx, cus_aura_bh11_lookup[rip]
	movsx r8, edx
	movzx ecx, BYTE PTR [rcx+r8]
	cmp ecx, 30 # must match behaviour max in Xv2PreBakedFile
	ja orig2
	lea eax, [rcx-2]
	cmp eax, 0x1A
	jmp ret2
	
orig2:

	lea eax, [rdx-2]
	cmp eax, 0x1A	
	
ret2:
# return to B5A9A (1.20)
	mov rcx, 0x123456789ABCDEF
	jmp rcx
	.p2align 4,,15
	

	.intel_syntax noprefix
	.globl	ForceTeleport
	.def	ForceTeleport;	.scl	2;	.type	32;	.endef
	
ForceTeleport:

	mov ecx, [rbx+0x223C]
	cmp ecx, 8191
	ja default_teleport
	lea rdx, force_teleport[rip]
	mov r8, rcx
	movzx ecx, BYTE PTR [rcx+rdx]
	test ecx, ecx
	jne	teleport
	
	lea rdx, cus_aura_bh66_lookup[rip]
	movzx ecx, BYTE PTR [r8+rdx]
	cmp ecx, 0xFF
	je	default_teleport
	
	cmp eax, ecx
	je	teleport
	
default_teleport:

	cmp  [rbx+0x223C], eax
	jne no_teleport

teleport:	
	
	mov rcx, 0x123456789ABCDEF
	jmp rcx
	
no_teleport:

	mov rcx, 0xFEDCBA987654321
	jmp rcx

	.p2align 4,,15
	

	.global CanUseAnyDualSkill
	.def	CanUseAnyDualSkill;	.scl	2;	.type	32;	.endef
	
CanUseAnyDualSkill:

	test byte ptr [rbx+0xB0], 1
	jnz use_any_dual
	
	lea rax, any_dual_skill_lookup[rip]
	mov edx,  [rbx+0xBC]
	cmp edx, 8191
	ja default_dual_usage
	movzx eax, BYTE PTR [rax+rdx] 
	test eax, eax
	jnz	use_any_dual
	
default_dual_usage:

	mov rax, 0x123456789ABCDEF
	jmp rax
	
use_any_dual:

	mov rax, 0xFEDCBA987654321
	jmp rax

	.p2align 4,,15
	

	.global ModifyArrayOffset4Asm
	.def	ModifyArrayOffset4Asm;	.scl	2;	.type	32;	.endef
	
ModifyArrayOffset4Asm:

	# r8 = 0x2910 + (r14 << 5) + rsi
	mov r8, 0x2910  # 0x2910 is the size of original object, and the address where we store the new slots array
	mov r9, r14
	shl r9, 5
	add r8, r9
	add r8, rsi
	cmp qword ptr [r8+0x18], 8

	mov rax, 0x123456789ABCDEF
	jmp rax

	.p2align 4,,15
	
	
	.intel_syntax noprefix
	.globl	AurToBpeMap
	.def	AurToBpeMap;	.scl	2;	.type	32;	.endef
	
AurToBpeMap:
	# In its current value: edx = aur_id
	cmp edx, 8191
	ja if_not_bpe
	lea rax, aur_bpe_map[rip]
	mov edx, DWORD PTR [rax+rdx*4] # edx is bpe id or -1 now	
	cmp edx, 0
	jl if_not_bpe	

if_bpe:
	# (1.21: 0xF28F5)
	mov rax, 0x123456789ABCDEF
	jmp rax
	
if_not_bpe:
	# Jump to original default case (1.21: 0xF296F)
	mov rax, 0xFEDCBA987654321
	jmp rax		

	.p2align 4,,15
	
	.intel_syntax noprefix
	.globl	GetCABehaviour64
	.def	GetCABehaviour64;	.scl	2;	.type	32;	.endef
	
GetCABehaviour64:
	mov eax, [rbx+0x223C]
	cmp eax, 8191
	ja orig3
	lea rcx, cus_aura_bh64_lookup[rip]
	movzx ecx, BYTE PTR [rax+rcx]
	cmp ecx, 30 # must match behaviour max in Xv2PreBakedFile
	ja orig3
	mov eax, ecx
	
orig3:
	add eax, 0xFFFFFFFC
	cmp eax, 0x1A
	# Jump to original default case (1.21: 0xD9970)
	mov rcx, 0x123456789ABCDEF
	jmp rcx
	
	.p2align 4,,15
	