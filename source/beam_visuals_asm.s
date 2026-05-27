.text
.align 4

/*
 * Loads the "has_wave" byte and from c code and compares.
 * eq = wave collected (keep spazer behaviour)
 * ne = wave missing  (fall through to power-beam redirect)
 */
.macro check_has_wave
    push {r0}
    ldr r0, =has_wave
    ldrb r0, [r0]
    cmp r0, #1
    pop {r0}
.endm

SpazerVisualsAsm:
    push {r0}

    // burst mode check
    ldr  r0, [r6]
    cmp  r0, #0x2
    beq  .KEEP_SPAZER

    // this part is for without burst
    check_has_wave
    beq  .KEEP_SPAZER

    // use power beam enum but damage values
    // are loaded before this part
    mov  r1, #0
    pop  {r0}
    bx   lr

.KEEP_SPAZER:
    /* use spazer enum */
    mov  r1, #4
    pop  {r0}
    bx   lr





SpazerGlowConditionalHookAsm:
    check_has_wave
    beq 1f                 // has wave => jump to 1

    // wave not collected => go to CPowerBeamGun's charge glow function
    // spazer function has executed stmdb sp!,{r4,lr};
    ldr lr, [sp, #4]                // recover real caller LR
    add sp, sp, #8                  // drop the {r4,lr}
    ldr pc, =0x002af8e8             // jump to power-beam version

// this part executes the orginal instruction and then goes back to the original code
// this means we have wave and get the original wave colored glow
1:
    sub sp, sp, #0x10               // original instruction
    bx lr
