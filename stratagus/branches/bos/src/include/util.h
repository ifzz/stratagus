//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name util.h - General utilities. */
//
//      (c) Copyright 1998-2005 by Lutz Sammer and Jimmy Salmon
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//      $Id$

#ifndef __UTIL_H__
#define __UTIL_H__

#include "SDL.h"


/*----------------------------------------------------------------------------
--  Random
----------------------------------------------------------------------------*/

extern unsigned SyncRandSeed;               /// Sync random seed value

extern void InitSyncRand(void);             /// Initialize the syncron rand
extern int SyncRand(void);                  /// Syncron rand

/*----------------------------------------------------------------------------
--  Math
----------------------------------------------------------------------------*/

	///  rand only used on this computer.
#define MyRand() rand()

	/// Compute a square root using ints
extern long isqrt(long num);

/*----------------------------------------------------------------------------
--  Strings
----------------------------------------------------------------------------*/

	/// strdup + strcat
extern char *strdcat(const char *l, const char *r);
	/// strdup + strcat + strcat
extern char *strdcat3(const char *l, const char *m, const char *r);

#if !defined(HAVE_STRCASESTR)
	/// case insensitive strstr
extern char *strcasestr(const char *str, const char *substr);
#endif // !HAVE_STRCASESTR

#endif /* __UTIL_H__ */