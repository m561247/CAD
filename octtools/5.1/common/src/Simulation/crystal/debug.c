/* *************************************************************** */
/* Fred W. Obermeier (c)                          Created 04/18/88 */
/* *************************************************************** */
/* Copyright (c) 04/18/88  Fred W. Obermeier, All rights reserved. */
/* *************************************************************** */
/* FILE: debug.c                                                   */
/* SCCS: %Z%    %M%     Version %I%     updated_%G% */
/* *************************************************************** */
/* WARNING:  Under development.                                    */
/* WARNING:  Do not sell without author's consent.                 */
/* *************************************************************** */
#define DEBUG_C

#include "crystal.h"

/* A few structure that a user can manipulate inside dbx without */
/* changing crystal's own data structures.*/

Node	*debug_node;
FET	*debug_fet;
Flow	*debug_flow;
Stage	*debug_stage;
Type	*debug_type;
