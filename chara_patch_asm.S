	.intel_syntax noprefix
	.globl	GetRealAura
	.def	GetRealAura;	.scl	2;	.type	32;	.endef
	
GetRealAura:

	cmp	rcx, 4095
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

	cmp edx, 4095
	ja orig2
	lea rcx, cus_aura_bh11_lookup[rip]
	movsx rdx, edx
	movzx ecx, BYTE PTR [rcx+rdx]
	cmp ecx, 25
	ja orig2
	mov edx, ecx
	
orig2:

	cmp edx, 2
	jl ret2
	
	cmp edx, 3
	jle lab_E0
	
	cmp edx, 0xD
	je lab_E0
	
	cmp edx, 0x16
	je lab_E0
	
	cmp edx, 0x17
	jne ret2
	
	mov DWORD PTR [rbx+0xDC], 0x1C2
	mov DWORD PTR [rbx+0xE0], 0x1C3
	jmp ret2
	
lab_E0:

	mov DWORD PTR [rbx+0xDC], 0xE0
	mov DWORD PTR [rbx+0xE0], 0xE1

ret2:

	mov rcx, 0x123456789ABCDEF
	jmp rcx
	.p2align 4,,15
	

	.intel_syntax noprefix
	.globl	ForceTeleport
	.def	ForceTeleport;	.scl	2;	.type	32;	.endef
	
ForceTeleport:

	mov ecx, [rbx+0x2148]
	cmp ecx, 4095
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

	cmp  [rbx+0x2148], eax
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
	cmp edx, 4095
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

	# r8 = 0x2900 + (r14 << 5) + rsi
	mov r8, 0x2900  # 0x2900 is the size of original object, and the address where we store the new slots array
	mov r9, r14
	shl r9, 5
	add r8, r9
	add r8, rsi
	cmp qword ptr [r8+0x18], 8

	mov rax, 0x123456789ABCDEF
	jmp rax

	.p2align 4,,15