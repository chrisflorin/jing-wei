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

#include <algorithm>
#include <cassert>

#include "attack.h"

#include "../../game/math/bitscan.h"
#include "../../game/math/bitreset.h"
#include "../../game/math/shift.h"

extern Bitboard WhitePawnCaptures[Square::SQUARE_COUNT];
extern Bitboard BlackPawnCaptures[Square::SQUARE_COUNT];

extern Bitboard PieceMoves[PieceType::PIECETYPE_COUNT][Square::SQUARE_COUNT];

extern Bitboard InBetween[Square::SQUARE_COUNT][Square::SQUARE_COUNT];

extern Evaluation MaterialParameters[PieceType::PIECETYPE_COUNT];

ChessAttackGenerator::ChessAttackGenerator()
{

}

ChessAttackGenerator::~ChessAttackGenerator()
{

}

Bitboard ChessAttackGenerator::getAttackingPieces(ChessBoard& board, Square dst, bool earlyExit, Bitboard attackThrough)
{
    bool whiteToMove = board.sideToMove == Color::WHITE;
    Bitboard attackingPieces = EmptyBitboard;

    Bitboard* otherPieces = whiteToMove ? board.blackPieces : board.whitePieces;

    //1) Check to see if a pawn or knight is doing attacking.  If so, enter them in the bitboard and return.
    Bitboard pawnAttacks = (whiteToMove ? WhitePawnCaptures[dst] : BlackPawnCaptures[dst]) & otherPieces[PieceType::PAWN];
    Bitboard knightAttacks = PieceMoves[PieceType::KNIGHT][dst] & otherPieces[PieceType::KNIGHT];
    Bitboard kingAttacks = PieceMoves[PieceType::KING][dst] & otherPieces[PieceType::KING];

    attackingPieces |= pawnAttacks | knightAttacks | kingAttacks;

    //Early exit is used for check detection.  If a pawn is attacking the king, another piece cannot be attacking it.
    //	It is rare enough for a knight to check a king and reveal a second attack from a rook/queen that we don't consider it.
    //	This assumption does not break the isInCheck or isSquareAttacked macro.
    if ((earlyExit) && (attackingPieces != EmptyBitboard)) { return attackingPieces; }

    //2) Check to see if a bishop or queen (diagonally) is doing the attacking.
    Bitboard bishopAttacks = PieceMoves[PieceType::BISHOP][dst] & (otherPieces[PieceType::BISHOP] | otherPieces[PieceType::QUEEN]);

    //Scan through our Bishop attacks to see if any of them are not blocked
    Square src;
    while (BitScanForward64((std::uint32_t *)&src, bishopAttacks)) {
        bishopAttacks = ResetLowestSetBit(bishopAttacks);

        if ((InBetween[dst][src] & (board.allPieces & ~attackThrough)) == EmptyBitboard) {
            attackingPieces |= OneShiftedBy(src);
        }
    }

    //If we want to exit early, go ahead and exit if an attack has been found.  The caller is simply looking for any
    //	attack on this square, not necessarily all of them.
    if ((earlyExit) && (attackingPieces != EmptyBitboard)) { return attackingPieces; }

    //3) Check to see if a rook or queen (straight) is doing the attacking.
    Bitboard rookAttacks = PieceMoves[PieceType::ROOK][dst] & (otherPieces[PieceType::ROOK] | otherPieces[PieceType::QUEEN]);

    //Scan through our Rook attacks to see if any of them are not blocked
    while (BitScanForward64((std::uint32_t *)&src, rookAttacks)) {
        rookAttacks = ResetLowestSetBit(rookAttacks);

        if ((InBetween[dst][src] & (board.allPieces & ~attackThrough)) == EmptyBitboard) {
            attackingPieces |= OneShiftedBy(src);
        }
    }

    //4) Return the bitboard.
    return attackingPieces;
}

bool ChessAttackGenerator::isInCheck(ChessBoard& board, bool otherSide)
{
    bool whiteToMove = board.sideToMove == Color::WHITE;

    if (otherSide) {
        whiteToMove = !whiteToMove;
    }

    if (whiteToMove == (board.sideToMove == Color::WHITE)) {
        return board.checkingPieces != EmptyBitboard;
    }

    Square kingPosition = whiteToMove ? board.whiteKingPosition : board.blackKingPosition;

    Bitboard* otherPieces = whiteToMove ? board.blackPieces : board.whitePieces;

    //This may be confusing at first.  These captures are compared against the opposite side pawns to see if they can "capture" the king
    Bitboard ourPawnAttacks = whiteToMove ? WhitePawnCaptures[kingPosition] : BlackPawnCaptures[kingPosition];

    //Do an easy check to see if a knight is giving check
    if ((PieceMoves[PieceType::KNIGHT][kingPosition] & otherPieces[PieceType::KNIGHT]) != EmptyBitboard) {
        return true;
    }

    //Do an easy check to see if a pawn is giving check
    if ((ourPawnAttacks & otherPieces[PieceType::PAWN]) != EmptyBitboard) {
        return true;
    }

    Bitboard bishops = otherPieces[PieceType::BISHOP] | otherPieces[PieceType::QUEEN];
    //Sliding pieces are different.  We must test the rook moves for bishop/queen checks and bishop moves for rook/queen checks.
    Bitboard boardMoves = PieceMoves[PieceType::BISHOP][kingPosition] & bishops;

    Square dst;
    while (BitScanForward64((std::uint32_t *)&dst, boardMoves)) {
        boardMoves = ResetLowestSetBit(boardMoves);

        if ((InBetween[kingPosition][dst] & board.allPieces) == EmptyBitboard) {
            return true;
        }
    }

    Bitboard rooks = otherPieces[PieceType::ROOK] | otherPieces[PieceType::QUEEN];
    boardMoves = PieceMoves[PieceType::ROOK][kingPosition] & rooks;

    while (BitScanForward64((std::uint32_t *)&dst, boardMoves)) {
        boardMoves = ResetLowestSetBit(boardMoves);

        //If there are no pieces between the current sliding move and the king
        if (((InBetween[kingPosition][dst] & board.allPieces) == EmptyBitboard)) {
            return true;
        }
    }

    return false;
}

bool ChessAttackGenerator::isSquareAttacked(ChessBoard& board, Square dst)
{
    bool whiteToMove = board.sideToMove == Color::WHITE;
    Bitboard kingBitboard = whiteToMove ? board.whitePieces[PieceType::KING] : board.blackPieces[PieceType::KING];

    return this->getAttackingPieces(board, dst, false, kingBitboard) != EmptyBitboard;
}
