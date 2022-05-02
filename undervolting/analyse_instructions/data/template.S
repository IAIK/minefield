.intel_syntax noprefix


.text
.global target_instruction
.type target_instruction, @function

target_instruction:
    push R12
    push R13
    push R14
    push R15

    cmp RDI, 0
    je skip1

    # store inits for AVX
    push qword ptr [RDI + 0]
    push qword ptr [RDI + 8]
    push qword ptr [RDI + 16]
    push qword ptr [RDI + 24]
    push qword ptr [RDI + 32]
    push qword ptr [RDI + 40]
    push qword ptr [RDI + 48]
    push qword ptr [RDI + 56]

    jmp done1
skip1:
    sub RSP, 64
done1:

    mov RAX, RSP // memory target
    mov R11, RSP // memory target
    
    push R8      // trap factor
    mov R15, R8 // factor pointer for the traps

    pushf

    push R9 // trap init
    push R9 // trap init
    vmovdqu xmm5, xmmword ptr[rsp]

    // init the results
    mov R8, RDI  // inits
    mov R9, RSI  // inits_count

    mov R12, RDX // results
    mov R13, RCX // results_count

    // init traps
    pop RDI      // trap1 init
    pop RDI      // trap1 init
    mov RSI, RDI // trap2 init
    pcmpeqq    %xmm0, %xmm0
    pxor       %xmm3, %xmm3
    
    %%template_asm%%

    popf
    add RSP, 8
    add RSP, 64
    pop R15
    pop R14
    pop R13
    pop R12
    ret


.text
.global target_instruction_protected
.type target_instruction_protected, @function


// int (*)(uint64_t const *init, uint64_t init_count, uint64_t *results, uint64_t results_count, uint64_t trap_factor, uint64_t trap_init)
// rdi = init
// rsi = init_count
// rdx = results
// rcx = results_count
// r8 = trap_factor
// r9 = trap_init

target_instruction_protected:
    push R12
    push R13
    push R14
    push R15

    # store inits for AVX
    cmp RDI, 0
    je skip2

    # store inits for AVX
    push qword ptr [RDI + 0]
    push qword ptr [RDI + 8]
    push qword ptr [RDI + 16]
    push qword ptr [RDI + 24]
    push qword ptr [RDI + 32]
    push qword ptr [RDI + 40]
    push qword ptr [RDI + 48]
    push qword ptr [RDI + 56]

    jmp done2
skip2:
    sub RSP, 64
done2:
    mov RAX, RSP // memory target
    mov R11, RSP // memory target
    
    push R8      // trap factor
    mov R15, R8 // factor pointer for the traps

    pushf

    push R9 // trap init
    push R9 // trap init
    vmovdqu xmm5, xmmword ptr[rsp]

    // init the results
    mov R8, RDI  // inits
    mov R9, RSI  // inits_count

    mov R12, RDX // results
    mov R13, RCX // results_count

    // init traps
    pop RDI      // trap1 init
    pop RDI      // trap1 init
    mov RSI, RDI // trap2 init
    pcmpeqq    %xmm4, %xmm4
    pxor       %xmm3, %xmm3

    %%template_protected%%

    popf
    add RSP, 8
    add RSP, 64
    pop R15
    pop R14
    pop R13
    pop R12
    ret

.text
.global get_load_count
.type get_load_count, @function
get_load_count:
    mov RAX, %%template_loaded%%
    ret

.text
.global get_store_count
.type get_store_count, @function
get_store_count:
    mov RAX, %%template_stored%%
    ret


.data
string1:
.string "HelloWorld"
string2:
.string "HelloWorlth"

