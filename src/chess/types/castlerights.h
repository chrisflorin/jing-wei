/*
    Jing Wei, the rebirth of the chess engine I started in 2010
    Copyright(C) 2019-2020 Chris Florin

    This program is free software : you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

enum CastleRights {
	CASTLE_NONE,
	WHITE_OO = 1,
	WHITE_OOO = 2,
	BLACK_OO = 4,
	BLACK_OOO = 8,
	WHITE_ALL = WHITE_OO | WHITE_OOO,
	BLACK_ALL = BLACK_OO | BLACK_OOO,
	CASTLE_ALL = WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO,
	CASTLERIGHTS_START = 0,
	CASTLERIGHTS_COUNT = 16
};

static CastleRights& operator ++ (CastleRights& cr, int)
{
    return cr = CastleRights(int(cr) + 1);
}

static CastleRights operator ^ (CastleRights cr1, CastleRights cr2)
{
    return CastleRights(int(cr1) ^ int(cr2));
}

static CastleRights operator | (CastleRights cr1, CastleRights cr2)
{
    return CastleRights(int(cr1) | int(cr2));
}

static CastleRights operator & (CastleRights cr1, CastleRights cr2)
{
    return CastleRights(int(cr1) & int(cr2));
}

static CastleRights& operator |= (CastleRights& cr1, CastleRights cr2)
{
    return cr1 = cr1 | cr2;
}

static CastleRights& operator &= (CastleRights& cr1, CastleRights cr2)
{
    return cr1 = cr1 & cr2;
}
