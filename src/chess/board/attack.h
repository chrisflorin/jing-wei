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

#include "board.h"

#include "../../game/types/score.h"

#include "../types/square.h"

class ChessAttackGenerator
{
public:
	ChessAttackGenerator();
	~ChessAttackGenerator();

	Bitboard getAttackingPieces(ChessBoard& board, Square dst, bool earlyExit, Bitboard attackThrough);

	bool isInCheck(ChessBoard& board, bool otherSide = false);
	bool isSquareAttacked(ChessBoard& board, Square dst);
};
