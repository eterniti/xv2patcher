	.intel_syntax noprefix
	.globl	FS_SetEdi
	.def	FS_SetEdi;	.scl	2;	.type	32;	.endef
	
FS_SetEdi:

	mov edi, ecx
	ret
	
	.p2align 4,,15