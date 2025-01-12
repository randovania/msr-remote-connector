.text
.align 4

GravitySuitAsm:
    push  {r7}
    ldr r7, =gravity_suit_damage_flags  /* load address of our damaga flags */
    ldr r7, [r7]                        /* load value of our damaga flags */
    tst r0,r7                           /* update condition flags in processor */
    pop  {r7}                           /* clean up */
    bx lr                               /* jump to 0x00384d4 which is a bne instruction */