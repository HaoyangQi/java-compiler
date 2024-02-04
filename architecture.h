#pragma once
#ifndef __COMPILER_ARCHITECTURE_H__
#define __COMPILER_ARCHITECTURE_H__

/**
 * Architecture Bit Length
*/
typedef enum
{
    ARCH_32_BIT = 32,
    ARCH_64_BIT = 64,
} arch_bl;

/**
 * Architecture Info
*/
typedef struct
{
    arch_bl bits;
} architecture;

#endif
