/*
 * rpc-crystal.h
 *
 * Header file for rpc-crystal
 */

#include "port.h"
#include "rpc.h"
#include "utility.h"
#include "oh.h"
#include "cOct.h"

#define RPC_CRYSTAL_BUILD_DONE 0x1

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct s_helper{
    int (* usageHelpFunct)();
} s_helper;
