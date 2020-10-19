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

#include <cassert>
#include <cstdint>

#include "pawnevaluator.h"

#include "../../game/math/bitreset.h"
#include "../../game/math/bitscan.h"
#include "../../game/math/byteswap.h"
#include "../../game/math/popcount.h"

extern Bitboard bbFile[File::FILE_COUNT];

extern Bitboard BlackPawnCaptures[Square::SQUARE_COUNT];
extern Bitboard WhitePawnCaptures[Square::SQUARE_COUNT];

extern Evaluation PawnChainBackPstParameters[Square::SQUARE_COUNT];
extern Evaluation PawnChainFrontPstParameters[Square::SQUARE_COUNT];
extern Evaluation PawnDoubledPstParameters[Square::SQUARE_COUNT];
extern Evaluation PawnPassedPstParameters[Square::SQUARE_COUNT];
extern Evaluation PawnTripledPstParameters[Square::SQUARE_COUNT];

extern Bitboard PassedPawnCheck[Square::SQUARE_COUNT];
extern Bitboard SquaresInFront[Square::SQUARE_COUNT];

ChessPawnEvaluator::ChessPawnEvaluator()
{

}

ChessPawnEvaluator::~ChessPawnEvaluator()
{

}

Score ChessPawnEvaluator::evaluateImplementation(ChessBoard& board, Score alpha, Score beta)
{
	constexpr Rank lastRank = Rank::_8;

	Evaluation evaluation = { ZERO_SCORE, ZERO_SCORE };

	this->evaluatePawnChain(evaluation, board);

	for (Color color = Color::COLOR_START; color < Color::COLOR_COUNT; color++) {
		bool colorIsWhite = color == Color::WHITE;
		int multiplier = colorIsWhite ? 1 : -1;

		Bitboard* colorPieces = colorIsWhite ? board.whitePieces : board.blackPieces;
		Bitboard* otherPieces = colorIsWhite ? board.blackPieces : board.whitePieces;

		Bitboard colorPawns = colorPieces[PieceType::PAWN];
		Bitboard otherPawns = otherPieces[PieceType::PAWN];

		Bitboard passedPawns = EmptyBitboard;

		Square src;
		Bitboard loopingPawns = colorPawns;
		while (BitScanForward64((std::uint32_t*) & src, loopingPawns)) {
			loopingPawns = ResetLowestSetBit(loopingPawns);

			Square evaluatedSquare = colorIsWhite ? src : FlipSqY(src);

			File evaluatedSrcFile = getFile(evaluatedSquare);
			Rank evaluatedSrcRank = getRank(evaluatedSquare);

			Bitboard evaluatedColorPawns = colorIsWhite ? colorPawns : SwapBytes(colorPawns);
			Bitboard evaluatedOtherPawns = colorIsWhite ? otherPawns : SwapBytes(otherPawns);

			bool passed = false;

			if (((PassedPawnCheck[evaluatedSquare] & evaluatedOtherPawns) == EmptyBitboard)) {
				passed = true;

				passedPawns |= src;

				//2b) The pawn is unstoppable if it's closer to the back rank than the enemy king is to the pawn
				//if (Distance(evaluatedSrcRank, lastRank) < Distance(evaluatedSrcFile, getFile(otherKingPosition))) {
				//	evaluation += multiplier * UnstoppablePawnValues[evaluatedSquare];
				//}
				//else {
					evaluation += multiplier * PawnPassedPstParameters[evaluatedSquare];
				//}
			}

			Bitboard pawnsInFrontOfSrc = SquaresInFront[evaluatedSquare] & evaluatedColorPawns;
			if (pawnsInFrontOfSrc != EmptyBitboard) {
				if (popCountIsOne(pawnsInFrontOfSrc)) {
					evaluation += multiplier * PawnDoubledPstParameters[evaluatedSquare];
				}
				else {
					evaluation += multiplier * PawnTripledPstParameters[evaluatedSquare];
				}
			}
		}

		this->passedPawns[color] = passedPawns;
	}

	std::int32_t pieceCount = popCount(board.allPieces);
	Score result = ((evaluation.mg * pieceCount) + (evaluation.eg * (32 - pieceCount))) / 32;

	bool whiteToMove = board.sideToMove == Color::WHITE;
	return whiteToMove ? result : -result;
}

void ChessPawnEvaluator::evaluatePawnChain(Evaluation& evaluation, ChessBoard& board)
{
	Bitboard whitePawns = board.whitePieces[PieceType::PAWN];
	Bitboard blackPawns = board.blackPieces[PieceType::PAWN];

	Bitboard upLeft = (whitePawns & ~bbFile[File::_A]) >> (-Direction::UP_LEFT);
	Bitboard upRight = (whitePawns & ~bbFile[File::_H]) >> (-Direction::UP_RIGHT);

	Bitboard whitePawnChains = (upLeft | upRight) & whitePawns;

	while (whitePawnChains != EmptyBitboard) {
		Square dst;
		
		BitScanForward64((std::uint32_t*)& dst, whitePawnChains);
		whitePawnChains = ResetLowestSetBit(whitePawnChains);

		evaluation += PawnChainFrontPstParameters[dst];

		Bitboard backChainPawns = BlackPawnCaptures[dst] & whitePawns;
		while (backChainPawns != EmptyBitboard) {
			Square src;

			BitScanForward64((std::uint32_t*) & src, backChainPawns);
			backChainPawns = ResetLowestSetBit(backChainPawns);

			evaluation += PawnChainBackPstParameters[src];
		}
	}

	Bitboard downLeft = (blackPawns & ~bbFile[File::_A]) << Direction::DOWN_LEFT;
	Bitboard downRight = (blackPawns & ~bbFile[File::_H]) << Direction::DOWN_RIGHT;

	Bitboard blackPawnChains = (downLeft | downRight) & blackPawns;

	while (blackPawnChains != EmptyBitboard) {
		Square dst;

		BitScanForward64((std::uint32_t*)& dst, blackPawnChains);
		blackPawnChains = ResetLowestSetBit(blackPawnChains);

		dst = FlipSqY(dst);
		evaluation -= PawnChainFrontPstParameters[dst];

		Bitboard backChainPawns = WhitePawnCaptures[dst] & blackPawns;
		while (backChainPawns != EmptyBitboard) {
			Square src;

			BitScanForward64((std::uint32_t*) & src, backChainPawns);
			backChainPawns = ResetLowestSetBit(backChainPawns);

			src = FlipSqY(src);
			evaluation -= PawnChainBackPstParameters[src];
		}
	}
}

Bitboard ChessPawnEvaluator::getPassedPawns(Color color)
{
	return this->passedPawns[color];
}

Score ChessPawnEvaluator::lazyEvaluateImplementation(ChessBoard& board)
{
	return ZERO_SCORE;
}
