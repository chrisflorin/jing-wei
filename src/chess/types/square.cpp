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

#include <string>

#include "square.h"

static const std::string FileToChar = "abcdefgh";
static const std::string RankToChar = "87654321";

static constexpr Bitboard DarkSquares = 0xaaaaaaaaaaaaaaaaull;
static constexpr Bitboard LightSquares = 0x5555555555555555ull;

Bitboard GetDarkSquares(Bitboard squares)
{
    return squares & DarkSquares;
}

Bitboard GetLightSquares(Bitboard squares)
{
    return squares & LightSquares;
}

bool IsDarkSquare(Square src)
{
    return (DarkSquares & OneShiftedBy(src)) != EmptyBitboard;
}

bool IsLightSquare(Square src)
{
    return (LightSquares & OneShiftedBy(src)) != EmptyBitboard;
}

Bitboard SquaresOppositeColorAs(Bitboard squares, Square src)
{
    return IsDarkSquare(src) ? GetLightSquares(squares) : GetDarkSquares(squares);
}

Bitboard SquaresSameColorAs(Bitboard squares, Square src)
{
    return IsDarkSquare(src) ? GetDarkSquares(squares) : GetLightSquares(squares);
}

Square StringToSquare(std::string& str)
{
    File file;
    Rank rank;

    const char* token = str.c_str();

    file = File(FileToChar.find(token[0]));
    rank = Rank(RankToChar.find(token[1]));

    return rank * file;
}
