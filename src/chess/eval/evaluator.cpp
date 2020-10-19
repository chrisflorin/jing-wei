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

#include "evaluator.h"

#include "../endgame/function.h"

#include "../types/bitboard.h"
#include "../types/score.h"

#include "../../game/math/bitreset.h"
#include "../../game/math/bitscan.h"
#include "../../game/math/popcount.h"

extern Bitboard PieceMoves[PieceType::PIECETYPE_COUNT][Square::SQUARE_COUNT];

extern Bitboard InBetween[Square::SQUARE_COUNT][Square::SQUARE_COUNT];

extern Bitboard BlackPawnCaptures[Square::SQUARE_COUNT];
extern Bitboard WhitePawnCaptures[Square::SQUARE_COUNT];

extern Bitboard bbFile[File::FILE_COUNT];

extern Evaluation AttackParameters[PieceType::PIECETYPE_COUNT][PieceType::PIECETYPE_COUNT]; 
extern Evaluation DoubledRooks;
extern Evaluation EmptyFileQueen;
extern Evaluation EmptyFileRook;
extern Evaluation GoodBishopPawns[8];
extern Evaluation BetterMobilityParameters[PieceType::PIECETYPE_COUNT][32];
extern Evaluation BoardControlPstParameters[Square::SQUARE_COUNT];
extern Evaluation KingControlPstParameters[Square::SQUARE_COUNT];
extern Evaluation MobilityParameters[PieceType::PIECETYPE_COUNT][32];
extern Evaluation PiecePairs[PieceType::PIECETYPE_COUNT];
extern Evaluation QueenBehindPassedPawnPst[Square::SQUARE_COUNT];
extern Evaluation RookBehindPassedPawnPst[Square::SQUARE_COUNT];
extern Evaluation SafeMobilityParameters[PieceType::PIECETYPE_COUNT][32];
extern Evaluation TropismParameters[PieceType::PIECETYPE_COUNT][16];

extern std::uint32_t Distance[File::FILE_COUNT][Rank::RANK_COUNT];

ChessEvaluator::ChessEvaluator()
{
    InitializeEndgame(this->endgame);
}

ChessEvaluator::~ChessEvaluator()
{

}

bool ChessEvaluator::checkBoardForInsufficientMaterial(BoardType& board)
{
    std::uint32_t pieceCount = popCount(board.allPieces);

    switch (pieceCount) {
    case 2:
        return true;
    case 3:
        if ((board.whitePieces[PieceType::KNIGHT] | board.whitePieces[PieceType::BISHOP] | board.blackPieces[PieceType::KNIGHT] | board.blackPieces[PieceType::BISHOP]) != EmptyBitboard) {
            return true;
        }
        break;
    case 4:
        if (popCount(board.whitePieces[PieceType::KNIGHT]) == 2) {
            return true;
        }

        if (popCount(board.blackPieces[PieceType::KNIGHT]) == 2) {
            return true;
        }

        if (popCountIsOne(board.whitePieces[PieceType::BISHOP]) && popCountIsOne(board.blackPieces[PieceType::BISHOP])) {
            if (SameColorAsPiece(board.whitePieces[PieceType::BISHOP], board.blackPieces[PieceType::BISHOP]) != EmptyBitboard) {
                return true;
            }
        }

        break;
    }

    return false;
}

Score ChessEvaluator::evaluateImplementation(BoardType& board, Score alpha, Score beta)
{
    bool whiteToMove = board.sideToMove == Color::WHITE;

    //1) Check for end game score
    Score endgameScore;

    std::int32_t pieceCount = popCount(board.allPieces);
    if (pieceCount <= 5) {
        bool endgameFound = this->endgame.probe(board, endgameScore);

        if (endgameFound) {
            return endgameScore;
        }
    }

    //2) Check for lone king
    else if (popCount(board.whitePieces[PieceType::ALL]) == 1
        || popCount(board.blackPieces[PieceType::ALL]) == 1) {
        weakKingEndgameFunction(board, endgameScore);
        return endgameScore;
    }

    //3) Check for lazy evaluation
    Score lazyEvaluation = this->lazyEvaluate(board);

    constexpr Score LazyThreshold = Score(4 * PAWN_SCORE);
    if (lazyEvaluation + LazyThreshold < alpha
        || lazyEvaluation - LazyThreshold >= beta) {
        return lazyEvaluation;
    }

    //4) Continue, actually evaluating the board
    EvaluationTable evaluationTable;
    Evaluation evaluation = board.materialEvaluation + board.pstEvaluation;

    for (Color color = Color::WHITE; color < Color::COLOR_COUNT; color++) {
        bool colorIsWhite = color == Color::WHITE;
        std::int32_t multiplier = colorIsWhite ? 1 : -1;

        Bitboard* piecesToMove = colorIsWhite ? board.whitePieces : board.blackPieces;
        Bitboard* otherPieces = colorIsWhite ? board.blackPieces : board.whitePieces;

        Direction left = colorIsWhite ? Direction::DOWN_LEFT : Direction::UP_LEFT;
        Direction right = colorIsWhite ? Direction::DOWN_RIGHT : Direction::UP_RIGHT;

        Bitboard unsafeSquares = ((otherPieces[PieceType::PAWN] & ~bbFile[File::_A]) + left) | ((otherPieces[PieceType::PAWN] & ~bbFile[File::_H]) + right);
        evaluationTable.Attacks[~color][PieceType::PAWN] = unsafeSquares;

        for (PieceType pieceType = PieceType::PAWN; pieceType <= PieceType::QUEEN; pieceType++) {
            Bitboard srcPieces = piecesToMove[pieceType];
            bool hasPiecePair = false;

            if (pieceType != PieceType::PAWN
                && popCount(srcPieces) > 1) {
                evaluation += multiplier * PiecePairs[pieceType];
                hasPiecePair = true;
            }

            Square src;
            while (BitScanForward64((std::uint32_t *)&src, srcPieces)) {
                srcPieces = ResetLowestSetBit(srcPieces);

                Bitboard* pieceMoves = pieceType != PieceType::PAWN ? PieceMoves[pieceType] :
                    (color == Color::WHITE ? WhitePawnCaptures : BlackPawnCaptures);

                Bitboard dstSquares = PieceMoves[pieceType][src];
                Bitboard mobilityDstSquares;
                std::int32_t mobility;

                Square otherKingPosition = color == Color::WHITE ? board.blackKingPosition : board.whiteKingPosition;
                if (pieceType != PieceType::PAWN) {
                    evaluation += multiplier * this->evaluateMobility(evaluationTable, mobilityDstSquares, board.allPieces, dstSquares, unsafeSquares, mobility, color, pieceType, src);
                }

                dstSquares &= otherPieces[PieceType::ALL];

                Square dst;
                while (BitScanForward64((std::uint32_t *)&dst, dstSquares)) {
                    dstSquares = ResetLowestSetBit(dstSquares);

                    if ((InBetween[src][dst] & board.allPieces) == EmptyBitboard) {
                        PieceType attackedPiece = board.pieces[dst];
                        evaluation += multiplier * this->evaluateAttacks(pieceType, attackedPiece);
                    }
                }

                if (pieceType > PieceType::PAWN) {
                    evaluation += multiplier * this->evaluateTropism(pieceType, src, otherKingPosition);

                    Bitboard passedPawns = this->pawnEvaluator.getPassedPawns(color);

                    switch (pieceType) {
                    case PieceType::BISHOP:
                        evaluation += multiplier * this->evaluateBishop(otherPieces, src, hasPiecePair);
                        break;
                    case PieceType::ROOK:
                        evaluation += multiplier * this->evaluateRook(piecesToMove, board.allPieces, passedPawns, src, hasPiecePair);
                        break;
                    case PieceType::QUEEN:
                        evaluation += multiplier * this->evaluateQueen(board.allPieces, passedPawns, src);
                    }
                }
            }
        }
    }

    //5) Evaluate Board Control
    evaluation += this->evaluateBoardControl(board, evaluationTable);

    //6) Evaluate Mobility difference
    for (PieceType pieceType = PieceType::KNIGHT; pieceType <= PieceType::QUEEN; pieceType++) {
        std::int32_t betterMobility = evaluationTable.Mobility[Color::WHITE][pieceType] - evaluationTable.Mobility[Color::BLACK][pieceType];
        std::int32_t multiplier = 1;

        if (betterMobility < 0) {
            multiplier = -1;
        }

        evaluation += multiplier * BetterMobilityParameters[pieceType][multiplier * betterMobility];
    }

    //7) Begin Result Calculation
    Score result = ((evaluation.mg * pieceCount) + (evaluation.eg * (32 - pieceCount))) / 32;
    result = whiteToMove ? result : -result;

    //8) Evaluate Pawn Structure.  Since the Pawn Evaluator is another evaluator, it will return score with side to move
    result += this->pawnEvaluator.evaluate(board, alpha, beta);

    return result;
}

Evaluation ChessEvaluator::evaluateAttacks(PieceType srcPiece, PieceType attackedPiece)
{
    return AttackParameters[srcPiece][attackedPiece];
}

Evaluation ChessEvaluator::evaluateBoardControl(BoardType& board, EvaluationTable& evaluationTable)
{
    Bitboard whiteControl = EmptyBitboard;
    Bitboard blackControl = EmptyBitboard;

    //1) Calculate which squares are controlled by which piece on which side
    for (PieceType pieceType = PieceType::PAWN; pieceType < PieceType::KING; pieceType++) {
        Bitboard allControl = whiteControl | blackControl;

        Bitboard whiteAttacks = ~allControl & evaluationTable.Attacks[Color::WHITE][pieceType];
        Bitboard blackAttacks = ~allControl & evaluationTable.Attacks[Color::BLACK][pieceType];

        Bitboard commonAttacks = whiteAttacks & blackAttacks;

        whiteAttacks = whiteAttacks & ~commonAttacks;
        blackAttacks = blackAttacks & ~commonAttacks;

        whiteControl = whiteControl | whiteAttacks;
        blackControl = blackControl | blackAttacks;

        evaluationTable.WhiteControl[pieceType] = whiteAttacks;
        evaluationTable.BlackControl[pieceType] = blackAttacks;
    }

    assert((whiteControl & blackControl) == EmptyBitboard);

    evaluationTable.WhiteControl[PieceType::ALL] = whiteControl;
    evaluationTable.BlackControl[PieceType::ALL] = blackControl;

    //2) Evaluate Board and King Square Control
    Bitboard whiteKingControl = whiteControl & PieceMoves[PieceType::KING][board.blackKingPosition];
    Bitboard blackKingControl = blackControl & PieceMoves[PieceType::KING][board.whiteKingPosition];

    Evaluation result = { ZERO_SCORE, ZERO_SCORE };

    Square dst;
    for (Color color = Color::WHITE; color < Color::COLOR_COUNT; color++) {
        bool colorIsWhite = color == Color::WHITE;
        std::int32_t multiplier = colorIsWhite ? 1 : -1;

        Bitboard colorControl = colorIsWhite ? whiteControl : blackControl;
        while (BitScanForward64((std::uint32_t*) & dst, colorControl)) {
            colorControl = ResetLowestSetBit(colorControl);

            dst = colorIsWhite ? dst : FlipSqY(dst);
            result += multiplier * BoardControlPstParameters[dst];
        }

        Bitboard colorKingControl = colorIsWhite ? whiteKingControl : blackKingControl;
        while (BitScanForward64((std::uint32_t*) & dst, colorKingControl)) {
            colorKingControl = ResetLowestSetBit(colorKingControl);

            dst = colorIsWhite ? dst : FlipSqY(dst);
            result += multiplier * KingControlPstParameters[dst];
        }
    }

    return result;
}

Evaluation ChessEvaluator::evaluateMobility(EvaluationTable& evaluationTable, Bitboard& outDstSquares, Bitboard allPieces, Bitboard dstSquares, Bitboard unsafeSquares, std::int32_t& mobility, Color movingSide, PieceType pieceType, Square src)
{
    outDstSquares = dstSquares;

    switch (pieceType) {
    case PieceType::KNIGHT:
        break;
    case PieceType::BISHOP:
    case PieceType::ROOK:
    case PieceType::QUEEN:
        Square dst;

        while (dstSquares != EmptyBitboard) {
            BitScanForward64((std::uint32_t *)&dst, dstSquares);
            dstSquares = ResetLowestSetBit(dstSquares);

            if ((InBetween[src][dst] & allPieces) != EmptyBitboard) {
                outDstSquares ^= dst;
            }
        }

        break;
    default:
        return { ZERO_SCORE, ZERO_SCORE };
    }

    evaluationTable.Attacks[movingSide][pieceType] = outDstSquares;
    mobility = popCount(outDstSquares);

    Bitboard safeDstSquares = outDstSquares & ~unsafeSquares;
    std::int32_t safeMobility = popCount(safeDstSquares);

    evaluationTable.Mobility[movingSide][pieceType] = mobility;
    evaluationTable.SafeMobility[movingSide][pieceType] = safeMobility;

    return MobilityParameters[pieceType][mobility] + SafeMobilityParameters[pieceType][safeMobility];
}

Evaluation ChessEvaluator::evaluateTropism(PieceType pieceType, Square src, Square otherKingPosition)
{
    std::uint32_t tropism = Distance[FileDistance(otherKingPosition, src)][RankDistance(otherKingPosition, src)];

    return TropismParameters[pieceType][tropism];
}

Evaluation ChessEvaluator::evaluateBishop(Bitboard* otherPieces, Square src, bool hasPiecePair)
{
    Evaluation result = { ZERO_SCORE, ZERO_SCORE };

    if (!hasPiecePair) {
        std::int32_t goodPawnCount = popCount(SquaresSameColorAs(otherPieces[PieceType::PAWN], src));
        std::int32_t badPawnCount = popCount(SquaresOppositeColorAs(otherPieces[PieceType::PAWN], src));

        if (goodPawnCount > badPawnCount) {
            result += GoodBishopPawns[goodPawnCount - badPawnCount];
        }
        else if (goodPawnCount < badPawnCount) {
            result -= GoodBishopPawns[badPawnCount - goodPawnCount];
        }
    }

    return result;
}

Evaluation ChessEvaluator::evaluateRook(Bitboard* piecesToMove, Bitboard allPieces, Bitboard passedPawns, Square src, bool hasPiecePair)
{
    Evaluation result = { ZERO_SCORE, ZERO_SCORE };

    if (hasPiecePair) {
        Bitboard otherRooks = piecesToMove[PieceType::ROOK] & PieceMoves[PieceType::ROOK][src];
        Square dst;

        if (otherRooks != EmptyBitboard) {
            while (otherRooks != EmptyBitboard) {
                BitScanForward64((std::uint32_t*) & dst, otherRooks);
                otherRooks = ResetLowestSetBit(otherRooks);

                if ((InBetween[src][dst] & allPieces) != EmptyBitboard) {
                    result += DoubledRooks;
                }
            }
        }
    }

    File file = getFile(src);
    Bitboard piecesInSameFile = allPieces & bbFile[file];

    if (piecesInSameFile == OneShiftedBy(src)) {
        result += EmptyFileRook;
    }
    else if ((piecesInSameFile & passedPawns) != EmptyBitboard) {
        Square src;
        while (passedPawns != EmptyBitboard) {
            BitScanForward64((std::uint32_t*) & src, passedPawns);
            passedPawns = ResetLowestSetBit(passedPawns);

            result += RookBehindPassedPawnPst[src];
        }
    }

    return result;
}

Evaluation ChessEvaluator::evaluateQueen(Bitboard allPieces, Bitboard passedPawns, Square src)
{
    Evaluation result = { ZERO_SCORE, ZERO_SCORE };

    File file = getFile(src);
    Bitboard piecesInSameFile = allPieces & bbFile[file];

    if (piecesInSameFile == OneShiftedBy(src)) {
        result += EmptyFileQueen;
    }
    else if ((piecesInSameFile & passedPawns) != EmptyBitboard) {
        Square src;
        while (passedPawns != EmptyBitboard) {
            BitScanForward64((std::uint32_t*) & src, passedPawns);
            passedPawns = ResetLowestSetBit(passedPawns);

            result += QueenBehindPassedPawnPst[src];
        }
    }

    return result;
}

Score ChessEvaluator::lazyEvaluateImplementation(BoardType& board)
{
    Evaluation evaluation = board.materialEvaluation + board.pstEvaluation;

    std::int32_t pieceCount = popCount(board.allPieces);

    Score result = ((evaluation.mg * pieceCount) + (evaluation.eg * (32 - pieceCount))) / 32;

    bool whiteToMove = board.sideToMove == Color::WHITE;

    return whiteToMove ? result : -result;
}
