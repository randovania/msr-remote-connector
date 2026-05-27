.text
.align 4

/*
 * Loads the "use_fusion_models" byte from c code and compares.
 * equal = fusion models are active
 * not equal = fusion models are inactive
 */
.macro check_fusion_flag
    push {r0}
    ldr r0, =use_fusion_models
    ldrb r0, [r0]
    cmp r0, #1
    pop {r0}
.endm

FusionModelsFirstHookAsm:
    check_fusion_flag
    bne 1f                  // no fusion models => jump to 1
    mov r1, #1              // original code compares to "fusion mode is false", writing 1 means we load fusion models
    bx lr
1:
    ldrb r1, [r0, #0x59]    // original instruction, loads the vanilla "is fusion mode" flag to r1
    bx lr

FusionModelsSecondHookAsm:
    check_fusion_flag
    bne 1f                  // no fusion models => jump to 1
    mov r0, #1              // original code compares to "fusion mode is false", writing 1 means we load fusion models
    bx lr
1:
    ldrb r0, [r0, r6]       // original instruction, loads the vanilla "is fusion mode" flag to r0
    bx lr