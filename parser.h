#pragma once
#ifndef __COMPILER_PARSER_H__
#define __COMPILER_PARSER_H__

#include "types.h"
#include "token.h"

typedef struct _java_parser
{
    /* 4 look-ahead tokens */
    java_token tokens[4];
    /* num look-ahead available */
    size_t num_token_available;
} java_parser;

#endif
