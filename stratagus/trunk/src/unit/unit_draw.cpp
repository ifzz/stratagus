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
/**@name unit_draw.cpp - The draw routines for units. */
//
//      (c) Copyright 1998-2005 by Lutz Sammer, Jimmy Salmon, Nehal Mistry
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

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "video.h"
#include "sound_id.h"
#include "unitsound.h"
#include "editor.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "tileset.h"
#include "map.h"
#include "construct.h"
#include "cursor.h"
#include "interface.h"
#include "font.h"
#include "ui.h"
#include "script.h"

/*----------------------------------------------------------------------------
--  Definitions
----------------------------------------------------------------------------*/
#ifndef min
#ifdef __GNUC__
#define min(a,b) ({ typeof(a) _a = a; typeof(b) _b = b; _a < _b ? _a : _b; })
#else
#define min min
static inline min(int a, int b) { return a < b ? a : b; }
#endif
#endif

#ifndef max
#ifdef __GNUC__
#define max(a,b) ({ typeof(a) _a = a; typeof(b) _b = b; _a > _b ? _a : _b; })
#else
#define max max
static inline max(int a, int b) { return a > b ? a : b; }
#endif
#endif


/*----------------------------------------------------------------------------
-- Variables
----------------------------------------------------------------------------*/

/**
** Decoration: health, mana.
*/
typedef struct {
	char* File;       /// File containing the graphics data
	int HotX;         /// X drawing position (relative)
	int HotY;         /// Y drawing position (relative)
	int Width;        /// width of the decoration
	int Height;       /// height of the decoration

// --- FILLED UP ---
	Graphic* Sprite;  /// loaded sprite images
} Decoration;


/**
**	Structure grouping all Sprites for decoration.
*/
typedef struct {
	char** Name;             /// Name of the sprite.
	Decoration* SpriteArray; /// Sprite to display variable.
	int SpriteNumber;        /// Size of SpriteArray (same as size of SriteName).
} DecoSpriteType;

static DecoSpriteType DecoSprite; /// All sprite's infos.

int ShowSightRange;               /// Flag: show right range
int ShowReactionRange;            /// Flag: show reaction range
int ShowAttackRange;              /// Flag: show attack range
int ShowOrders;                   /// Flag: show orders of unit on map
unsigned long ShowOrdersCount;    /// Show orders for some time



// FIXME: not all variables of this file are here
// FIXME: perhaps split this file into two or three parts?

/**
**  Show that units are selected.
**
**  @param color    FIXME
**  @param x1,y1    Coordinates of the top left corner.
**  @param x2,y2    Coordinates of the bottom right corner.
*/
void (*DrawSelection)(Uint32 color, int x1, int y1,
	int x2, int y2) = DrawSelectionNone;

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

DrawDecoFunc DrawBar;
DrawDecoFunc PrintValue;
DrawDecoFunc DrawSpriteBar;
DrawDecoFunc DrawStaticSprite;

// FIXME: clean split screen support
// FIXME: integrate this with global versions of these functions in map.c

const Viewport* CurrentViewport;  /// FIXME: quick hack for split screen

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

/**
**  Show selection marker around an unit.
**
**  @param unit  Pointer to unit.
*/
void DrawUnitSelection(const Unit* unit)
{
	int x;
	int y;
	UnitType* type;
	Uint32 color;

	type = unit->Type;

	// FIXME: make these colors customizable with scripts.

	if (EditorRunning && unit == UnitUnderCursor &&
			EditorState == EditorSelecting) {
		color = ColorWhite;
	} else if (unit->Selected || unit->TeamSelected || (unit->Blink & 1)) {
		if (unit->Player->Index == PlayerNumNeutral) {
			color = ColorYellow;
		} else if ((unit->Selected || (unit->Blink & 1)) &&
				(unit->Player == ThisPlayer ||
					PlayersTeamed(ThisPlayer->Index, unit->Player->Index))) {
			color = ColorGreen;
		} else if (IsEnemy(ThisPlayer, unit)) {
			color = ColorRed;
		} else {
			int i;

			for (i = 0; i < PlayerMax; ++i) {
				if (unit->TeamSelected & (1 << i)) {
					break;
				}
			}
			if (i == PlayerMax) {
				color = unit->Player->Color;
			} else {
				color = Players[i].Color;
			}
		}
	} else if (CursorBuilding && unit->Type->Building &&
			unit->Orders[0].Action != UnitActionDie &&
			(unit->Player == ThisPlayer ||
				PlayersTeamed(ThisPlayer->Index, unit->Player->Index))) {
		// If building mark all own buildings
		color = ColorGray;
	} else {
		return;
	}

	x = Map2ViewportX(CurrentViewport, unit->X) + unit->IX +
		type->TileWidth * TileSizeX / 2 - type->BoxWidth / 2 -
		(type->Width - type->Sprite->Width) / 2;
	y = Map2ViewportY(CurrentViewport, unit->Y) + unit->IY +
		type->TileHeight * TileSizeY / 2 - type->BoxHeight/2 -
		(type->Height - type->Sprite->Height) / 2;
	DrawSelection(color, x, y, x + type->BoxWidth, y + type->BoxHeight);
}

/**
**  Don't show selected units.
**
**  @param color  Color to draw, nothing in this case.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionNone(Uint32 color, int x1, int y1,
	int x2, int y2)
{
}

/**
**  Show selected units with circle.
**
**  @param color  Color to draw circle
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionCircle(Uint32 color, int x1, int y1,
	int x2, int y2)
{
	VideoDrawCircleClip(color, (x1 + x2) / 2, (y1 + y2) / 2,
		min((x2 - x1) / 2, (y2 - y1) / 2) + 2);
}

/**
**  Show selected units with circle.
**
**  @param color  Color to draw and fill circle.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionCircleWithTrans(Uint32 color, int x1, int y1,
	int x2, int y2)
{
	VideoFillTransCircleClip(color, (x1 + x2) / 2, (y1 + y2) / 2,
		min((x2 - x1) / 2, (y2 - y1) / 2), 95);
	VideoDrawCircleClip(color, (x1 + x2) / 2, (y1 + y2) / 2,
		min((x2 - x1) / 2, (y2 - y1) / 2));
}

/**
**  Draw selected rectangle around the unit.
**
**  @param color  Color to draw rectangle.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionRectangle(Uint32 color, int x1, int y1,
	int x2, int y2)
{
	VideoDrawRectangleClip(color, x1, y1, x2 - x1, y2 - y1);
}

/**
**  Draw selected rectangle around the unit.
**
**  @param color  Color to draw and fill rectangle.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionRectangleWithTrans(Uint32 color, int x1, int y1,
	int x2, int y2)
{
	VideoDrawRectangleClip(color, x1, y1, x2 - x1, y2 - y1);
	VideoFillTransRectangleClip(color, x1 + 1, y1 + 1,
		x2 - x1 - 2, y2 - y1 - 2, 75);
}

/**
**  Draw selected corners around the unit.
**
**  @param color  Color to draw corners.
**  @param x1,y1  Coordinates of the top left corner.
**  @param x2,y2  Coordinates of the bottom right corner.
*/
void DrawSelectionCorners(Uint32 color, int x1, int y1,
	int x2, int y2)
{
#define CORNER_PIXELS 6

	VideoDrawVLineClip(color, x1, y1, CORNER_PIXELS);
	VideoDrawHLineClip(color, x1 + 1, y1, CORNER_PIXELS - 1);

	VideoDrawVLineClip(color, x2, y1, CORNER_PIXELS);
	VideoDrawHLineClip(color, x2 - CORNER_PIXELS + 1, y1, CORNER_PIXELS - 1);

	VideoDrawVLineClip(color, x1, y2 - CORNER_PIXELS + 1, CORNER_PIXELS);
	VideoDrawHLineClip(color, x1, y2, CORNER_PIXELS - 1);

	VideoDrawVLineClip(color, x2, y2 - CORNER_PIXELS + 1, CORNER_PIXELS);
	VideoDrawHLineClip(color, x2 - CORNER_PIXELS + 1, y2, CORNER_PIXELS - 1);
}


/**
**  Return the index of the sprite named SpriteName.
**
**  @param SpriteName    Name of the sprite.
**
**  @return              Index of the sprite. -1 if not found.
*/
int GetSpriteIndex(const char* SpriteName)
{
	int i;

	Assert(SpriteName);
	for (i = 0; i < DecoSprite.SpriteNumber; i++) {
		if (!strcmp(SpriteName, DecoSprite.Name[i])) {
			return i;
		}
	}
	return -1;
}

/**
**  Define the sprite to show variables.
**
**  @param l    Lua_state
*/
static int CclDefineSprites(lua_State* l)
{
	Decoration deco;      // temp decoration to stock arguments.
	const char* name;     // name of the current sprite.
	int args;             // number of arguments.
	int i;                // iterator on argument.
	const char* key;      // Current key of the lua table.
	int index;            // Index of the Sprite.

	args = lua_gettop(l);
	for (i = 0; i < args; ++i) {
		lua_pushnil(l);

		name = 0;
		memset(&deco, 0, sizeof(deco));
		while (lua_next(l, i + 1)) {
			key = LuaToString(l, -2); // key name
			if (!strcmp(key, "Name")) {
				name = LuaToString(l, -1);
			} else if (!strcmp(key, "File")) {
				deco.File = strdup(LuaToString(l, -1));
			} else if (!strcmp(key, "Offset")) {
				if (!lua_istable(l, -1) || luaL_getn(l, -1) != 2) {
					LuaError(l, "incorrect argument");
				}
				lua_rawgeti(l, -1, 1); // offsetX
				lua_rawgeti(l, -2, 2); // offsetY
				deco.HotX = LuaToNumber(l, -2);
				deco.HotY = LuaToNumber(l, -1);
				lua_pop(l, 2); // Pop offsetX and Y
			} else if (!strcmp(key, "Size")) {
				if (!lua_istable(l, -1) || luaL_getn(l, -1) != 2) {
					LuaError(l, "incorrect argument");
				}
				lua_rawgeti(l, -1, 1); // Width
				lua_rawgeti(l, -2, 2); // Height
				deco.Width = LuaToNumber(l, -2);
				deco.Height = LuaToNumber(l, -1);
				lua_pop(l, 2); // Pop Width and Height
			} else { // Error.
				LuaError(l, "incorrect field '%s' for the DefineSprite." _C_ key);
			}
			lua_pop(l, 1); // pop the value;
		}
		if (name == 0) {
			LuaError(l, "CclDefineSprites requires the Name flag for sprite.");
		}
		index = GetSpriteIndex(name);
		if (index == -1) { // new sprite.
			index = DecoSprite.SpriteNumber++;
			DecoSprite.Name = (char**)realloc(DecoSprite.Name,
				DecoSprite.SpriteNumber * sizeof(*DecoSprite.Name));
			DecoSprite.Name[index] = strdup(name);
			DecoSprite.SpriteArray = (Decoration*)realloc(DecoSprite.SpriteArray,
				DecoSprite.SpriteNumber * sizeof(*DecoSprite.SpriteArray));
			memset(DecoSprite.SpriteArray + index, 0, sizeof(*DecoSprite.SpriteArray));
		}
		free(DecoSprite.SpriteArray[index].File);
		memcpy(DecoSprite.SpriteArray + index, &deco, sizeof(*DecoSprite.SpriteArray));
		// Now verify validity.
		if (DecoSprite.SpriteArray[index].File == 0) {
			LuaError(l, "CclDefineSprites requires the Filen flag for sprite.");
		}
		// FIXME check if file is valid with good size ?
	}
	return 0;
}

/**
**  Define mana sprite.
**  Equivalent of
**  DefineSprites({Name = "sprite-mana",
**    File = file, Offset = {hotx, hoty}, Size = {w, h}})
**
**  @param l  Lua state
*/
static int CclManaSprite(lua_State* l)
{
	char buffer[1024]; // lua equivalent.

	LuaCheckArgs(l, 5);

	sprintf(buffer, "DefineSprites({Name = \"%s\", File = \"%s\", "
		"Offset = {%i, %i}, Size = {%i, %i}})",
		"sprite-mana", LuaToString(l, 1),
		(int) LuaToNumber(l, 2), (int) LuaToNumber(l, 3),
		(int) LuaToNumber(l, 4), (int) LuaToNumber(l, 5));

	DebugPrint("instead of ManaSprite, you should write instead :\n%s" _C_ buffer);

	CclCommand(buffer);
	return 0;
}

/**
**  Define health sprite.
**
**  @param l  Lua state
*/
static int CclHealthSprite(lua_State* l)
{
	char buffer[1024]; // lua equivalent.

	LuaCheckArgs(l, 5);

	sprintf(buffer, "DefineSprites({Name = \"%s\", File = \"%s\", "
		"Offset = {%i, %i}, Size = {%i, %i}})",
		"sprite-health", LuaToString(l, 1),
		(int) LuaToNumber(l, 2), (int) LuaToNumber(l, 3),
		(int )LuaToNumber(l, 4), (int) LuaToNumber(l, 5));

	DebugPrint("instead of HealthSprite, you should write instead :\n%s" _C_ buffer);

	CclCommand(buffer);
	return 0;
}

#if 1 // To be deleted ? DefineDecoration do that.

/**
**  Enable display health as health-sprite.
**  Equivalent of
**  DefineDecorations({Index = "HitPoints", HideNeutral = true, CenterX = true,
**    OffsetPercent = {50, 100}, Offset = {0, -7},
**    Method = {"sprite", {1}}})
**
**  @param l  Lua state
*/
static int CclShowHealthDot(lua_State* l)
{
	const char* lua_equiv = "DefineDecorations({Index = \"HitPoints\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100}, Method = {\"sprite\", {\"sprite-health\"}}})";

	LuaCheckArgs(l, 0);

	DebugPrint("instead of ShowHealthDot, you should write instead :\n%s\n" _C_ lua_equiv);

	CclCommand(lua_equiv);
	return 0;
}

/**
**  Enable display health as health-bar.
**  Equivalent of
**  DefineDecorations({Index = "HitPoints", HideNeutral = true, CenterX = true,
**    OffsetPercent = {50, 100}, Offset = {0, -7},
**    Method = {"bar", {Width = 3, BorderSize = 1}}})
**
**  @param l  Lua state
*/
static int CclShowHealthHorizontal(lua_State* l)
{
	const char* lua_equiv = "DefineDecorations({Index = \"HitPoints\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100}, Offset = {0, -7},"
		"Method = {\"bar\", {Width = 3, BorderSize = 1}}})";

	LuaCheckArgs(l, 0);

	DebugPrint("instead of ShowHealthHorizontal, you should write instead :\n%s\n" _C_ lua_equiv);

	CclCommand(lua_equiv);
	return 0;
}

/**
**  Enable display health as health-bar.
**  Equivalent of
**  DefineDecorations({Index = "HitPoints", HideNeutral = true,
**    Offset = {-7, 0},
**    Method = {"bar", {Width = 3, BorderSize = 1, Orientation = "vertical"}}})
**
**  @param l  Lua state
*/
static int CclShowHealthVertical(lua_State* l)
{
	const char* lua_equiv = "DefineDecorations({Index = \"HitPoints\", HideNeutral = true,"
		"Offset = {-7, 0},"
		"Method = {\"bar\", {Width = 3, BorderSize = 1, Orientation = \"vertical\"}}})";

	LuaCheckArgs(l, 0);

	DebugPrint("instead of ShowHealthVertical, you should write instead :\n%s\n" _C_ lua_equiv);

	CclCommand(lua_equiv);
	return 0;
}

/**
**  Enable display health as health-bar.
**
**  Equivalent of ShowHealthVertical()
**
**  @param l  Lua state
*/
static int CclShowHealthBar(lua_State* l)
{
	return CclShowHealthVertical(l);
}

/**
**  Enable display mana as mana-sprite.
**
**  Equivalent of
**  DefineDecorations({Index = "Mana", HideNeutral = true, CenterX = true,
**    OffsetPercent = {50, 100},
**    Method = {"sprite", {0}}})
**  For index Mana, Transport, Research, Training, UpgradeTo, Resource.
**  Except for ressource which have HideNeutral = false.
**
**  @param l  Lua state
*/
static int CclShowManaDot(lua_State* l)
{
	char lua_equiv[] = "DefineDecorations({Index = \"Mana\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100},Method = {\"sprite\", {\"sprite-mana\"}}})\n"
		"DefineDecorations({Index = \"Transport\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100},Method = {\"sprite\", {\"sprite-mana\"}}})\n"
		"DefineDecorations({Index = \"Research\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100},Method = {\"sprite\", {\"sprite-mana\"}}})\n"
		"DefineDecorations({Index = \"Training\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100},Method = {\"sprite\", {\"sprite-mana\"}}})\n"
		"DefineDecorations({Index = \"UpgradeTo\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100},Method = {\"sprite\", {\"sprite-mana\"}}})\n"
		"DefineDecorations({Index = \"GiveResource\", HideNeutral = false, CenterX = true,"
		"OffsetPercent = {50, 100},Method = {\"sprite\", {\"sprite-mana\"}}})\n"
		"DefineDecorations({Index = \"CarryResource\", HideNeutral = false, CenterX = true,"
		"OffsetPercent = {50, 100},Method = {\"sprite\", {\"sprite-mana\"}}})\n";


	LuaCheckArgs(l, 0);

	DebugPrint("instead of ShowManaDot, you should write instead :\n%s\n" _C_ lua_equiv);

	CclCommand(lua_equiv);
	return 0;
}

/**
**  Enable display mana as horizontal bar.
**
**  Equivalent of
**  DefineDecorations({Index = "Mana", HideNeutral = true, CenterX = true,
**    OffsetPercent = {50, 100},
**    Method = {"bar", {Width = 3, BorderSize = 1}}})
**  For index Mana, Transport, Research, Training, UpgradeTo, Resource.
**
**  @param l  Lua state
*/
static int CclShowManaHorizontal(lua_State* l)
{
	char lua_equiv[] = "DefineDecorations({Index = \"Mana\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100}, Method = {\"bar\", {Width = 3, BorderSize = 1}}})\n"
		"DefineDecorations({Index = \"Transport\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100}, Method = {\"bar\", {Width = 3, BorderSize = 1}}})\n"
		"DefineDecorations({Index = \"Research\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100}, Method = {\"bar\", {Width = 3, BorderSize = 1}}})\n"
		"DefineDecorations({Index = \"Training\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100}, Method = {\"bar\", {Width = 3, BorderSize = 1}}})\n"
		"DefineDecorations({Index = \"UpgradeTo\", HideNeutral = true, CenterX = true,"
		"OffsetPercent = {50, 100}, Method = {\"bar\", {Width = 3, BorderSize = 1}}})\n"
		"DefineDecorations({Index = \"GiveResource\", HideNeutral = false, CenterX = true,"
		"OffsetPercent = {50, 100}, Method = {\"bar\", {Width = 3, BorderSize = 1}}})\n"
		"DefineDecorations({Index = \"CarryResource\", HideNeutral = false, CenterX = true,"
		"OffsetPercent = {50, 100}, Method = {\"bar\", {Width = 3, BorderSize = 1}}})\n";


	LuaCheckArgs(l, 0);

	DebugPrint("instead of ShowManaHorizontal, you should write instead :\n%s\n" _C_ lua_equiv);

	CclCommand(lua_equiv);
	return 0;
}

/**
**  Enable display mana as vertical bar.
**
**  Equivalent of
**  DefineDecorations({Index = "Mana", HideNeutral = true,
**    Method = {"bar", {Width = 3, BorderSize = 1, Orientation = "vertical"}}})
**  For index Mana, Transport, Research, Training, UpgradeTo, Resource.
**
**  @param l  Lua state
*/
static int CclShowManaVertical(lua_State* l)
{
	char lua_equiv[] = "DefineDecorations({Index = \"Mana\", HideNeutral = true"
		"Method = {\"bar\", {Width = 3, BorderSize = 1, Orientation = \"vertical\"}}})\n"
		"DefineDecorations({Index = \"Transport\", HideNeutral = true,"
		"Method = {\"bar\", {Width = 3, BorderSize = 1, Orientation = \"vertical\"}}})\n"
		"DefineDecorations({Index = \"Research\", HideNeutral = true,"
		"Method = {\"bar\", {Width = 3, BorderSize = 1, Orientation = \"vertical\"}}})\n"
		"DefineDecorations({Index = \"Training\", HideNeutral = true,"
		"Method = {\"bar\", {Width = 3, BorderSize = 1, Orientation = \"vertical\"}}})\n"
		"DefineDecorations({Index = \"UpgradeTo\", HideNeutral = true,"
		"Method = {\"bar\", {Width = 3, BorderSize = 1, Orientation = \"vertical\"}}})\n"
		"DefineDecorations({Index = \"GiveResource\", HideNeutral = false,"
		"Method = {\"bar\", {Width = 3, BorderSize = 1, Orientation = \"vertical\"}}})\n"
		"DefineDecorations({Index = \"CarryResource\", HideNeutral = false,"
		"Method = {\"bar\", {Width = 3, BorderSize = 1, Orientation = \"vertical\"}}})\n";


	LuaCheckArgs(l, 0);

	DebugPrint("instead of ShowManaVertical, you should write instead :\n%s\n" _C_ lua_equiv);

	CclCommand(lua_equiv);
	return 0;
}

/**
**  Enable display mana as mana-bar.
**
**  Equivalent of ShowManaVertical()
**
**  @param l  Lua state
*/
static int CclShowManaBar(lua_State* l)
{
	return CclShowManaVertical(l);
}

/**
**  Enable energy bars and dots only for selected units
**
**  @param l  Lua state
*/
static int CclShowEnergySelected(lua_State* l)
{
	int i;

	LuaCheckArgs(l, 0);
	for (i = 0; i < UnitTypeVar.NumberDeco; ++i) {
		UnitTypeVar.DecoVar[i].ShowOnlySelected = true;
	}
	return 0;
}

/**
**  Enable display of full bars/dots.
**
**  @param l  Lua state
*/
static int CclShowFull(lua_State* l)
{
	int i;

	LuaCheckArgs(l, 0);
	for (i = 0; i < UnitTypeVar.NumberDeco; i++) {
		UnitTypeVar.DecoVar[i].ShowWhenMax = 1;
	}
	return 0;
}

/**
**  Disable display of full bars/dots.
**
**  @param l  Lua state
*/
static int CclShowNoFull(lua_State* l)
{
	int i;

	LuaCheckArgs(l, 0);
	for (i = 0; i < UnitTypeVar.NumberDeco; i++) {
		UnitTypeVar.DecoVar[i].ShowWhenMax = 0;
	}
	return 0;
}

/**
**  Draw decorations always on top.
**
**  @param l  Lua state
*/
static int CclDecorationOnTop(lua_State* l)
{
	LuaCheckArgs(l, 0);
	// FIXME: not implemented
	return 0;
}

#endif

/**
**  Register CCL features for decorations.
*/
void DecorationCclRegister(void)
{
	memset(&DecoSprite, 0, sizeof(DecoSprite));

	lua_register(Lua, "DefineSprites", CclDefineSprites);
#if 1 // to be removed.
	lua_register(Lua, "ManaSprite", CclManaSprite);
	lua_register(Lua, "HealthSprite", CclHealthSprite);
#endif

#if 1 // To be deleted ? alredy Replaced by DefineDecoration
	// These fonctions ara aliases to DefineDecorations
	lua_register(Lua, "ShowHealthBar", CclShowHealthBar);
	lua_register(Lua, "ShowHealthDot", CclShowHealthDot);
// adicionado por protoman
	lua_register(Lua, "ShowHealthVertical", CclShowHealthVertical);
	lua_register(Lua, "ShowHealthHorizontal", CclShowHealthHorizontal);
	lua_register(Lua, "ShowManaVertical", CclShowManaVertical);
	lua_register(Lua, "ShowManaHorizontal", CclShowManaHorizontal);
// fim

	lua_register(Lua, "ShowManaBar", CclShowManaBar);
	lua_register(Lua, "ShowManaDot", CclShowManaDot);
	lua_register(Lua, "ShowEnergySelectedOnly", CclShowEnergySelected);
	lua_register(Lua, "ShowFull", CclShowFull);
	lua_register(Lua, "ShowNoFull", CclShowNoFull);
	lua_register(Lua, "DecorationOnTop", CclDecorationOnTop);
#endif
}

/**
**  Load decoration.
*/
void LoadDecorations(void)
{
	int i;             // iterator on sprite decoration.
	Decoration* deco;  // current decoration

	for (i = 0; i < DecoSprite.SpriteNumber; i++) {
		deco = &DecoSprite.SpriteArray[i];
		ShowLoadProgress("Decorations `%s'", deco->File);
		deco->Sprite = NewGraphic(deco->File, deco->Width, deco->Height);
		deco->Sprite->Load();
	}
}

/**
**  Clean decorations.
*/
void CleanDecorations(void)
{
	int i;             // iterator on sprite decoration.
	Decoration* deco;  // current decoration

	for (i = 0; i < DecoSprite.SpriteNumber; i++) {
		deco = &DecoSprite.SpriteArray[i];
		free(DecoSprite.Name[i]);
		free(deco->File);
		FreeGraphic(deco->Sprite);
	}

	free(DecoSprite.SpriteArray);
	free(DecoSprite.Name);
	memset(&DecoSprite, 0, sizeof(DecoSprite));
}

/**
**  Draw bar for variables.
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**  @param Deco    More data arguments
**  @todo fix color configuration.
*/
void DrawBar(int x, int y, const Unit* unit, const DecoVarType* Deco)
{
	int height;
	int width;
	int h;
	int w;
	char b;        // BorderSize.
	Uint32 bcolor; // Border color.
	Uint32 color;  // inseide color.
	int f;         // 100 * value / max.

	Assert(unit);
	Assert(Deco);
	Assert(unit->Type);
	Assert(unit->Variable[Deco->Index].Max);
	height = Deco->Data.Bar.Height;
	if (height == 0) { // Default value
		height = unit->Type->BoxHeight; // Better size ? {,Box, Tile}
	}
	width = Deco->Data.Bar.Width;
	if (width == 0) { // Default value
		width = unit->Type->BoxWidth; // Better size ? {,Box, Tile}
	}
	if (Deco->Data.Bar.IsVertical)  { // Vertical
		w = width;
		h = unit->Variable[Deco->Index].Value * height / unit->Variable[Deco->Index].Max;
	} else {
		w = unit->Variable[Deco->Index].Value * width / unit->Variable[Deco->Index].Max;
		h = height;
	}

	if (Deco->IsCenteredInX) {
		x -= w / 2;
	}
	if (Deco->IsCenteredInY) {
		y -= h / 2;
	}

	b = Deco->Data.Bar.BorderSize;
	// Could depend of (value / max)
	f = unit->Variable[Deco->Index].Value * 100 / unit->Variable[Deco->Index].Max;
	bcolor = ColorBlack; // Deco->Data.Bar.BColor
	color = f > 50 ? (f > 75 ? ColorGreen : ColorYellow) : (f > 25 ? ColorOrange : ColorRed);
	// Deco->Data.Bar.Color
	if (b) {
		if (Deco->Data.Bar.ShowFullBackground) {
			VideoFillRectangleClip(bcolor, x - b, y - b, 2 * b + width, 2 * b + height);
		} else {
			if (Deco->Data.Bar.SEToNW) {
				VideoFillRectangleClip(bcolor, x - b - w + width, y - b - h + height,
					2 * b + w, 2 * b + h);
			} else {
				VideoFillRectangleClip(bcolor, x - b, y - b, 2 * b + w, 2 * b + h);
			}
		}
	}
	if (Deco->Data.Bar.SEToNW) {
		VideoFillRectangleClip(color, x - w + width, y - h + height, w, h);
	} else {
		VideoFillRectangleClip(color, x, y, w, h);
	}
}

/**
**  Print variable values (and max....).
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**  @param Deco    More data arguments
**  @todo fix font/color configuration.
*/
void PrintValue(int x, int y, const Unit* unit, const DecoVarType* Deco)
{
	int font;  // font to display the value.

	font = Deco->Data.Text.Font;
	if (Deco->IsCenteredInX) {
		x -= 2; // VideoTextLength(GameFont, buf) / 2, with buf = str(Value)
	}
	if (Deco->IsCenteredInY) {
		y -= VideoTextHeight(font) / 2;
	}
	VideoDrawNumberClip(x, y, font, unit->Variable[Deco->Index].Value);
}

/**
**  Draw a sprite with is like a bar (several stages)
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**  @param Deco    More data arguments
**  @todo fix sprite configuration.
*/
void DrawSpriteBar(int x, int y, const Unit* unit, const DecoVarType* Deco)
{
	int n;                   // frame of the sprite to show.
	Graphic* sprite;         // the sprite to show.
	Decoration* decosprite;  // Info on the sprite.

	Assert(unit);
	Assert(Deco);
	Assert(unit->Variable[Deco->Index].Max);
	Assert(Deco->Data.SpriteBar.NSprite != -1);

	decosprite = &DecoSprite.SpriteArray[(int) Deco->Data.SpriteBar.NSprite];
	sprite = decosprite->Sprite;
	x += decosprite->HotX; // in addition of OffsetX... Usefull ?
	y += decosprite->HotY; // in addition of OffsetY... Usefull ?

	n = sprite->NumFrames - 1;
	n -= (n * unit->Variable[Deco->Index].Value) / unit->Variable[Deco->Index].Max;

	if (Deco->IsCenteredInX) {
		x -= sprite->Width / 2;
	}
	if (Deco->IsCenteredInY) {
		y -= sprite->Height / 2;
	}
	sprite->DrawFrameClip(n, x, y);
}

/**
**  Draw a static sprite.
**
**  @param x       X screen pixel position
**  @param y       Y screen pixel position
**  @param unit    Unit pointer
**  @param Deco    More data arguments
**
**  @todo fix sprite configuration configuration.
*/
void DrawStaticSprite(int x, int y, const Unit* unit, const DecoVarType* Deco)
{
	Graphic* sprite;         // the sprite to show.
	Decoration* decosprite;  // Info on the sprite.

	decosprite = &DecoSprite.SpriteArray[(int) Deco->Data.StaticSprite.NSprite];
	sprite = decosprite->Sprite;
	x += decosprite->HotX; // in addition of OffsetX... Usefull ?
	y += decosprite->HotY; // in addition of OffsetY... Usefull ?
	if (Deco->IsCenteredInX) {
		x -= sprite->Width / 2;
	}
	if (Deco->IsCenteredInY) {
		y -= sprite->Height / 2;
	}
	sprite->DrawFrameClip(Deco->Data.StaticSprite.n, x, y);
}


extern void UpdateUnitVariables(const Unit* unit);


/**
**  Draw decoration (invis, for the unit.)
**
**  @param unit  Pointer to the unit.
**  @param type  Type of the unit.
**  @param x     Screen X position of the unit.
**  @param y     Screen Y position of the unit.
*/
static void DrawDecoration(const Unit* unit, const UnitType* type, int x, int y)
{
	int i;

#ifdef REFS_DEBUG
	//
	// Show the number of references.
	//
	VideoDrawNumberClip(x + 1, y + 1, GameFont, unit->Refs);
#endif

	UpdateUnitVariables(unit);
	// Now show decoration for each variable.
	for (i = 0; i < UnitTypeVar.NumberDeco; i++) {
		int value;
		int max;
		const DecoVarType* Deco;

		Deco = &UnitTypeVar.DecoVar[i];
		Assert(Deco->f);
		value = unit->Variable[Deco->Index].Value;
		max = unit->Variable[Deco->Index].Max;
		Assert(value <= max);

		if (!((value == 0 && !Deco->ShowWhenNull) || (value == max && !Deco->ShowWhenMax)
			|| (Deco->HideHalf && value != 0 && value != max)
			|| (!Deco->ShowIfNotEnable && !unit->Variable[Deco->Index].Enable)
			|| (Deco->ShowOnlySelected && !unit->Selected)
			|| (unit->Player->Type == PlayerNeutral && Deco->HideNeutral)
			|| (IsEnemy(ThisPlayer, unit) && !Deco->ShowOpponent)
			|| (IsAllied(ThisPlayer, unit) && (unit->Player != ThisPlayer) && Deco->HideAllied)
			|| max == 0)) {
			Deco->f(
				x + Deco->OffsetX + Deco->OffsetXPercent * unit->Type->TileWidth * TileSizeX / 100,
				y + Deco->OffsetY + Deco->OffsetYPercent * unit->Type->TileHeight * TileSizeY / 100,
				unit, Deco);
		}
	}
	//
	// Draw group number
	//
	if (unit->Selected && unit->GroupId != 0) {
		char buf[2];
		int num;
		int f;

		for (num = 0; !(unit->GroupId & (1 << num)); ++num) {
			;
		}
		buf[0] = num + '0';
		buf[1] = '\0';
		f = VideoTextLength(GameFont, buf);
		x += (type->TileWidth * TileSizeX + type->BoxWidth) / 2 - f;
		f = VideoTextHeight(GameFont);
		y += (type->TileHeight * TileSizeY + type->BoxHeight) / 2 - f;
		VideoDrawNumberClip(x, y, GameFont, num);
	}
}

/**
**  Draw unit's shadow.
**
**  @param unit   Pointer to the unit.
**  @param type   Pointer to the unit type.
**  @param frame  Frame number
**  @param x      Screen X position of the unit.
**  @param y      Screen Y position of the unit.
**
**  @todo FIXME: combine new shadow code with old shadow code.
*/
void DrawShadow(const Unit* unit, const UnitType* type, int frame,
	int x, int y)
{
	if (!type) {
		Assert(unit);
		type = unit->Type;
	}
	Assert(type);
	Assert(!unit || unit->Type == type);

	// unit == NULL for the editor
	if (unit && unit->Orders[0].Action == UnitActionDie) {
		return;
	}

	// Draw normal shadow sprite if available
	if (type->ShadowSprite) {
		x -= (type->ShadowWidth -
			type->TileWidth * TileSizeX) / 2;
		y -= (type->ShadowHeight -
			type->TileHeight * TileSizeY) / 2;
		x += type->OffsetX + type->ShadowOffsetX;
		y += type->OffsetY + type->ShadowOffsetY;

		if (type->Flip) {
			if (frame < 0) {
				type->ShadowSprite->DrawFrameClipX(-frame - 1, x, y);
			} else {
				type->ShadowSprite->DrawFrameClip(frame, x, y);
			}
		} else {
			int row;

			row = type->NumDirections / 2 + 1;
			if (frame < 0) {
				frame = ((-frame - 1) / row) * type->NumDirections + type->NumDirections - (-frame - 1) % row;
			} else {
				frame = (frame / row) * type->NumDirections + frame % row;
			}
			type->ShadowSprite->DrawFrameClip(frame, x, y);
		}
	}
}

/**
**  Get the location of an unit's order.
**
**  @param unit   Pointer to unit.
**  @param order  Pointer to order.
**  @param x      Resulting screen X cordinate.
**  @param y      Resulting screen Y cordinate.
*/
static void GetOrderPosition(const Unit* unit, const Order* order, int* x, int* y)
{
	Unit* goal;

	// FIXME: n0body: Check for goal gone?
	if ((goal = order->Goal) && (!goal->Removed)) {
		// Order has a goal, get it's location.
		*x = Map2ViewportX(CurrentViewport, goal->X) + goal->IX +
			goal->Type->TileWidth * TileSizeX / 2;
		*y = Map2ViewportY(CurrentViewport, goal->Y) + goal->IY +
			goal->Type->TileHeight * TileSizeY / 2;
	} else {
		if (order->X >= 0 && order->Y >= 0) {
			// Order is for a location, show that.
			*x = Map2ViewportX(CurrentViewport, order->X) + TileSizeX / 2;
			*y = Map2ViewportY(CurrentViewport, order->Y) + TileSizeY / 2;
		} else {
			// Some orders ignore x,y (like StandStill).
			// Use the unit's position instead.
			*x = Map2ViewportX(CurrentViewport, unit->X) + unit->IX +
				unit->Type->TileWidth * TileSizeX / 2;
			*y = Map2ViewportY(CurrentViewport, unit->Y) + unit->IY +
				unit->Type->TileHeight * TileSizeY / 2;
		}
		if (order->Action == UnitActionBuild) {
			*x += (order->Type->TileWidth - 1) * TileSizeX / 2;
			*y += (order->Type->TileHeight - 1) * TileSizeY / 2;
		}
	}
}

/**
**  Show the order on map.
**
**  @param unit   Unit pointer.
**  @param x1     X pixel coordinate.
**  @param y1     Y pixel coordinate.
**  @param order  Order to display.
*/
static void ShowSingleOrder(const Unit* unit, int x1, int y1, const Order* order)
{
	int x2;
	int y2;
	Uint32 color;
	Uint32 e_color;
	int dest;

	GetOrderPosition(unit, order, &x2, &y2);

	dest = 0;
	switch (order->Action) {
		case UnitActionNone:
			e_color = color = ColorGray;
			break;

		case UnitActionStill:
			e_color = color = ColorGray;
			break;

		case UnitActionStandGround:
			e_color = color = ColorGreen;
			break;

		case UnitActionFollow:
		case UnitActionMove:
			e_color = color = ColorGreen;
			dest = 1;
			break;

		case UnitActionPatrol:
			VideoDrawLineClip(ColorGreen, x1, y1, x2, y2);
			e_color = color = ColorBlue;
			x1 = Map2ViewportX(CurrentViewport,
				order->Arg1.Patrol.X) + TileSizeX / 2;
			y1 = Map2ViewportY(CurrentViewport,
				order->Arg1.Patrol.Y) + TileSizeY / 2;
			dest = 1;
			break;

		case UnitActionRepair:
			e_color = color = ColorGreen;
			dest = 1;
			break;

		case UnitActionAttackGround:
			x2 = Map2ViewportX(CurrentViewport, order->X) + TileSizeX / 2;
			y2 = Map2ViewportY(CurrentViewport, order->Y) + TileSizeY / 2;
			// FALL THROUGH
		case UnitActionAttack:
			if (unit->SubAction & 2) { // Show weak targets.
				e_color = ColorBlue;
			} else {
				e_color = ColorRed;
			}
			color = ColorRed;
			dest = 1;
			break;

		case UnitActionBoard:
			e_color = color = ColorGreen;
			dest = 1;
			break;

		case UnitActionUnload:
			e_color = color = ColorGreen;
			dest = 1;
			break;

		case UnitActionDie:
			e_color = color = ColorGray;
			break;

		case UnitActionSpellCast:
			e_color = color = ColorBlue;
			dest = 1;
			break;

		case UnitActionTrain:
			e_color = color = ColorGray;
			break;

		case UnitActionUpgradeTo:
			e_color = color = ColorGray;
			break;

		case UnitActionResearch:
			e_color = color = ColorGray;
			break;

		case UnitActionBuild:
			DrawSelection(ColorGray, x2 - order->Type->BoxWidth / 2,
				y2 - order->Type->BoxHeight / 2,
				x2 + order->Type->BoxWidth / 2,
				y2 + order->Type->BoxHeight / 2);
			e_color = color = ColorGreen;
			dest = 1;
			break;

		case UnitActionBuilt:
			e_color = color = ColorGray;
			break;

		case UnitActionResource:
			e_color = color = ColorYellow;
			dest = 1;
			break;

		case UnitActionReturnGoods:
			e_color = color = ColorYellow;
			dest = 1;
			break;

		default:
			e_color = color = ColorGray;
			DebugPrint("Unknown action %d\n" _C_ order->Action);
			break;
	}
	VideoFillCircleClip(color, x1, y1, 2);
	if (dest) {
		VideoDrawLineClip(color, x1, y1, x2, y2);
		VideoFillCircleClip(e_color, x2, y2, 3);
	}
}

/**
**  Show the current order of an unit.
**
**  @param unit  Pointer to the unit.
*/
void ShowOrder(const Unit* unit)
{
	int x1;
	int y1;
	int i;

	if (unit->Destroyed) {
		return;
	}

	x1 = Map2ViewportX(CurrentViewport,
		unit->X) + unit->IX + unit->Type->TileWidth * TileSizeX / 2;
	y1 = Map2ViewportY(CurrentViewport,
		unit->Y) + unit->IY + unit->Type->TileHeight * TileSizeY / 2;

	ShowSingleOrder(unit, x1, y1, unit->Orders);
#if 1
	for (i = 1; i < unit->OrderCount; ++i) {
		GetOrderPosition(unit, unit->Orders + i - 1, &x1, &y1);
		ShowSingleOrder(unit, x1, y1, unit->Orders + i);
	}
#endif
	if (!CanMove(unit)) {
		ShowSingleOrder(unit, x1, y1, &unit->NewOrder);
	}
}

/**
**  Draw additional informations of an unit.
**
**  @param unit  Unit pointer of drawn unit.
**  @param type  Unit-type pointer.
**  @param x     X screen pixel position of unit.
**  @param y     Y screen pixel position of unit.
**
**  @todo FIXME: The different styles should become a function call.
*/
static void DrawInformations(const Unit* unit, const UnitType* type, int x, int y)
{
	const UnitStats* stats;
	int r;

#if 0 && DEBUG // This is for showing vis counts and refs.
	char buf[10];
	sprintf(buf, "%d%c%c%d", unit->VisCount[ThisPlayer->Index],
		unit->Seen.ByPlayer & (1 << ThisPlayer->Index) ? 'Y' : 'N',
		unit->Seen.Destroyed & (1 << ThisPlayer->Index) ? 'Y' : 'N',
		unit->Refs);
	VideoDrawTextClip(x + 10, y + 10, 1, buf);
#endif

	stats = unit->Stats;

	//
	// For debug draw sight, react and attack range!
	//
	if (NumSelected == 1 && unit->Selected) {
		if (ShowSightRange) {
			// Radius -1 so you can see all ranges
			VideoDrawCircleClip(ColorGreen,
				x + type->TileWidth * TileSizeX / 2,
				y + type->TileHeight * TileSizeY / 2,
				((stats->Variables[SIGHTRANGE_INDEX].Max + (type->TileWidth - 1)) * TileSizeX) - 1);
		}
		if (type->CanAttack) {
			if (ShowReactionRange) {
				r = (unit->Player->Type == PlayerPerson) ?
					type->ReactRangePerson : type->ReactRangeComputer;
				if (r) {
					VideoDrawCircleClip(ColorBlue,
						x + type->TileWidth * TileSizeX / 2,
						y + type->TileHeight * TileSizeY / 2,
						(r + (type->TileWidth - 1)) * TileSizeX);
				}
			}
			if (ShowAttackRange && stats->Variables[ATTACKRANGE_INDEX].Max) {
				// Radius + 1 so you can see all ranges
				VideoDrawCircleClip(ColorRed,
					x + type->TileWidth * TileSizeX / 2,
					y + type->TileHeight * TileSizeY / 2,
					(stats->Variables[ATTACKRANGE_INDEX].Max + (type->TileWidth - 1)) * TileSizeX + 1);
			}
		}
	}

	// FIXME: johns: ugly check here, should be removed!
	if (unit->Orders[0].Action != UnitActionDie && UnitVisible(unit, ThisPlayer)) {
		DrawDecoration(unit, type, x, y);
	}
}

#if 0
/**
**  Draw the sprite with the player colors
**
**  @param type      Unit type
**  @param sprite    Original sprite
**  @param player    Player number
**  @param frame     Frame number to draw.
**  @param x         X position.
**  @param y         Y position.
*/
void DrawUnitPlayerColor(const UnitType* type, Graphic* sprite,
	int player, int frame, int x, int y)
{
	int f;

	if (type->Flip) {
		if (frame < 0) {
			f = -frame - 1;
		} else {
			f = frame;
		}
	} else {
		int row;

		row = type->NumDirections / 2 + 1;
		if (frame < 0) {
			f = ((-frame - 1) / row) * type->NumDirections + type->NumDirections - (-frame - 1) % row;
		} else {
			f = (frame / row) * type->NumDirections + frame % row;
		}
	}
	if (!sprite->PlayerColorTextures[player]) {
		MakePlayerColorTexture(sprite, player, &Players[player].UnitColors);
	}

	// FIXME: move this calculation to high level.
	x -= (type->Width - type->TileWidth * TileSizeX) / 2;
	y -= (type->Height - type->TileHeight * TileSizeY) / 2;

	if (type->Flip) {
		if (frame < 0) {
			VideoDrawClipX(glsprite[player], -frame - 1, x, y);
		} else {
			VideoDrawClip(glsprite[player], frame, x, y);
		}
	} else {
		int row;

		row = type->NumDirections / 2 + 1;
		if (frame < 0) {
			frame = ((-frame - 1) / row) * type->NumDirections + type->NumDirections - (-frame - 1) % row;
		} else {
			frame = (frame / row) * type->NumDirections + frame % row;
		}
		VideoDrawClip(glsprite[player], frame, x, y);
	}
}
#endif

/**
**  Draw construction shadow.
**
**  @param unit   Unit pointer.
**  @param frame  Frame number to draw.
**  @param x      X position.
**  @param y      Y position.
*/
static void DrawConstructionShadow(const Unit* unit, int frame, int x, int y)
{
	ConstructionFrame* cframe;

	cframe = unit->Data.Built.Frame;
	if (cframe->File == ConstructionFileConstruction) {
		if (unit->Type->Construction->ShadowSprite) {
			x -= (unit->Type->Construction->Width - unit->Type->TileWidth * TileSizeX) / 2;
			y -= (unit->Type->Construction->Height - unit->Type->TileHeight * TileSizeY )/ 2;
// x += type->ShadowOffsetX;
// y += type->ShadowOffsetY;
			x += unit->Type->OffsetX;
			y += unit->Type->OffsetY;
			if (unit->Type->Flip) {
				if (frame < 0) {
					unit->Type->Construction->ShadowSprite->DrawFrameClipX(
						-frame - 1, x, y);
				} else {
					unit->Type->Construction->ShadowSprite->DrawFrameClip(
						frame, x, y);
				}
			} else {
				int row;

				row = unit->Type->NumDirections / 2 + 1;
				if (frame < 0) {
					frame = ((-frame - 1) / row) * unit->Type->NumDirections + unit->Type->NumDirections - (-frame - 1) % row;
				} else {
					frame = (frame / row) * unit->Type->NumDirections + frame % row;
				}
				unit->Type->Construction->ShadowSprite->DrawFrameClip(frame,
					x, y);
			}
		}
	} else {
		if (unit->Type->ShadowSprite) {
			x -= (unit->Type->ShadowWidth - unit->Type->TileWidth * TileSizeX) / 2;
			y -= (unit->Type->ShadowHeight - unit->Type->TileHeight * TileSizeY) / 2;
			x += unit->Type->ShadowOffsetX;
			y += unit->Type->ShadowOffsetY;
			x += unit->Type->OffsetX;
			y += unit->Type->OffsetY;
			if (unit->Type->Flip) {
				if (frame < 0) {
					unit->Type->ShadowSprite->DrawFrameClipX(-frame - 1, x, y);
				} else {
					unit->Type->ShadowSprite->DrawFrameClip(frame, x, y);
				}
			} else {
				int row;

				row = unit->Type->NumDirections / 2 + 1;
				if (frame < 0) {
					frame = ((-frame - 1) / row) * unit->Type->NumDirections + unit->Type->NumDirections - (-frame - 1) % row;
				} else {
					frame = (frame / row) * unit->Type->NumDirections + frame % row;
				}
				unit->Type->ShadowSprite->DrawFrameClip(frame, x, y);
			}
		}
	}
}

/**
**  Draw construction.
**
**  @param unit    Unit pointer.
**  @param cframe  Construction frame to draw.
**  @param type    Unit type.
**  @param frame   Frame number.
**  @param x       X position.
**  @param y       Y position.
*/
static void DrawConstruction(const Unit* unit, const ConstructionFrame* cframe,
	const UnitType* type, int frame, int x, int y)
{
	int player;

	player = unit->RescuedFrom ? unit->RescuedFrom->Index : unit->Player->Index;
	if (cframe->File == ConstructionFileConstruction) {
		const Construction* construction;

		construction = type->Construction;
		x -= construction->Width / 2;
		y -= construction->Height / 2;
		if (frame < 0) {
			construction->Sprite->DrawPlayerColorFrameClipX(player, -frame - 1, x, y);
		} else {
			construction->Sprite->DrawPlayerColorFrameClip(player, frame, x, y);
		}
	} else {
		x += type->OffsetX - type->Width / 2;
		y += type->OffsetY - type->Height / 2;
		if (frame < 0) {
			frame = -frame - 1;
		}
		type->Sprite->DrawPlayerColorFrameClip(player, frame, x, y);
	}
}

/**
**  Units on map:
*/

/**
**  Draw unit on map.
**
**  @param unit  Pointer to the unit.
*/
void DrawUnit(const Unit* unit)
{
	int x;
	int y;
	int frame;
	int state;
	int constructed;
	Graphic* sprite;
	ResourceInfo* resinfo;
	ConstructionFrame* cframe;
	UnitType* type;

	if (unit->Type->Revealer) { // Revealers are not drawn
		return;
	}

	// Those should have been filtered. Check doesn't make sense with ReplayRevealMap
	Assert(ReplayRevealMap || unit->Type->VisibleUnderFog || UnitVisible(unit, ThisPlayer));

	if (ReplayRevealMap || UnitVisible(unit, ThisPlayer)) {
		type = unit->Type;
		frame = unit->Frame;
		y = unit->IY;
		x = unit->IX;
		x += Map2ViewportX(CurrentViewport, unit->X);
		y += Map2ViewportY(CurrentViewport, unit->Y);
		state = (unit->Orders[0].Action == UnitActionBuilt) |
			((unit->Orders[0].Action == UnitActionUpgradeTo) << 1);
		constructed = unit->Constructed;
		// Reset Type to the type being upgraded to
		if (state == 2) {
			type = unit->Orders[0].Type;
		}
		// This is trash unless the unit is being built, and that's when we use it.
		cframe = unit->Data.Built.Frame;
	} else {
		y = unit->Seen.IY;
		x = unit->Seen.IX;
		x += Map2ViewportX(CurrentViewport, unit->Seen.X);
		y += Map2ViewportY(CurrentViewport, unit->Seen.Y);
		frame = unit->Seen.Frame;
		type = unit->Seen.Type;
		constructed = unit->Seen.Constructed;
		state = unit->Seen.State;
		cframe = unit->Seen.CFrame;
	}

#ifdef DYNAMIC_LOAD
	if (!type->Sprite) {
		LoadUnitTypeSprite(type);
	}
#endif

	if ((!UnitVisible(unit, ThisPlayer)) && frame == UnitNotSeen) {
		DebugPrint("FIXME: Something is wrong, unit %d not seen but drawn time %lu?.\n" _C_
			unit->Slot _C_ GameCycle);
		return;
	}


	if (state == 1 && constructed) {
		DrawConstructionShadow(unit, frame, x, y);
	} else {
		DrawShadow(unit, NULL, frame, x, y);
	}

	//
	// Show that the unit is selected
	//
	DrawUnitSelection(unit);

	//
	// Adjust sprite for Harvesters.
	//
	sprite = type->Sprite;
	if (type->Harvester && unit->CurrentResource) {
		resinfo = type->ResInfo[unit->CurrentResource];
		if (unit->ResourcesHeld) {
			if (resinfo->SpriteWhenLoaded) {
				sprite = resinfo->SpriteWhenLoaded;
			}
		} else {
			if (resinfo->SpriteWhenEmpty) {
				sprite = resinfo->SpriteWhenEmpty;
			}
		}
	}

	//
	// Now draw!
	// Buildings under construction/upgrade/ready.
	//
	if (state == 1) {
		if (constructed) {
			DrawConstruction(unit, cframe, type, frame,
				x + (type->TileWidth * TileSizeX) / 2,
				y + (type->TileHeight * TileSizeY) / 2);
		}
	//
	// Draw the future unit type, if upgrading to it.
	//
	} else if (state == 2) {
		// FIXME: this frame is hardcoded!!!
		DrawUnitType(type, sprite,
			unit->RescuedFrom ? unit->RescuedFrom->Index : unit->Player->Index,
			frame < 0 ? -1 - 1 : 1, x, y);
	} else {
		DrawUnitType(type, sprite,
			unit->RescuedFrom ? unit->RescuedFrom->Index : unit->Player->Index,
			frame, x, y);
	}

	// Unit's extras not fully supported.. need to be decorations themselves.
	DrawInformations(unit, type, x, y);
}

/**
**  Compare what order 2 units should be drawn on the map
**
**  @param v1  First Unit to compare (**Unit)
**  @param v2  Second Unit to compare (**Unit)
**
**  @return -1 for v1 < v2, 1 for v2 < v1
*/
static int DrawLevelCompare(const void* v1, const void* v2) {

	const Unit* c1;
	const Unit* c2;
	int drawlevel1;
	int drawlevel2;
	int diffpos;

	c1 = *(Unit**)v1;
	c2 = *(Unit**)v2;

	if (c1->Orders[0].Action == UnitActionDie && c1->Type->CorpseType) {
		drawlevel1 = c1->Type->CorpseType->DrawLevel;
	} else {
		drawlevel1 = c1->Type->DrawLevel;
	}
	if (c2->Orders[0].Action == UnitActionDie && c2->Type->CorpseType) {
		drawlevel2 = c2->Type->CorpseType->DrawLevel;
	} else {
		drawlevel2 = c2->Type->DrawLevel;
	}
	if (drawlevel1 == drawlevel2) {
		// diffpos compares unit's Y positions (bottom of sprite) on the map
		// and uses X position in case Y positions are equal.
		// FIXME: Use BoxHeight?
		diffpos = c1->Y * TileSizeY + c1->IY + c1->Type->Height -
			(c2->Y * TileSizeY + c2->IY + c2->Type->Height);
		return diffpos ? diffpos : c1->X - c2->X ? c1->X - c2->X : c1->Slot - c2->Slot;
	} else {
		return drawlevel1 <= drawlevel2 ? -1 : 1;
	}
}
/**
**  Find all units to draw in viewport.
**
**  @param vp     Viewport to be drawn.
**  @param table  Table of units to return in sorted order 
**
*/
int FindAndSortUnits(const Viewport* vp, Unit** table)
{
	int i;
	int n;

	//
	//  Select all units touching the viewpoint.
	//
	n = UnitCacheSelect(vp->MapX - 1, vp->MapY - 1, vp->MapX + vp->MapWidth + 1,
		vp->MapY + vp->MapHeight + 1, table);

	for (i = 0; i < n; i++) {
		if (!UnitVisibleInViewport(table[i], vp)) {
			table[i--] = table[--n];
		}
	}

	if (n) {
		qsort((void*)table, n, sizeof(Unit*), DrawLevelCompare);
	}

	return n;
}

//@}
