.text
.fpu vfpv2
.align 4

AreaDenomTrampA:
    push {r5, r6, r7, r8, r9, r10, r11, lr}
    add  r0, sp, #0x118                         /* &MapName  (orig sp+0xf8, +0x20 for the push)  */
    bl   get_area_denom                         /* r0 = GAME_PROGRESS.<MapName>_PROGRESS (>=1)    */
    vmov s0, r0                                 /* the overwritten instruction */
    pop  {r5, r6, r7, r8, r9, r10, r11, lr}
    ldr  pc, =0x0048624c                        /* explicit return to the next instruction        */

AreaDenomTrampB:
    push {r5, r6, r7, r8, r9, r10, r11, lr}
    add  r0, sp, #0xd8                         /* &MapName  (orig sp+0xb8, +0x20 for the push)  */
    bl   get_area_denom
    vmov s0, r0
    pop  {r5, r6, r7, r8, r9, r10, r11, lr}
    ldr  pc, =0x004865e4
