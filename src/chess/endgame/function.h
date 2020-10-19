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

#include <cassert>
#include <cmath>

#include "endgame.h"

#include "../board/board.h"

#include "../types/score.h"
#include "../types/square.h"

static Score GeneralMate[Square::SQUARE_COUNT] =
{
    5000, 4500, 4000, 3500, 3500, 4000, 4500, 5000,
    4500, 4000, 3500, 3000, 3000, 3500, 4000, 4500,
    4000, 3500, 3000, 2500, 2500, 3000, 3500, 4000,
    3500, 3000, 2500, 2000, 2000, 2500, 3000, 3500,
    3500, 3000, 2500, 2000, 2000, 2500, 3000, 3500,
    4000, 3500, 3000, 2500, 2500, 3000, 3500, 4000,
    4500, 4000, 3500, 3000, 3000, 3500, 4000, 4500,
    5000, 4500, 4000, 3500, 3500, 4000, 4500, 5000
};

static Score Proximity[11] = { 0, 0, 90, 80, 70, 60, 50, 40, 30, 20, 10 };

static bool drawEndgameFunction(ChessBoard& board, Score& score)
{
    score = DRAW_SCORE;

    return true;
}

static Color FindStrongSide(ChessBoard& board)
{
    //There's no need to calculate the actual material value based on the phase calculated score
    Score materialScore = board.materialEvaluation.eg;

    if (materialScore == DRAW_SCORE) {
        return board.sideToMove;
    }

    return materialScore > DRAW_SCORE ? Color::WHITE : Color::BLACK;
}

static bool nullEndgameFunction(ChessBoard& board, Score& score)
{
    return false;
}

template <Score baseScore = BASICALLY_WINNING_SCORE>
static bool weakKingEndgameFunction(ChessBoard& board, Score& score)
{
    //1) Determine strong side
    const Color strongSide = FindStrongSide(board);

    //2) Return value based on weak king's position
    Square weakKingPosition = strongSide == Color::WHITE ? board.blackKingPosition : board.whiteKingPosition;

    //3) Calculate king proximity.  The strong king being close to the weak king makes
    //  it easier to force it around
    File file = getFile(board.whiteKingPosition) - getFile(board.blackKingPosition);
    Rank rank = getRank(board.whiteKingPosition) - getRank(board.blackKingPosition);

    std::int32_t kingDistance = (std::int32_t)std::sqrt(file * file + rank * rank);

    //4) Account for other pieces being placed optimally
    Score pst = board.pstEvaluation.eg;

    //psts are relative to white.  If strong side is Black, we have to negate.
    if (strongSide == Color::BLACK) {
        pst = -pst;
    }

    //5) Put it all together for the strong side
    score = baseScore + GeneralMate[weakKingPosition] + Proximity[kingDistance] + pst;

    //6) Ensure score is returned for side to move
    if (board.sideToMove != strongSide) {
        score = -score;
    }

    return true;
}

static bool weakKingDrawishEndgameFunction(ChessBoard& board, Score& score)
{
    //1) Determine strong side
    const Color strongSide = FindStrongSide(board);

    //2) Return value based on weak king's position
    Square weakKingPosition = strongSide == Color::WHITE ? board.blackKingPosition : board.whiteKingPosition;

    //3) Calculate king proximity.  The strong king being close to the weak king makes
    //  it easier to force it around
    File file = getFile(board.whiteKingPosition) - getFile(board.blackKingPosition);
    Rank rank = getRank(board.whiteKingPosition) - getRank(board.blackKingPosition);

    std::int32_t kingDistance = (std::int32_t)std::sqrt(file * file + rank * rank);

    //4) Account for other pieces being places optimally
    Score pst = board.pstEvaluation.eg;

    //psts are relative to white.  If strong side is Black, we have to negate.
    if (strongSide == Color::BLACK) {
        pst = -pst;
    }

    //5) Put it all together for the strong side
    score = DRAW_SCORE + Proximity[kingDistance] + pst;

    //6) Ensure score is returned for side to move
    if (board.sideToMove != strongSide) {
        score = -score;
    }

    return true;
}
