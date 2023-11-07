/***********************************************************************
 * Tables for any table lookups that might be done
 * Filename: tables.c
 * Program: Musa - Multi-Level Simulator
 * Environment: Oct/Vem
 * Date: March, 1989
 * Professor: Richard Newton
 * Oct Tools Manager: Rick Spickelmier
 * Musa/Bdsim Original Design: Russell Segal
 * Organization:  University Of California, Berkeley
 *                Electronic Research Laboratory
 *                Berkeley, CA 94720
 * Routines:
 * Modifications:
 * Note: File is formatted with 4 space tab.
 ***********************************************************************/

#define TABLES_MUSA
#include "musa.h"

/*
 * Table to map drive level into 5 catagories from level numbers
 */
char drive_table[] = {
DRAIL, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE, DRIVE,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, WEAK,
WEAK, WEAK, WEAK, WEAK, WEAK, WEAK, CHARGED, NOTHING };

/*
 * Map a combo of high drive and low drive to composite node values
 * low three bits are low drive level.
 * high three bits are high drive level.
 */
char combo_table[] = {
L_ill, L_I, L_I,  L_I,  L_I, L_ill, L_ill, L_ill,
L_O,   L_X, L_1,  L_1,  L_1, L_ill, L_ill, L_ill,
L_O,   L_0, L_wX, L_w1, L_w1, L_ill, L_ill, L_ill,
L_O,   L_0, L_w0, L_X,  L_i, L_ill, L_ill, L_ill,
L_O,   L_0, L_w0, L_o,  L_x, L_ill, L_ill, L_ill,
L_ill, L_ill, L_ill, L_ill, L_ill, L_ill, L_ill, L_ill,
L_ill, L_ill, L_ill, L_ill, L_ill, L_ill, L_ill, L_ill,
L_ill, L_ill, L_ill, L_ill, L_ill, L_ill, L_ill, L_ill };

/* characters for composite node values */
char letter_table[] = {
'$', 'o', 'i', 'x', '#', '0', '1', 'X',
'#', '0', '1', 'X', '#', 'O', 'I', '.' }; 

/* Info for transistor networks */
struct network_struct network[] = {
"always off",	0x00000000, 0,
			-1, -1, -1,	-1, -1, -1,	/* 0 */
"always on",	0xffffffff, 0,
			-1, -1, -1,	-1, -1, -1,	/* 1 */
"transistor",	0xffff0000, 1,
			4, -1, -1,	5, -1, -1,	/* 2 */
"transistor",	0xffff0000, 1,
			4, -1, -1,	5, -1, -1,	/* 3 */
"(+ 1 2)",	0xffffff00, 2,
			6, 8, 12,	19, 27, 21,	/* 4 */
"(* 1 2)",	0xff000000, 2,
			18, 20, 26,	7, 13, 9,	/* 5 */
"(+ 1 2 3)",	0xfffffff0, 3,
			8, 10, 16,	31, 35, 33,	/* 6 */
"(* 1 2 3)",	0xf0000000, 3,
			30, 32, 34,	9, 17, 11,	/* 7 */
"(+ 1 2 3 4)",	0xfffffffc, 4,
			10, -1, -1,	37, -1, -1,	/* 8 */
"(* 1 2 3 4)",	0xc0000000, 4,
			36, -1, -1,	11, -1, -1,	/* 9 */
"(+ 1 2 3 4 5)",	0xfffffffe, 5,
			-1, -1, -1,	-1, -1, -1,	/* 10 */
"(* 1 2 3 4 5)",	0x80000000, 5,
			-1, -1, -1,	-1, -1, -1,	/* 11 */
"(+ 1 2 (* 3 4))",	0xffffffc0, 4,
			14, -1, -1,	39, -1, -1,	/* 12 */
"(* 1 2 (+ 3 4))",	0xfc000000, 4,
			38, -1, -1,	15, -1, -1,	/* 13 */
"(+ 1 2 (* 3 4) 5)",	0xffffffea, 5,
			-1, -1, -1,	-1, -1, -1,	/* 14 */
"(* 1 2 (+ 3 4) 5)",	0xa8000000, 5,
			-1, -1, -1,	-1, -1, -1,	/* 15 */
"(+ 1 2 3 (* 4 5))",	0xfffffff8, 5,
			-1, -1, -1,	-1, -1, -1,	/* 16 */
"(* 1 2 3 (+ 4 5))",	0xe0000000, 5,
			-1, -1, -1,	-1, -1, -1,	/* 17 */
"(+ (* 1 2) 3)",	0xfff0f0f0, 3,
			20, 22, 24,	41, 45, 43,	/* 18 */
"(* (+ 1 2) 3)",	0xf0f0f000, 3,
			40, 42, 44,	21, 25, 23,	/* 19 */
"(+ (* 1 2) 3 4)",	0xfffcfcfc, 4,
			22, -1, -1,	47, -1, -1,	/* 20 */
"(* (+ 1 2) 3 4)",	0xc0c0c000, 4,
			46, -1, -1,	23, -1, -1,	/* 21 */
"(+ (* 1 2) 3 4 5)",	0xfffefefe, 5,
			-1, -1, -1,	-1, -1, -1,	/* 22 */
"(* (+ 1 2) 3 4 5)",	0x80808000, 5,
			-1, -1, -1,	-1, -1, -1,	/* 23 */
"(+ (* 1 2) 3 (* 4 5))",	0xfff8f8f8, 5,
			-1, -1, -1,	-1, -1, -1,	/* 24 */
"(* (+ 1 2) 3 (+ 4 5))",	0xe0e0e000, 5,
			-1, -1, -1,	-1, -1, -1,	/* 25 */
"(+ (* 1 2) (* 3 4))",	0xffc0c0c0, 4,
			28, -1, -1,	49, -1, -1,	/* 26 */
"(* (+ 1 2) (+ 3 4))",	0xfcfcfc00, 4,
			48, -1, -1,	29, -1, -1,	/* 27 */
"(+ (* 1 2) (* 3 4) 5)",	0xffeaeaea, 5,
			-1, -1, -1,	-1, -1, -1,	/* 28 */
"(* (+ 1 2) (+ 3 4) 5)",	0xa8a8a800, 5,
			-1, -1, -1,	-1, -1, -1,	/* 29 */
"(+ (* 1 2 3) 4)",	0xfccccccc, 4,
			32, -1, -1,	51, -1, -1,	/* 30 */
"(* (+ 1 2 3) 4)",	0xccccccc0, 4,
			50, -1, -1,	33, -1, -1,	/* 31 */
"(+ (* 1 2 3) 4 5)",	0xfeeeeeee, 5,
			-1, -1, -1,	-1, -1, -1,	/* 32 */
"(* (+ 1 2 3) 4 5)",	0x88888880, 5,
			-1, -1, -1,	-1, -1, -1,	/* 33 */
"(+ (* 1 2 3) (* 4 5))",	0xf8888888, 5,
			-1, -1, -1,	-1, -1, -1,	/* 34 */
"(* (+ 1 2 3) (+ 4 5))",	0xeeeeeee0, 5,
			-1, -1, -1,	-1, -1, -1,	/* 35 */
"(+ (* 1 2 3 4) 5)",	0xeaaaaaaa, 5,
			-1, -1, -1,	-1, -1, -1,	/* 36 */
"(* (+ 1 2 3 4) 5)",	0xaaaaaaa8, 5,
			-1, -1, -1,	-1, -1, -1,	/* 37 */
"(+ (* 1 2 (+ 3 4)) 5)",	0xfeaaaaaa, 5,
			-1, -1, -1,	-1, -1, -1,	/* 38 */
"(* (+ 1 2 (* 3 4)) 5)",	0xaaaaaa80, 5,
			-1, -1, -1,	-1, -1, -1,	/* 39 */
"(+ (* (+ 1 2) 3) 4)",	0xfcfcfccc, 4,
			42, -1, -1,	53, -1, -1,	/* 40 */
"(* (+ (* 1 2) 3) 4)",	0xccc0c0c0, 4,
			52, -1, -1,	43, -1, -1,	/* 41 */
"(+ (* (+ 1 2) 3) 4 5)",	0xfefefeee, 5,
			-1, -1, -1,	-1, -1, -1,	/* 42 */
"(* (+ (* 1 2) 3) 4 5)",	0x88808080, 5,
			-1, -1, -1,	-1, -1, -1,	/* 43 */
"(+ (* (+ 1 2) 3) (* 4 5))",	0xf8f8f888, 5,
			-1, -1, -1,	-1, -1, -1,	/* 44 */
"(* (+ (* 1 2) 3) (+ 4 5))",	0xeee0e0e0, 5,
			-1, -1, -1,	-1, -1, -1,	/* 45 */
"(+ (* (+ 1 2) 3 4) 5)",	0xeaeaeaaa, 5,
			-1, -1, -1,	-1, -1, -1,	/* 46 */
"(* (+ (* 1 2) 3 4) 5)",	0xaaa8a8a8, 5,
			-1, -1, -1,	-1, -1, -1,	/* 47 */
"(+ (* (+ 1 2) (+ 3 4)) 5)",	0xfefefeaa, 5,
			-1, -1, -1,	-1, -1, -1,	/* 48 */
"(* (+ (* 1 2) (* 3 4)) 5)",	0xaa808080, 5,
			-1, -1, -1,	-1, -1, -1,	/* 49 */
"(+ (* (+ 1 2 3) 4) 5)",	0xeeeeeeea, 5,
			-1, -1, -1,	-1, -1, -1,	/* 50 */
"(* (+ (* 1 2 3) 4) 5)",	0xa8888888, 5,
			-1, -1, -1,	-1, -1, -1,	/* 51 */
"(+ (* (+ (* 1 2) 3) 4) 5)",	0xeeeaeaea, 5,
			-1, -1, -1,	-1, -1, -1,	/* 52 */
"(* (+ (* (+ 1 2) 3) 4) 5)",	0xa8a8a888, 5,
			-1, -1, -1,	-1, -1, -1,	/* 53 */
} ;
