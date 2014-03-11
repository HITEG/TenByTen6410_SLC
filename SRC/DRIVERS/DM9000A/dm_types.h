/**************************************************************************
 *
 * $Id: dm_types.h,v 1.1.1.1 2007/04/16 03:45:52 bill Exp $
 *
 * File: Types.h
 *
 * Copyright (c) 2000-2002 Davicom Inc.  All rights reserved.
 *
 *************************************************************************/
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;
typedef unsigned char *PU8;
typedef unsigned short *PU16;
typedef unsigned long *PU32;

#ifdef __cplusplus
extern "C"
{
#include <ndis.h>
}
#endif

#ifndef __MY_TEYPES_H__
#define __MY_TEYPES_H__

#define DIM(a)                          (sizeof(a) / sizeof(a[0]))
#define MINI(a,b)                       (((a) < (b)) ? (a) : (b))
#define MAXI(a,b)                       (((a) > (b)) ? (a) : (b))

#define MAKE_MASK1(a)                       (1 << (a))
#define MAKE_MASK2(a, b)                    (MAKE_MASK1(a) | MAKE_MASK1(b))
#define MAKE_MASK3(a, b, c)                 (MAKE_MASK1(a) | MAKE_MASK2(b, c))
#define MAKE_MASK4(a, b, c, d)              (MAKE_MASK2(a, b) | MAKE_MASK2(c, d))
#define MAKE_MASK5(a, b, c, d, e)           (MAKE_MASK2(a, b) | MAKE_MASK3(c, d, e))
#define MAKE_MASK6(a, b, c, d, e, f)        (MAKE_MASK3(a, b, c) | MAKE_MASK3(d, e, f))
#define MAKE_MASK7(a, b, c, d, e, f, g)     (MAKE_MASK3(a, b, c) | MAKE_MASK4(d, e, f, g))
#define MAKE_MASK8(a, b, c, d, e, f, g, h)  (MAKE_MASK4(a, b, c, d) | MAKE_MASK4(e, f, g, h))

#define HIGH_BYTE(n)                    (((n) >> 8) & 0xFF)
#define LOW_BYTE(n)                     (((n) & 0xFF))
#define HIGH_WORD(n)                    (((n) >> 16) & 0xFFFF)
#define LOW_WORD(n)                     (((n) & 0xFFFF))

#define MAKE_WORD(h, l)                 (((h) << 8) | ((l) & 0xff))
#define MAKE_DWORD(hh, hl, lh, ll)      ((MAKE_WORD(hh ,hl) << 16 ) | MAKE_WORD(lh, ll))
#endif // of __MY_TEYPES_H__
