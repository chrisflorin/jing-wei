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

#include "pawnevaluator.h"

#include "../board/board.h"

#include "../endgame/endgame.h"

#include "../../game/eval/evaluator.h"

struct EvaluationTable
{
    Bitboard Attacks[Color::COLOR_COUNT][PieceType::PIECETYPE_COUNT];

    Bitboard WhiteControl[PieceType::PIECETYPE_COUNT];
    Bitboard BlackControl[PieceType::PIECETYPE_COUNT];

    std::int32_t Mobility[Color::COLOR_COUNT][PieceType::PIECETYPE_COUNT];
    std::int32_t SafeMobility[Color::COLOR_COUNT][PieceType::PIECETYPE_COUNT];

    EvaluationTable(): Attacks(), WhiteControl(), BlackControl(), Mobility(), SafeMobility()
    {
    }
};

class ChessEvaluator : public Evaluator<ChessEvaluator, ChessBoard>
{
    ChessEndgame endgame;
    ChessPawnEvaluator pawnEvaluator;

    Evaluation evaluateAttacks(PieceType srcPiece, PieceType attackedPiece);
    Evaluation evaluateBoardControl(BoardType& board, EvaluationTable& evaluationTable);
    Evaluation evaluateMobility(EvaluationTable& evaluationTable, Bitboard& outDstSquares, Bitboard allPieces, Bitboard dstSquares, Bitboard unsafeSquares, std::int32_t& mobility, Color movingSide, PieceType pieceType, Square src);
    Evaluation evaluateTropism(PieceType pieceType, Square src, Square otherKingPosition);

    Evaluation evaluateBishop(Bitboard* otherPieces, Square src, bool hasPiecePair);
    Evaluation evaluateRook(Bitboard* piecesToMove, Bitboard allPieces, Bitboard passedPawns, Square src, bool hasPiecePair);
    Evaluation evaluateQueen(Bitboard allPieces, Bitboard passedPawns, Square src);
public:
	ChessEvaluator();
	~ChessEvaluator();

    bool checkBoardForInsufficientMaterial(BoardType& board);

	Score evaluateImplementation(BoardType& board, Score alpha, Score beta);

	Score lazyEvaluateImplementation(BoardType& board);
};
