/*
This file is part of UFO2000 (http://ufo2000.sourceforge.net)

Copyright (C) 2000-2001  Alexander Ivanov aka Sanami
Copyright (C) 2002-2005  ufo2000 development team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef TERRAPCK_H
#define TERRAPCK_H

#include "global.h"

/**
 * Information about the shape of map cell. Each cell is represented
 * by 16x16x12 small elements called voxels. Each element is stored as
 * a single bit, so each map cell shape information takes 384 bytes.
 * When a bit descriping voxel is set, that part of map is solid
 * and can block bullets and other projectiles.
 */
struct ShapeInfo
{
    uint16 data[12][16];
};

/**
 * Map cell description data. Provides information about cell object
 * shape, ability to walk/see through it, reference to pictures for
 * rendering cell. Map cell is described by 4 MCD records: two walls,
 * floor and object
 *
 * @ingroup battlescape
 */
#pragma pack(1)
struct MCD
{
//! Each frame is an index into the ____.TAB file; it rotates between the frames constantly.
    unsigned char Frame[8];
//! The 12 levels of references into GEODATA/LOFTEMPS.DAT
    unsigned char LOFT[12];      
//! A reference into the GEODATA/SCANG.DAT
    int16         ScanG;
    unsigned char u23;
    unsigned char u24;
    unsigned char u25;
    unsigned char u26;
    unsigned char u27;
    unsigned char u28;
    unsigned char u29;
    unsigned char u30;
//! If it's a UFO door it uses only Frame[0] until it is walked through, then 
//! it animates once and becomes Alt_MCD.  It changes back at the end of the turn
    unsigned char UFO_Door;
//! You cannot see through this tile.
    unsigned char Stop_LOS;
//! If 1, then a non-flying unit can't stand here
    unsigned char No_Floor;
//! It's an object (tile type 3), but it acts like a wall
    unsigned char Big_Wall;      
    unsigned char Gravlift;
//! It's a human style door--you walk through it and it changes to Alt_MCD
    unsigned char Door;
//! If 1, fire won't go through the tile
    unsigned char Block_Fire;
//! If 1, smoke won't go through the tile
    unsigned char Block_Smoke;
    unsigned char u39;
//! The number of TUs require to pass the tile while walking. 
//! An 0xFF (255) means it's unpassable.
    unsigned char TU_Walk;
//! remember, 0xFF means it's impassable!
    unsigned char TU_Fly;
//! sliding things include snakemen and silacoids
    unsigned char TU_Slide;
//! The higher this is the less likely it is that a weapon will destroy this tile when it's hit.
    unsigned char Armour;
//! How much of an explosion this tile will block
    unsigned char HE_Block;
//! If the terrain is destroyed, it is set to 0 and a tile of type Die_tile (Which is Die_MCD) is added
    unsigned char Die_MCD;
//! How flammable it is (the higher the harder it is to set aflame)
    unsigned char Flammable;
//! If "Door" or "UFO_Door" is on, then when a unit walks through it the door 
//! is set to 0 and a tile type Alt_tile (Which is Alt_MCD) is added.
    unsigned char Alt_MCD;       
    unsigned char u48;
//! When a unit or object is standing on the tile, the unit is shifted by this amount
    signed char T_Level;      
//! When the tile is drawn, this amount is subtracted from its y (so y 
//! position-P_Level is where it's drawn)
    unsigned char P_Level;
    unsigned char u51;
//! The amount of light it blocks, from 0 to 10
    unsigned char Light_Block;     
//! The Sound Effect set to choose from when footsteps are on the tile
    unsigned char Footstep;         
//! This is the type of tile it is meant to be -- 0=floor, 1=west wall, 
//! 2=north wall, 3=object .  When this type of tile is in the Die_As or 
//! Open_As flags, this value is added to the tile coordinate to determine 
//! the byte in which the tile type should be written.
    unsigned char Tile_Type;
//! 0=HE  1=Smoke
    unsigned char HE_Type;         
//! The strength of the explosion caused when it's destroyed.  0 means no explosion.
    unsigned char HE_Strength;
//! ? Not sure about this ...
    unsigned char Smoke_Blockage;      
//! The number of turns the tile will burn when set aflame
    unsigned char Fuel;
//! The amount of light this tile produces
    unsigned char Light_Source;
//! The special properties of the tile
    unsigned char Target_Type;
    unsigned char u61;
    unsigned char u62;

//! End of X-COM data, ufo2000 specific data comes next
    unsigned char ufo2000_data_start_marker[2];

//! Out of the structure of the MCD file, we add these values for supporting more than 255 tiles.
	unsigned int Die_tile;
	unsigned int Alt_tile;
	
    ALPHA_SPRITE *FrameBitmap[8];
    BITMAP *ScangBitmap;
    int     ShapeIndex;
}
#ifdef __GNUC__
__attribute__((packed))
#endif
;
#pragma pack()

/**
 * Map cell description data. Provides information about cell object
 * shape, ability to walk/see through it, reference to pictures for
 * rendering cell. Map cell is described by 4 MCD records: two walls,
 * floor and object
 *
 * @ingroup battlescape
 */
class TerraPCK
{
    void add_xcom_tileset(const char *mcd_name, int tftd_flag);
    void add_ufo2000_tileset(const char *tileset_name);
    int  tileset_get_int(uint32 index, const char *tileset_name, const char *property_name, int defval);
    int  get_shape_index(const ShapeInfo &s);
public:
    std::vector<MCD> m_mcd;

    int is_solid_voxel(int index, int z, int x, int y);

    TerraPCK(const char *mcd_name, int tftd_flag);
    ~TerraPCK();

    void add(const char *mcd_name, int tftd_flag);
};

#endif
