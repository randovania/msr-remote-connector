.text
.align 4

ChargeDoorImmuneToBurstAsm:
    push {r1, r2, r3}
    ldr  r3, =charge_door_burst_buff
    ldrb r3, [r3]
    cmp  r3, #0             /* option disabled? -> keep vanilla behaviour */
    beq  .Lcont
    ldr  r1, [r8, #0x64]    /* incoming DamageSource bits */
    tst  r1, #0x80          /* is WEAPON_BOOST / Burst flag set? */
    beq  .Lcont             /* Z flag is zero => no flag in the projectile's DamageSource */
    ldr  r2, [r4, #0xb4]    /* door vulnerability mask */
    tst  r2, #0x400         /* is CHARGE_BEAM a vulnerability? */
    beq  .Lcont             /* Z flag is zero => no charge beam door */
    mov  r0, #0             /* vtable+0xac result -> "do not open" */
.Lcont:
    pop  {r1, r2, r3}
    cmp  r0, #0x0           /* overwritten instruction (sets flags for the following beq) */
    b    0x0032f62c
