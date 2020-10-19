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

#include <string>

#include "../../game/math/shift.h"

#include "../../game/types/bitboard.h"

enum Square {
    A8, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1,
    SQUARE_COUNT,
    NO_SQUARE,
    FIRST_SQUARE = A8
};

Bitboard GetDarkSquares(Bitboard squares);
Bitboard GetLightSquares(Bitboard squares);

bool IsDarkSquare(Square src);
bool IsLightSquare(Square src);

Bitboard SquaresOppositeColorAs(Bitboard squares, Square src);
Bitboard SquaresSameColorAs(Bitboard squares, Square src);

#define FlipSqY(src) Square((src) ^ 56)

Square StringToSquare(std::string& str);

static Square& operator ++ (Square& s, int)
{
    return s = Square(int(s) + 1);
}

static Bitboard& operator |= (Bitboard& b, Square s)
{
    return b |= OneShiftedBy(s);
}

enum Direction {
    NO_DIRECTION = 0,
    UP = -8, DOWN = 8, RIGHT = 1, LEFT = -1,
    UP_RIGHT = UP + RIGHT,
    UP_LEFT = UP + LEFT,
    DOWN_RIGHT = DOWN + RIGHT,
    DOWN_LEFT = DOWN + LEFT,
    TWO_UP = UP + UP,
    TWO_DOWN = DOWN + DOWN,
    UP_LEFT_LEFT = UP + LEFT + LEFT,
    UP_UP_LEFT = UP + UP + LEFT,
    UP_UP_RIGHT = UP + UP + RIGHT,
    UP_RIGHT_RIGHT = UP + RIGHT + RIGHT,
    DOWN_LEFT_LEFT = DOWN + LEFT + LEFT,
    DOWN_DOWN_LEFT = DOWN + DOWN + LEFT,
    DOWN_DOWN_RIGHT = DOWN + DOWN + RIGHT,
    DOWN_RIGHT_RIGHT = DOWN + RIGHT + RIGHT,
    ONE_RANK = 8, ONE_FILE = 1
};

static Direction operator * (Direction d, int i)
{
    return Direction(int(d) * i);
}

static Square operator + (Square s, Direction d)
{
    return Square(int(s) + int(d));
}

static Square operator += (Square s, Direction d)
{
    return s = s + d;
}

static Bitboard operator + (Bitboard b, Direction d)
{
    if (d < 0) {
        return b >> (-d);
    }
    else {
        return b << d;
    }
}

static Bitboard operator += (Bitboard b, Direction d)
{
    return b = b + d;
}

enum File {
    _A, _B, _C, _D, _E, _F, _G, _H, FILE_COUNT, FIRST_FILE = _A
};

static File getFile(Square s)
{
    return File(int(s) % 8);
}

static File operator + (File f, Direction dir)
{
    return File(int(f) + int(dir));
}

static File operator + (File f1, File f2)
{
    return File(int(f1) + int(f2));
}

static File operator - (File f1, File f2)
{
    return File(int(f1) - int(f2));
}

static bool operator > (File r1, File r2)
{
    return int(r1) > int(r2);
}

static bool operator < (File r1, File r2)
{
    return int(r1) < int(r2);
}

static File FileDistance(Square s1, Square s2)
{
    return File(std::abs(getFile(s1) - getFile(s2)));
}

enum Rank {
    _8, _7, _6, _5, _4, _3, _2, _1, RANK_COUNT, FIRST_RANK = _8
};

static Rank getRank(Square s) {
    return Rank(int(s) / 8);
}

static Rank operator + (Rank r, Direction d)
{
    Rank dr = getRank(Square(int(d)));

    return Rank(r + int(dr));
}

static Rank operator + (Rank r1, Rank r2)
{
    return Rank(int(r1) + int(r2));
}

static Rank operator - (Rank r1, Rank r2)
{
    return Rank(int(r1) - int(r2));
}

static bool operator > (Rank r1, Rank r2)
{
    return int(r1) < int(r2);
}

static bool operator < (Rank r1, Rank r2)
{
    return int(r1) > int(r2);
}

static Rank RankDistance(Square s1, Square s2)
{
    return Rank(std::abs(getRank(s1) - getRank(s2)));
}

static Square operator * (File f, Rank r)
{
    return Square(int(f) + 8 * int(r));
}

static Square operator * (Rank r, File f)
{
    return Square(8 * int(r) + int(f));
}

static Rank operator ~ (Rank& r)
{
    return Rank(int(Rank::_1) - int(r));
}
