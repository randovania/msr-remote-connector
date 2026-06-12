.text
.align 4

/*
 * That function force-selects the Grapple Beam (002a8a58) when the aim ray hits an
 * entity carrying a CGrapplePointComponent. The randomizer's grapple-doors are NOT
 * grapple points -- they are door shields (doorshieldgrapplebeam) whose LIFE
 * component is weak to GRAPPLE_BEAM -- so vanilla bails here with the grapple-target
 * flag r10 = 0 and never switches.
 *
 * This hook adds a second path: when no grapple point was found (r0 == 0), flag the
 * target anyway (r10 = 1) if the hit entity is a door shield weak to GRAPPLE_BEAM.
 *
 * Discriminator
 *   1. the entity's LIFE component is susceptible to GRAPPLE_BEAM (mask +0xb4), AND
 *   2. that LIFE component is an instance of CDoorShieldLifeComponent.
 *
 * Registers at entry (set up by the surrounding vanilla code):
 *   r0 = GetComponentForEntity_(hitEntity, "GRAPPLEPOINT")  (0 for a door shield)
 *   r4 = the hit CCollisionComponent ; hitEntity = [r4, #0x8]
 *   r10 = grapple-target flag (uVar30), already 0 on this bail path
 *   lr = 0x0029fc44 (link:true)
 *
 * Offsets / constants:
 *   CEntity->CLifeComponent @ +0x12C ; CLifeComponent susceptibility mask @ +0xb4
 *   DamageSourceBitfield GRAPPLE_BEAM = 0x400000
 *   RefToCDoorShieldLifeComponent = 0x00123234 ; CompareObjectToClass_ = 0x001eb828
 */
GrappleDoorAutoSwitchAsm:
    cmp   r0, #0
    bxne  lr                    /* real grapple point -> keep vanilla behaviour */

    push  {r0, r1, r2, r3, r12, lr}
    ldr   r0, [r4, #0x8]        /* hitEntity (CCollisionComponent owner) */
    cmp   r0, #0                /* null check */
    beq   .Lout

    ldr   r0, [r0, #0x12C]      /* hitEntity->CLifeComponent */
    cmp   r0, #0                /* null check */
    beq   .Lout

    ldr   r1, [r0, #0xb4]       /* susceptibility DamageSource mask */
    tst   r1, #0x400000         /* weak to GRAPPLE_BEAM ? */
    beq   .Lout                 /* no -> not a grapple target */

    /* get the RefToCDoorShieldLifeComponent */
    push  {r0}                  /* save lifeComp (obj) across the call */
    bl    0x00123234            /* RefToCDoorShieldLifeComponent -> r0 = class */
    mov   r1, r0

    /* compare if current object is of CDoorShieldLifeComponent */
    pop   {r0}
    bl    0x001eb828            /* CompareObjectToClass_(lifeComp, class) -> r0 = bool */
    cmp   r0, #0
    movne r10, #1               /* it IS a grapple-weak door shield -> auto-switch */
.Lout:
    pop   {r0, r1, r2, r3, r12, lr}
    cmp   r0, #0                /* r0 restored to 0 -> Z=1 so the following beq is taken */
    bx    lr                    /* lr restored to 0x0029fc44 by the pop above */
