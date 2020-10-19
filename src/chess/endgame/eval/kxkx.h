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

//Keep in mind here, even though the "strong side" has more value in material, the strong side cannot win and must prevent the pawn from advancing
static bool knkp(ChessBoard& board, Score& score)
{
    //1) Determine strong side
    const Color strongSide = FindStrongSide(board);

    //2) Get pst values for KN and KP
    Score pst = board.pstEvaluation.eg;

    //psts are relative to white.  If strong side is Black, we have to negate.
    if (strongSide == Color::BLACK) {
        pst = -pst;
    }

    score = pst;

    //3) If we're returning the strong side can win, bias toward "weak" side
    if (score > DRAW_SCORE) {
        score = DRAW_SCORE - 1;
    }

    //6) Ensure score is returned for side to move
    if (board.sideToMove != strongSide) {
        score = -score;
    }

    return true;
}

static ChessEndgame::EndgameFunctionType knkn = drawEndgameFunction;

static ChessEndgame::EndgameFunctionType kbkp = knkp;
static ChessEndgame::EndgameFunctionType kbkn = drawEndgameFunction;
static ChessEndgame::EndgameFunctionType kbkb = drawEndgameFunction;

static ChessEndgame::EndgameFunctionType krkp = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType krkn = drawEndgameFunction;
static ChessEndgame::EndgameFunctionType krkb = drawEndgameFunction;
static ChessEndgame::EndgameFunctionType krkr = drawEndgameFunction;

static ChessEndgame::EndgameFunctionType kqkp = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType kqkn = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType kqkb = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType kqkr = drawEndgameFunction;
static ChessEndgame::EndgameFunctionType kqkq = drawEndgameFunction;
