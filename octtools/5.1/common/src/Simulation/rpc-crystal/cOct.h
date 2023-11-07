/*
 * cOct.h
 *
 * A header file for header information that needs to be in more
 * than one of the Crystal Oct reader source files.
 */

#include "port.h"
#include "oct.h"
#include "oh.h"
#include "utility.h"
#include "tr.h"
#include "st.h"
#include "errtrap.h"

#define COCT_PKG_NAME "Crystal Oct reader"

typedef struct netHash {
    char *name;
    octId parentId;
    int netType;
    float cap;
    float res;
} s_netHash;

/* net type will be one of these, or zero				*/
#define VDD_NET 0x1
#define GND_NET 0x2

/* bit values that cOctStatus uses					*/
#define COCT_ON 0x1
#define COCT_VERBOSE 0x2
#define COCT_PENDING_CRIT_UPDATE 0x4
#define COCT_TRACE 0x8
#define COCT_BUILD_DONE 0x10


/* bits to identify layers concisely					*/
#define COCT_NDIF_MASK 0x1
#define COCT_PDIF_MASK 0x2
#define COCT_POLY_MASK 0x4
#define COCT_MET1_MASK 0x8
#define COCT_MET2_MASK 0x10
