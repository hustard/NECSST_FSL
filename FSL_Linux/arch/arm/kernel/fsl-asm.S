#include <linux/linkage.h>
#include <linux/threads.h>
#include <asm/asm-offsets.h>
#include <asm/assembler.h>
#include <asm/glue-cache.h>
#include <asm/glue-proc.h>

	.text

  	.macro compute_mpidr_hash dst, rs0, rs1, rs2, mpidr, mask
	and \mpidr, \mpidr, \mask           @ mask out MPIDR bits
	and \dst, \mpidr, #0xff         @ mask=aff0
 ARM(   mov \dst, \dst, lsr \rs0        )   @ dst=aff0>>rs0
 THUMB( lsr \dst, \dst, \rs0        )
	and \mask, \mpidr, #0xff00          @ mask = aff1
 ARM(   orr \dst, \dst, \mask, lsr \rs1 )   @ dst|=(aff1>>rs1)
 THUMB( lsr \mask, \mask, \rs1      )
 THUMB( orr \dst, \dst, \mask       )
	and \mask, \mpidr, #0xff0000        @ mask = aff2
 ARM(   orr \dst, \dst, \mask, lsr \rs2 )   @ dst|=(aff2>>rs2)
 THUMB( lsr \mask, \mask, \rs2      )
 THUMB( orr \dst, \dst, \mask       )
	.endm


ENTRY(__fsl_cpu_suspend)

	stmfd 	sp!, {r4 - r11, lr}
#ifdef MULTI_CPU
	ldr r10, =processor
	ldr r4, [r10, #CPU_SLEEP_SIZE]
#else
	ldr r4, =cpu_suspend_size
#endif

	mov r5, sp
	add r4, r4, #12
	sub sp, sp, r4
	ldr r3, =sleep_save_sp
	stmfd sp!, {r0, r1}
	ldr r3, [r3, #SLEEP_SAVE_SP_VIRT]
	ALT_SMP(ldr r0, =mpidr_hash)
	ALT_UP_B(1f)

	ldmia r0, {r1, r6-r8}
	compute_mpidr_hash 	r0, r6, r7, r8, r2, r1
	add r3, r3, r0, lsl #2
1:	mov r2, r5
	mov r1, r4
	add r0, sp, #8
	bl fsl_suspend_save
	
/*	 Hope we will not arrive here  */ 
	mov pc, lr 
ENDPROC(__fsl_cpu_suspend)
	.ltorg
