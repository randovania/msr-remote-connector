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
