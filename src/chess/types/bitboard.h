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

#include "../../game/types/bitboard.h"

static constexpr Bitboard DarkSquares = Bitboard(0xaaaaaaaaaaaaaaaa);
static constexpr Bitboard LightSquares = Bitboard(0x5555555555555555);

#define DarkSquarePieces(x) (x & DarkSquares)
#define LightSquarePieces(x) (x & LightSquares)

#define IsDarkSquare(x) (DarkSquarePieces(x) == EmptyBitboard)
#define IsLightSquare(x) (LightSquarePieces(x) == EmptyBitboard)

#define SameColorAsPiece(x,b) ( IsDarkSquare(b) ? DarkSquarePieces(x) : LightSquarePieces(x) )
#define OppositeColorAsPiece(x,b) ( IsDarkSquare(b) ? LightSquarePieces(x) : DarkSquarePieces(x) )
