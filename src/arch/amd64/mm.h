#pragma once

/// The place where the kernel pages are virtually mapped to
#define KERNEL_VIRT_BASE        0xFFFFFF0000000000

/// Page map level 4
typedef uint64_t *mm_pml4_t;
/// Page directory pointer table
typedef uint64_t *mm_pdpt_t;
/// Page directory
typedef uint64_t *mm_pagedir_t;
/// Page table
typedef uint64_t *mm_pagetab_t;

/// Virtual memory space
typedef mm_pml4_t mm_space_t;
