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

#include "player.h"

#include "../../game/personality/parametermap.h"

#include "../endgame/endgame.h"

#include "../eval/parameters.h"

#include "../hash/hash.h"

extern ParameterMap chessEngineParameterMap;

ChessPlayer::ChessPlayer()
{
    this->currentBoard = 0;
    this->parameterMap = chessEngineParameterMap;

    InitializeHashValues();
    InitializeParameters();
}

ChessPlayer::~ChessPlayer()
{

}

void ChessPlayer::applyPersonalityImplementation(bool strip)
{
    InitializeParameters();

    BoardType& board = this->getCurrentBoard();

    board.materialEvaluation = board.calculateMaterialEvaluation();
    board.pstEvaluation = board.calculatePstEvaluation();

}

TwoPlayerGameResult ChessPlayer::checkBoardGameResultImplementation(BoardType& board)
{
    return this->searcher.checkBoardGameResult(board, this->moveHistory, true);
}

void ChessPlayer::getMoveImplementation(MoveType& move)
{
    BoardType board = this->getCurrentBoard();
    ChessPrincipalVariation principalVariation;

    this->searcher.setClock(this->clock);

    this->searcher.iterativeDeepeningLoop(board, principalVariation);

    move = principalVariation[0];
}

NodeCount ChessPlayer::perft(Depth depth)
{
    BoardType board = this->getCurrentBoard();
    return this->moveGenerator.perft(board, depth, Depth::ONE);
}

void ChessPlayer::resetHashtable()
{
    this->searcher.resetHashtable();
}
