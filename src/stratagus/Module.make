##       _________ __                 __
##      /   _____//  |_____________ _/  |______     ____  __ __  ______
##      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
##      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ \
##     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
##             \/                  \/          \//_____/            \/
##  ______________________                           ______________________
##			  T H E   W A R   B E G I N S
##	   Stratagus - A free fantasy real time strategy game engine
##
##	Module.make	-	Module Makefile (included from Makefile).
##
##	(c) Copyright 2004 by The Stratagus Team
##	(c) Copyright 2010 by Pali Rohár
##
##      This program is free software; you can redistribute it and/or modify
##      it under the terms of the GNU General Public License as published by
##      the Free Software Foundation; only version 2 of the License.
##
##      This program is distributed in the hope that it will be useful,
##      but WITHOUT ANY WARRANTY; without even the implied warranty of
##      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##      GNU General Public License for more details.
##
##      You should have received a copy of the GNU General Public License
##      along with this program; if not, write to the Free Software
##      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

MODULE = src/stratagus
MSRC =   construct.cpp groups.cpp iolib.cpp mainloop.cpp missile.cpp \
         player.cpp script.cpp script_missile.cpp script_player.cpp \
         script_spell.cpp selection.cpp spells.cpp stratagus.cpp translate.cpp \
         util.cpp luacallback.cpp title.cpp

ifneq ($(findstring -DUSE_WIN32, $(CPPFLAGS)),)
	ifneq ($(findstring -DNO_STDIO_REDIRECT, $(CPPFLAGS)),)
		MSRC += attachconsole.cpp
	endif
endif

SRC +=   $(addprefix $(MODULE)/,$(MSRC))