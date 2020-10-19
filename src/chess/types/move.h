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

#include "piece.h"
#include "square.h"

#include "../../game/types/movelist.h"

enum ChessMoveOrdinal {
    NO_CHESS_MOVE_ORDINAL,
    PV_MOVE = -1000000,
    QUIESENCE_MOVE = -1000000,
    GOOD_CAPTURE_MOVE = -2000000,
    EQUAL_CAPTURE_MOVE = -3000000,
    KILLER1_MOVE = -4000000,
    KILLER2_MOVE = -5000000,
    BUTTERFLY_MOVE = -6000000,
    UNCLASSIFIED_MOVE = -6000000,
    BAD_CAPTURE_MOVE = -7000000,
    UNSAFE_MOVE = -8000000
};

static ChessMoveOrdinal operator + (ChessMoveOrdinal cmo1, ChessMoveOrdinal cmo2)
{
    return ChessMoveOrdinal(int(cmo1) + int(cmo2));
}

struct ChessMove {
    ChessMoveOrdinal ordinal;
	Square src, dst;
	PieceType promotionPiece;

    PieceType capturedPiece = PieceType::NO_PIECE;
    PieceType movedPiece = PieceType::NO_PIECE;
};

static bool operator == (ChessMove& m1, ChessMove& m2)
{
    return m1.src == m2.src
        && m1.dst == m2.dst
        && m1.promotionPiece == m2.promotionPiece;
}

static bool operator != (ChessMove& m1, ChessMove& m2)
{
    return m1.src != m2.src
        || m1.dst != m2.dst
        || m1.promotionPiece != m2.promotionPiece;
}

static bool operator > (ChessMove m1, ChessMove m2)
{
    return m1.ordinal > m2.ordinal;
}
