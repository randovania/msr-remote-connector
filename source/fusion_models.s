.text
.align 4

.macro check_fusion_flag
    ldr ip, =use_fusion_models
    ldrb ip, [ip]
    cmp ip, #1
.endm

FusionModelsFirstHookAsm:
    check_fusion_flag
    bne 1f
    mov r1, #1
    bx lr
1:
    ldrb r1, [r0, #0x59]
    bx lr

FusionModelsSecondHookAsm:
    check_fusion_flag
    bne 1f
    mov r0, #1
    bx lr
1:
    ldrb r0, [r0, r6]
    bx lr