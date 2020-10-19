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
#include <iostream>

#include "../../game/math/bitreset.h"
#include "../../game/math/bitscan.h"
#include "../../game/math/popcount.h"
#include "../../game/math/shift.h"
#include "../../game/math/sort.h"

#include "../../game/types/movelist.h"

#include "movegen.h"

#include "moves.h"

static constexpr bool enableButterflyTable = true;

extern Bitboard bbFile[File::FILE_COUNT];

extern Bitboard WhitePawnMoves[Square::SQUARE_COUNT];
extern Bitboard WhitePawnCaptures[Square::SQUARE_COUNT];
extern Bitboard BlackPawnMoves[Square::SQUARE_COUNT];
extern Bitboard BlackPawnCaptures[Square::SQUARE_COUNT];
extern Bitboard PieceMoves[PieceType::PIECETYPE_COUNT][Square::SQUARE_COUNT];
extern Bitboard InBetween[Square::SQUARE_COUNT][Square::SQUARE_COUNT];

extern Evaluation MaterialParameters[PieceType::PIECETYPE_COUNT];

static ChessPrincipalVariation principalVariation;
static MoveList<ChessMoveGenerator::MoveType> PerftMoveList[Depth::MAX];

ChessMoveGenerator::ChessMoveGenerator()
{
    SetupInBetweenBoard();
    SetupPassedPawnCheckBoard();
}

ChessMoveGenerator::~ChessMoveGenerator()
{

}

NodeCount ChessMoveGenerator::doubleCheckGeneratedMoves(BoardType& board, MoveList<MoveType>& moveList)
{
    NodeCount moveCount = moveList.size();

    MoveList<MoveType>::iterator it = moveList.begin();
    while (it != moveList.end()) {
        MoveType& move = (*it);

        Square src = move.src;
        Square dst = move.dst;

        bool isPinnedPiece = (board.pinnedPieces & OneShiftedBy(src)) != EmptyBitboard;
        bool isEnPassant = board.pieces[src] == PieceType::PAWN && board.enPassant == dst;

        if (isPinnedPiece || isEnPassant) {
            BoardType nextBoard = board;
            nextBoard.doMove(move);

            if (this->attackGenerator.isInCheck(nextBoard, true)) {
                it = moveList.erase(it);
            }
            else {
                ++it;
            }
        }
        else {
            ++it;
        }
    }

    return moveList.size();
}

NodeCount ChessMoveGenerator::generateAllCaptures(BoardType& board, MoveList<MoveType>& moveList)
{
    //1) If the side to move is in check, there's a highly optimized algorithm for generating just evasions
    if (this->attackGenerator.isInCheck(board)) {
        return this->generateCheckEvasions(board, moveList);
    }

    //2) Continue on with normal capture generation
    moveList.clear();

    bool whiteToMove = board.sideToMove == Color::WHITE;

    //The only pieces we can safely move are our own pieces which are not pinned.  Special care must be taken to move
    //	pinned pieces (this is at the end).
    Bitboard* piecesToMove = whiteToMove ? board.whitePieces : board.blackPieces;
    Bitboard* otherPieces = whiteToMove ? board.blackPieces : board.whitePieces;

    Bitboard srcPieces = piecesToMove[PieceType::ALL];
    Bitboard dstMoves;

    Square src, dst;

    while (BitScanForward64((std::uint32_t*) & src, srcPieces)) {
        srcPieces = ResetLowestSetBit(srcPieces);

        PieceType movingPiece = board.pieces[src];

        if (movingPiece == PieceType::PAWN) {
            dstMoves = whiteToMove ? WhitePawnCaptures[src] : BlackPawnCaptures[src];

            //Special en passant processing here (add the move to dstMoves)
            if (board.enPassant != Square::NO_SQUARE) {
                if (((whiteToMove ? WhitePawnCaptures[src] : BlackPawnCaptures[src]) & OneShiftedBy(board.enPassant)) != EmptyBitboard) {
                    dstMoves |= OneShiftedBy(board.enPassant);
                }
            }
        }
        else {
            dstMoves = PieceMoves[movingPiece][src];
        }

        //Don't allow us to capture our own pieces
        dstMoves &= otherPieces[PieceType::ALL];

        //If this piece is pinned, it's destination moves can only be other squares in between attackers (in this case, blocked pieces)
        if ((OneShiftedBy(src) & board.pinnedPieces) != EmptyBitboard) {
            dstMoves &= board.blockedPieces;
        }

        while (BitScanForward64((std::uint32_t*) & dst, dstMoves)) {
            dstMoves = ResetLowestSetBit(dstMoves);

            switch (movingPiece) {
            case PieceType::PAWN:
            {
                Rank rank = getRank(dst);

                if (rank == (whiteToMove ? Rank::_8 : Rank::_1)) {
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::QUEEN });
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::ROOK });
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::BISHOP });
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::KNIGHT });
                }
                else {
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::NO_PIECE });
                }
            }
            break;
            case PieceType::KNIGHT:
                //We know this is either an empty space or a capture move; there can't be anything in between so allow the move;
                moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::NO_PIECE });
                break;
            case PieceType::KING:
                //Can't move king into check
                if (!this->attackGenerator.isSquareAttacked(board, dst)) {
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::NO_PIECE });
                }
                break;
            default:
                //We know this is either an empty space or a capture move; just make sure there's nothing in between
                if ((board.allPieces & InBetween[src][dst]) == EmptyBitboard) {
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::NO_PIECE });
                }
            }
        }
    }

    //3) There are very rare instances where a pinned piece move or en passant move is errantly generated.  Resort to manual double checking of those moves.
    if (this->shouldDoubleCheckGeneratedMoves(board)) {
        this->doubleCheckGeneratedMoves(board, moveList);
    }

    return moveList.size();
}

NodeCount ChessMoveGenerator::generateAllMovesImplementation(BoardType& board, MoveList<MoveType>& moveList, bool countOnly)
{
    //1) If the side to move is in check, there's a highly optimized algorithm for generating just evasions
    if (this->attackGenerator.isInCheck(board)) {
        return this->generateCheckEvasions(board, moveList);
    }

    //If there's a special case, we have to verify moves at the end
    if (countOnly) {
        if (board.pinnedPieces != EmptyBitboard
            || board.enPassant != Square::NO_SQUARE) {
            countOnly = false;
        }
    }

    //2) Continue on with normal move generation
    if (!countOnly) {
        moveList.clear();
    }

    NodeCount moveCount = ZeroNodes;

    bool whiteToMove = board.sideToMove == Color::WHITE;

    Bitboard piecesToMove = whiteToMove ? board.whitePieces[PieceType::ALL] : board.blackPieces[PieceType::ALL];
    Bitboard* otherPieces = whiteToMove ? board.blackPieces : board.whitePieces;

    Bitboard srcPieces = piecesToMove;
    Bitboard dstMoves;

    Square src, dst;

    while (BitScanForward64((std::uint32_t *)&src, srcPieces)) {
        srcPieces = ResetLowestSetBit(srcPieces);

        PieceType movingPiece = board.pieces[src];
        dstMoves = PieceMoves[movingPiece][src];

        switch (movingPiece) {
        case PieceType::PAWN:
            dstMoves = (whiteToMove ? WhitePawnMoves[src] : BlackPawnMoves[src]) & ~board.allPieces;
            dstMoves |= (whiteToMove ? WhitePawnCaptures[src] : BlackPawnCaptures[src]) & otherPieces[PieceType::ALL];

            //If we're advancing by 2, make sure we're not blocked
            if (getRank(src) == (whiteToMove ? Rank::_2 : Rank::_7)) {
                dstMoves &= ~(whiteToMove ? (board.allPieces & 0x0000ff0000000000ull) >> 8 : (board.allPieces & 0x0000000000ff0000ull) << 8);
            }

            //Special en passant processing here (add the move to dstMoves)
            if (board.enPassant != Square::NO_SQUARE) {
                if (((whiteToMove ? WhitePawnCaptures[src] : BlackPawnCaptures[src]) & OneShiftedBy(board.enPassant)) != EmptyBitboard) {
                    dstMoves |= OneShiftedBy(board.enPassant);
                }
            }

            break;
        case PieceType::KING:
            //Special castle processing here
            //We don't have to do the IsInCheck check because if the king is in check, a specialized function is called for it.
            if ((src == (whiteToMove ? Square::E1 : Square::E8))/* & !IsInCheck(board, whiteToMove)*/) {
                if (whiteToMove) {
                    //Check castling rights and open availability
                    if ((board.castleRights & CastleRights::WHITE_OOO) != CastleRights::CASTLE_NONE) {
                        //If all of the needed spaces are empty, and we're not moving THROUGH check...
                        if (((board.allPieces & 0x0e00000000000000ull) == EmptyBitboard)
                            && (!this->attackGenerator.isSquareAttacked(board, Square::D1))) {
                            //We will test to see if we're put INTO check later...
                            dstMoves |= OneShiftedBy(Square::C1);
                        }
                    }
                    if ((board.castleRights & CastleRights::WHITE_OO) != CastleRights::CASTLE_NONE) {
                        if (((board.allPieces & 0x6000000000000000ull) == EmptyBitboard)
                            && (!this->attackGenerator.isSquareAttacked(board, Square::F1))) {
                            dstMoves |= OneShiftedBy(Square::G1);
                        }
                    }
                }
                else {
                    //Check castling rights and open availability
                    if ((board.castleRights & CastleRights::BLACK_OOO) != CastleRights::CASTLE_NONE) {
                        if (((board.allPieces & 0x000000000000000eull) == EmptyBitboard)
                            && (!this->attackGenerator.isSquareAttacked(board, Square::D8))) {
                            dstMoves |= OneShiftedBy(Square::C8);
                        }
                    }
                    if ((board.castleRights & CastleRights::BLACK_OO) != CastleRights::CASTLE_NONE) {
                        if (((board.allPieces & 0x0000000000000060ull) == EmptyBitboard)
                            && (!this->attackGenerator.isSquareAttacked(board, Square::F8))) {
                            dstMoves |= OneShiftedBy(Square::G8);
                        }
                    }
                }
            }
            break;
        }

        dstMoves &= ~piecesToMove;

        if (board.pinnedPieces != EmptyBitboard) {
            //If this piece is pinned, it's destination moves can only be other squares in between attackers (in this case, blocked pieces)
            if ((OneShiftedBy(src) & board.pinnedPieces) != EmptyBitboard) {
                dstMoves &= board.inBetweenSquares | board.blockedPieces;
            }
        }

        while (BitScanForward64((std::uint32_t *)&dst, dstMoves)) {
            dstMoves = ResetLowestSetBit(dstMoves);

            switch (movingPiece) {
            case PieceType::PAWN:
                if (getRank(dst) == (whiteToMove ? Rank::_8 : Rank::_1)) {
                    if (countOnly) {
                        moveCount += 4;
                    }
                    else {
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::QUEEN });
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::ROOK });
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::BISHOP });
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::KNIGHT });
                    }
                }
                else {
                    if (countOnly) {
                        moveCount ++;
                    }
                    else {
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::NO_PIECE });
                    }
                }
                break;
            case PieceType::KNIGHT:
                if (countOnly) {
                    moveCount++;
                }
                else {
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::NO_PIECE });
                }
                break;
            case PieceType::KING:
                if (!this->attackGenerator.isSquareAttacked(board, dst)) {
                    if (countOnly) {
                        moveCount++;
                    }
                    else {
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::NO_PIECE });
                    }
                }
                break;
            default:
                //We know this is either an empty space or a capture move; just make sure there's nothing in between
                if ((InBetween[src][dst] & board.allPieces) == EmptyBitboard) {
                    if (countOnly) {
                        moveCount++;
                    }
                    else {
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::NO_PIECE });
                    }
                }
            }
        }
    }

    //3) There are very rare instances where a pinned piece move or en passant move is errantly generated.  Resort to manual double checking of those moves.
    if (!countOnly
        && this->shouldDoubleCheckGeneratedMoves(board)) {
        this->doubleCheckGeneratedMoves(board, moveList);
    }

    if (countOnly) {
        return moveCount;
    }
    else {
        return moveList.size();
    }
}

NodeCount ChessMoveGenerator::generateAttacksOnSquares(BoardType& board, MoveList<MoveType>& moveList, Bitboard dstSquares, Bitboard excludeSrcSquares)
{
    bool whiteToMove = board.sideToMove == Color::WHITE;

    Bitboard includeSrcSquares = ~excludeSrcSquares;
    Bitboard* piecesToMove = whiteToMove ? board.whitePieces : board.blackPieces;

    //REMEMBER: Here, we're scanning backwards for moves!  We're scanning from the destination to the source rather than
    //	from the source to the destination
    Square dst;
    while (BitScanForward64((std::uint32_t *)&dst, dstSquares)) {
        dstSquares = ResetLowestSetBit(dstSquares);

        for (PieceType piece = PieceType::PAWN; piece < PieceType::PIECETYPE_COUNT; piece++) {
            Bitboard srcSquares;

            switch (piece) {
            case PAWN:
                srcSquares = (whiteToMove ? BlackPawnCaptures[dst] : WhitePawnCaptures[dst]) & piecesToMove[PieceType::PAWN];
                break;
            default:
                srcSquares = PieceMoves[piece][dst] & piecesToMove[piece];
            }

            srcSquares &= includeSrcSquares;

            Square src;
            while (BitScanForward64((std::uint32_t *)&src, srcSquares)) {
                srcSquares = ResetLowestSetBit(srcSquares);

                if ((piece == PAWN) && (getRank(dst) == (whiteToMove ? Rank::_8 : Rank::_1))) {
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::QUEEN });
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::ROOK });
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::BISHOP });
                    moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::KNIGHT });
                }
                else
                    //If there's actually one of our pieces at the source, and nothing in between, allow the move
                    if (((OneShiftedBy(src) & piecesToMove[PieceType::ALL]) != EmptyBitboard) && (board.pieces[src] == piece) && ((InBetween[src][dst] & board.allPieces) == EmptyBitboard)) {
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::NO_PIECE });
                    }
            }
        }
    }

    return moveList.size();
}

NodeCount ChessMoveGenerator::generateCheckEvasions(BoardType& board, MoveList<MoveType>& moveList)
{
    moveList.clear();

    bool whiteToMove = board.sideToMove == Color::WHITE;
    Square kingPosition = whiteToMove ? board.whiteKingPosition : board.blackKingPosition;

    Bitboard* piecesToMove = whiteToMove ? board.whitePieces : board.blackPieces;

    //1) Add all king moves which evade check.
    //It's okay for our king to capture an opponent piece or move to an empty space, but we cannot capture our own pieces
    Bitboard dstMoves = PieceMoves[PieceType::KING][kingPosition] & (~piecesToMove[PieceType::ALL]);

    Square dst;
    while (BitScanForward64((std::uint32_t *)&dst, dstMoves)) {
        dstMoves = ResetLowestSetBit(dstMoves);

        //If the destination square is not attacked, it is safe to move the king there
        if (!this->attackGenerator.isSquareAttacked(board, dst)) {
            moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, kingPosition, dst, PieceType::NO_PIECE });
        }
    }

    //2) Find which pieces are attacking the king.  If two pieces are checking the king, return the current list since
    //	only moves which move the king himself are able to evade two checking pieces.
    Bitboard checkingPieces = board.checkingPieces;

    if (popCountSparse(checkingPieces) == 2) {
        return moveList.size();
    }

    //3) Add moves which capture the piece doing the checking, except for king captures.  Those have already been
    //	generated.  If the piece was a pawn or a knight, or was one space within the king(indicating a block is
    //	impossible), return the current list.

    //From here, we know that only one piece is checking; Find it.
    Square checkingPosition;
    BitScanForward64((std::uint32_t *)&checkingPosition, checkingPieces);

    //Generate the pawn attacks which capture the en_passant pawn
    //The pawn cannot be captured unless
    if (board.enPassant != Square::NO_SQUARE) {
        Direction dir = whiteToMove ? Direction::DOWN : Direction::UP;

        //Ensure the pawn being captured is actually the piece doing the checking
        if ((OneShiftedBy(board.enPassant + dir) & board.checkingPieces) != EmptyBitboard) {
            //The positions from which pawns can capture onto the en passant square
            Bitboard pawnCaptures = whiteToMove ? BlackPawnCaptures[board.enPassant] : WhitePawnCaptures[board.enPassant];
            //Ensure the moving pawns are not pinned
            Bitboard srcPawns = pawnCaptures & piecesToMove[PieceType::PAWN] & ~board.pinnedPieces;

            Square src;
            while (BitScanForward64((std::uint32_t *)&src, srcPawns)) {
                srcPawns = ResetLowestSetBit(srcPawns);

                moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, board.enPassant, PieceType::NO_PIECE });
            }
        }
    }

    Bitboard ourKing = whiteToMove ? board.whitePieces[PieceType::KING] : board.blackPieces[PieceType::KING];

    //If the checking piece is a pawn or a knight, or the checking piece was next to the king, then it cannot be blocked.
    //	Return only moves which can attack the piece.
    if ((board.pieces[checkingPosition] <= KNIGHT) || ((PieceMoves[PieceType::KING][kingPosition] & checkingPieces) != EmptyBitboard)) {
        this->generateAttacksOnSquares(board, moveList, checkingPieces, ourKing | board.pinnedPieces);

        return moveList.size();
    }

    //If there's no way to block the attacking piece, simply return
    if (board.inBetweenSquares == EmptyBitboard) {
        return moveList.size();
    }

    //4) Generate all moves which either attack the checking piece, or block it
    this->generateAttacksOnSquares(board, moveList, checkingPieces, ourKing | board.pinnedPieces);

    //A piece that is already pinned cannot block another piece by moving, so exclude them
    Bitboard inBetweenSquares = InBetween[kingPosition][checkingPosition];
    this->generateMovesToSquares(board, moveList, inBetweenSquares, ourKing | board.pinnedPieces);

    return moveList.size();
}

NodeCount ChessMoveGenerator::generateMovesToSquares(BoardType& board, MoveList<MoveType>& moveList, Bitboard dstSquares, Bitboard excludeSrcSquares)
{
    Bitboard includeSrcSquares = ~excludeSrcSquares;

    bool whiteToMove = board.sideToMove == Color::WHITE;

    Bitboard* piecesToMove = whiteToMove ? board.whitePieces : board.blackPieces;
    Bitboard* otherPieces = whiteToMove ? board.blackPieces : board.whitePieces;

    Bitboard pawnCaptures;

    //REMEMBER: Here, we're scanning backwards for moves!  We're scanning from the destination to the source rather than
    //	from the source to the destination
    Square dst;
    while (BitScanForward64((std::uint32_t *)&dst, dstSquares)) {
        dstSquares = ResetLowestSetBit(dstSquares);

        for (PieceType piece = PieceType::PAWN; piece <= PieceType::KING; piece++) {
            Bitboard srcSquares = EmptyBitboard;

            switch (piece) {
            case PieceType::PAWN:
            {
                pawnCaptures = whiteToMove ? BlackPawnCaptures[dst] : WhitePawnCaptures[dst];
                Direction dir = whiteToMove ? Direction::DOWN : Direction::UP;
                Direction dir2 = whiteToMove ? Direction::TWO_DOWN : Direction::TWO_UP;

                //Since we're in reverse, we want the black pawn captures from the destination square to the white pawns
                srcSquares = (otherPieces[PieceType::ALL] & OneShiftedBy(dst)) & (pawnCaptures & piecesToMove[PieceType::PAWN]);

                if (whiteToMove) {
                    //We cannot, however, generate pawn moves in the same manner.
                    if (getRank(dst) == Rank::_4) {
                        srcSquares |= OneShiftedBy(dst + dir) | OneShiftedBy(dst + dir2);
                    }
                    else if (getRank(dst) == Rank::_1) {
                    }
                    else {
                        srcSquares |= OneShiftedBy(dst + dir);
                    }
                }
                else {
                    //We cannot, however, generate pawn moves in the same manner.
                    if (getRank(dst) == Rank::_5) {
                        srcSquares |= OneShiftedBy(dst + dir) | OneShiftedBy(dst + dir2);
                    }
                    else if (getRank(dst) == Rank::_8) {
                    }
                    else {
                        srcSquares |= OneShiftedBy(dst + dir);
                    }
                }
                break;
            }
            default:
                srcSquares = PieceMoves[piece][dst] & piecesToMove[piece];
            }

            srcSquares &= includeSrcSquares;

            Square src;
            while (BitScanForward64((std::uint32_t *)&src, srcSquares)) {
                srcSquares = ResetLowestSetBit(srcSquares);

                //If there's actually one of our pieces at the source, and nothing in between, allow the move
                if (((OneShiftedBy(src) & piecesToMove[PieceType::ALL]) != EmptyBitboard) && (board.pieces[src] == piece) && ((InBetween[src][dst] & board.allPieces) == EmptyBitboard)) {
                    if ((piece == PAWN) && (getRank(dst) == (whiteToMove ? Rank::_8 : Rank::_1))) {
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::QUEEN });
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::ROOK });
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::BISHOP });
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::KNIGHT });
                    }
                    else {
                        moveList.push_back({ ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL, src, dst, PieceType::NO_PIECE });
                    }
                }
            }
        }
    }

    return moveList.size();
}

NodeCount ChessMoveGenerator::perft(BoardType& board, Depth maxDepth, Depth currentDepth)
{
    NodeCount result = ZeroNodes;

    MoveList<MoveType>& moveList = PerftMoveList[currentDepth];

    if (currentDepth == maxDepth
        && currentDepth > Depth::ONE) {
        return this->generateAllMoves(board, moveList, true);
    }

    NodeCount moveCount = this->generateAllMoves(board, moveList);

    for (MoveList<MoveType>::iterator it = moveList.begin(); it != moveList.end(); ++it) {
        BoardType nextBoard = board;
        MoveType& move = *it;

        nextBoard.doMove<false>(move);

        if (currentDepth == Depth::ONE) {
            principalVariation.printMoveToConsole(move);
        }

        if (maxDepth == Depth::ONE) {
            std::cout << std::endl;

            result++;

            continue;
        }

        NodeCount nodeCount = this->perft(nextBoard, maxDepth, currentDepth + Depth::ONE);

        if (currentDepth == Depth::ONE) {
            std::cout << ": " << nodeCount << std::endl;
        }

        result += nodeCount;
    }

    return result;
}

template <NodeType nodeType>
void ChessMoveGenerator::reorderMoves(BoardType& board, MoveList<MoveType>& moveList, SearchStack& searchStack, ChessButterflyTable& butterflyTable)
{
    bool whiteToMove = board.sideToMove == Color::WHITE;

    ChessPrincipalVariation& principalVariation = searchStack.principalVariation;

    Bitboard* otherPieces = whiteToMove ? board.blackPieces : board.whitePieces;

    Direction left = whiteToMove ? Direction::DOWN_LEFT : Direction::UP_LEFT;
    Direction right = whiteToMove ? Direction::DOWN_RIGHT : Direction::UP_RIGHT;

    Bitboard unsafeSquares = ((otherPieces[PieceType::PAWN] & ~bbFile[File::_A]) + left) | ((otherPieces[PieceType::PAWN] & ~bbFile[File::_H]) + right);

    for (MoveList<MoveType>::iterator it = moveList.begin(); it != moveList.end(); ++it) {
        MoveType& move = (*it);

        Square src = move.src;
        Square dst = move.dst;

        PieceType movingPiece = board.pieces[src];
        PieceType capturedPiece = board.pieces[dst];

        if (nodeType == NodeType::PV_NODETYPE
            && principalVariation.size() > 0
            && principalVariation[0] == move
            && nodeType == NodeType::PV_NODETYPE) {
            move.ordinal = ChessMoveOrdinal::PV_MOVE;
        }
        else if (capturedPiece != PieceType::NO_PIECE) {
            Evaluation capturedPieceEvaluation = MaterialParameters[capturedPiece];
            Evaluation movingPieceEvaluation = MaterialParameters[movingPiece];

            if (capturedPieceEvaluation.mg > movingPieceEvaluation.mg) {
                move.ordinal = ChessMoveOrdinal::GOOD_CAPTURE_MOVE;
            }
            else if (capturedPieceEvaluation.mg == movingPieceEvaluation.mg) {
                move.ordinal = ChessMoveOrdinal::EQUAL_CAPTURE_MOVE;
            }
            else {
                move.ordinal = ChessMoveOrdinal::BAD_CAPTURE_MOVE;
            }
        }
        else if (searchStack.killer1 == move) {
            move.ordinal = ChessMoveOrdinal::KILLER1_MOVE;
        }
        else if (searchStack.killer2 == move) {
            move.ordinal = ChessMoveOrdinal::KILLER2_MOVE;
        }
        else if (movingPiece != PieceType::PAWN
            && (unsafeSquares & OneShiftedBy(src)) != EmptyBitboard) {
            move.ordinal = ChessMoveOrdinal::UNSAFE_MOVE;
        }
        else {
            if (enableButterflyTable) {
                std::uint32_t butterflyScore = butterflyTable.get(movingPiece, dst);

                move.ordinal = ChessMoveOrdinal::BUTTERFLY_MOVE + ChessMoveOrdinal(butterflyScore);
            }
            else {
                move.ordinal = ChessMoveOrdinal::UNCLASSIFIED_MOVE;
            }
        }
    }
}

template <NodeType nodeType>
void ChessMoveGenerator::reorderQuiescenceMoves(BoardType& board, MoveList<MoveType>& moveList, SearchStack& searchStack)
{
    bool whiteToMove = board.sideToMove == Color::WHITE;

    Bitboard* otherPieces = whiteToMove ? board.blackPieces : board.whitePieces;

    Direction left = whiteToMove ? Direction::DOWN_LEFT : Direction::UP_LEFT;
    Direction right = whiteToMove ? Direction::DOWN_RIGHT : Direction::UP_RIGHT;

    Bitboard unsafeSquares = ((otherPieces[PieceType::PAWN] & ~bbFile[File::_A]) + left) | ((otherPieces[PieceType::PAWN] & ~bbFile[File::_H]) + right);

    for (MoveList<MoveType>::iterator it = moveList.begin(); it != moveList.end(); ++it) {
        MoveType& move = (*it);

        Square src = move.src;
        Square dst = move.dst;

        PieceType movingPiece = board.pieces[src];
        PieceType capturedPiece = board.pieces[dst];

        if (movingPiece != PieceType::PAWN
            && (unsafeSquares & OneShiftedBy(src)) != EmptyBitboard) {
            move.ordinal = ChessMoveOrdinal::UNSAFE_MOVE;
        }
        else {
            Evaluation capturedPieceEvaluation = MaterialParameters[capturedPiece];
            Evaluation movingPieceEvaluation = MaterialParameters[movingPiece];

            move.ordinal = ChessMoveOrdinal::QUIESENCE_MOVE + ChessMoveOrdinal(1024 * capturedPieceEvaluation.mg - movingPieceEvaluation.mg);
        }
    }

    std::stable_sort(moveList.begin(), moveList.end(), greater<MoveType>);
}

bool ChessMoveGenerator::shouldDoubleCheckGeneratedMoves(BoardType& board)
{
    return (board.pinnedPieces != EmptyBitboard)
        || (board.enPassant != Square::NO_SQUARE);
}

template void ChessMoveGenerator::reorderMoves<NodeType::PV_NODETYPE>(ChessBoard& board, MoveList<MoveType>& moveList, SearchStack& searchStack, ChessButterflyTable& butterflyTable);
template void ChessMoveGenerator::reorderMoves<NodeType::ALL_NODETYPE>(ChessBoard& board, MoveList<MoveType>& moveList, SearchStack& searchStack, ChessButterflyTable& butterflyTable);
template void ChessMoveGenerator::reorderMoves<NodeType::CUT_NODETYPE>(ChessBoard& board, MoveList<MoveType>& moveList, SearchStack& searchStack, ChessButterflyTable& butterflyTable);

template void ChessMoveGenerator::reorderQuiescenceMoves<NodeType::PV_NODETYPE>(ChessBoard& board, MoveList<MoveType>& moveList, SearchStack& searchStack);
template void ChessMoveGenerator::reorderQuiescenceMoves<NodeType::ALL_NODETYPE>(ChessBoard& board, MoveList<MoveType>& moveList, SearchStack& searchStack);
template void ChessMoveGenerator::reorderQuiescenceMoves<NodeType::CUT_NODETYPE>(ChessBoard& board, MoveList<MoveType>& moveList, SearchStack& searchStack);
