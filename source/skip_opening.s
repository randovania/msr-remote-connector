.text
.align 4

SkipOpeningAsm:
    push  {r0, r1, r2, r3, r7, lr}       /* lr holds caller's return address */
    ldr   r7, =skip_opening              /* load address */
    ldr   r7, [r7]                       /* load value  */
    cmp   r7, #0
    beq   .LAB_RETURN                      
    pop   {r0, r1, r2, r3, r7, lr}       /* restore */
    ldr   r7, =0x4a94c0
    bx    r7                             /* jump to start game function */
.LAB_RETURN:
    pop   {r0, r1, r2, r3, r7, lr}       /* restore */
    stmdb sp!, {r4, r5, r6, r7, r8, lr}  /* original instruction */
    ldr   r7, =0x004ca96c                /* jump to one instruction after overwritten instruction */
    bx    r7                             /* jump */