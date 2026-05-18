.text
.align 4

ProjectileAsm:
    push {r7}
    ldr r7, =has_spazer     /* load address of has_spazer */
    ldr r7, [r7]            /* load value of has_spazer */
    cmp r7, #0x01
    pop {r7}            
    bxne lr                 /* no spazer, so go back to original function, skips projectile creation because we overwrote it */
    b 0x002c3e14            /* branch to CreatePlasmaBeamProjectiles (eventually it jumps back to lr which has the addr of ProjectileXHook + 4) */

SpazerShootThroughBlocksAsm:
    ldr r0, =has_wave       /* load address of has_wave */
    ldr r0, [r0]            /* load value of has_wave */
    bx lr                   /* jump back because it is compared against r0 in vanilla code */

PlasmaShootThroughBlocksAsm:
    push {r0}               /* save r0 */
    ldr r0, =has_wave       /* load address of has_wave */
    ldr r0, [r0]            /* load value of has_wave */
    strb r0,[r4,#0xb8]      /* vanilla code writes the flag to this offset */
    pop {r0}                /* restore r0 */
    bx lr                   /* jump back */
