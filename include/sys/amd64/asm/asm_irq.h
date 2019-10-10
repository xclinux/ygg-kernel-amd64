#pragma once

#define AMD64_STACK_CTX_CANARY      0xBAD57AC6BAD57AC6
// TODO: use base + off instead of an absolute address
#define AMD64_LAPIC_REG_EOI_ABS     0xFFFFFF00FEE000B0

#if defined(__ASM__)
.macro irq_push_ctx
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx

    // Don't push rsp - it's ignored
    pushq %rbp
    pushq %rsi
    pushq %rdi

    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    movq %ds, %rax
    pushq %rax
    movq %es, %rax
    pushq %rax
    movq %fs, %rax
    pushq %rax
    movq %gs, %rax
    pushq %rax

#if defined(AMD64_STACK_CTX_CANARY)
    movq $AMD64_STACK_CTX_CANARY, %rax
    pushq %rax
#endif
.endm

.macro irq_pop_ctx
#if defined(AMD64_STACK_CTX_CANARY)
    movq $AMD64_STACK_CTX_CANARY, %rax
    cmpq (%rsp), %rax
    jne amd64_kstack_canary_invalid
    popq %rax
#endif

    // XXX: should they be popped this way?
    popq %rax
    movq %rax, %gs
    popq %rax
    movq %rax, %fs

    popq %rax
    movq %rax, %es
    popq %rax
    movq %rax, %ds

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8

    popq %rdi
    popq %rsi
    popq %rbp

    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
.endm

.macro irq_eoi_lapic, n
    // n is actually ignored for APIC
    movq $AMD64_LAPIC_REG_EOI_ABS, %rax
    movl $0, (%rax)
.endm
#endif