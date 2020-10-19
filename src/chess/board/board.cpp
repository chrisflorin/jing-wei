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

#include <sstream>

#include "board.h"

#include "../../game/math/bitreset.h"
#include "../../game/math/bitscan.h"
#include "../../game/math/popcount.h"
#include "../../game/math/shift.h"

#include "../../game/types/hash.h"
#include "../../game/types/score.h"

#include "../types/square.h"

const std::string startingPositionFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

static const std::string PieceToChar = " PNBRQK  pnbrqk";

extern Bitboard EnPassant[Square::SQUARE_COUNT];

extern Bitboard WhitePawnCaptures[Square::SQUARE_COUNT];
extern Bitboard BlackPawnCaptures[Square::SQUARE_COUNT];
extern Bitboard PieceMoves[PieceType::PIECETYPE_COUNT][Square::SQUARE_COUNT];
extern Bitboard InBetween[Square::SQUARE_COUNT][Square::SQUARE_COUNT];

extern Evaluation MaterialParameters[PieceType::PIECETYPE_COUNT];
extern Evaluation PstParameters[PieceType::PIECETYPE_COUNT][Square::SQUARE_COUNT];

extern Hash PieceHashValues[Color::COLOR_COUNT][PieceType::PIECETYPE_COUNT][Square::SQUARE_COUNT];
extern Hash WhiteToMoveHash;
extern Hash CastleRightsHashValues[CastleRights::CASTLERIGHTS_COUNT];
extern Hash EnPassantHashValues[Square::SQUARE_COUNT];

ChessBoard::ChessBoard()
{

}

ChessBoard::~ChessBoard()
{

}

void ChessBoard::buildAttackBoards()
{
    bool whiteToMove = this->sideToMove == Color::WHITE;
    Square kingPosition = whiteToMove ? this->whiteKingPosition : this->blackKingPosition;

    Bitboard checkingPieces;
    Bitboard blockedPieces = EmptyBitboard;
    Bitboard pinnedPieces = EmptyBitboard;
    Bitboard inBetweenSquares = EmptyBitboard;

    Bitboard inBetween;

    Bitboard* otherPieces = whiteToMove ? this->blackPieces : this->whitePieces;

    //1) Check to see if a pawn or knight is doing attacking.  If so, enter them in the attack bitboard.
    Bitboard pawnAttacks = (whiteToMove ? WhitePawnCaptures[kingPosition] : BlackPawnCaptures[kingPosition]) & otherPieces[PieceType::PAWN];
    Bitboard knightAttacks = PieceMoves[PieceType::KNIGHT][kingPosition] & otherPieces[PieceType::KNIGHT];

    checkingPieces = pawnAttacks | knightAttacks;

    //2) Check to see if a bishop or queen (diagonally) is doing the attacking.
    Bitboard bishopAttacks = PieceMoves[PieceType::BISHOP][kingPosition] & (otherPieces[PieceType::BISHOP] | otherPieces[PieceType::QUEEN]);

    //Scan through our Bishop attacks to see if any of them are not blocked
    Square src;
    while (BitScanForward64((std::uint32_t *)&src, bishopAttacks)) {
        bishopAttacks = ResetLowestSetBit(bishopAttacks);

        inBetween = InBetween[kingPosition][src] & this->allPieces;
        inBetweenSquares |= InBetween[kingPosition][src];

        if (inBetween == EmptyBitboard) {
            checkingPieces |= OneShiftedBy(src);
        }
        else {
            blockedPieces |= OneShiftedBy(src);
            if (popCountIsOne(inBetween)) { pinnedPieces |= inBetween; }
        }
    }

    //3) Check to see if a rook or queen (straight) is doing the attacking.
    Bitboard rookAttacks = PieceMoves[PieceType::ROOK][kingPosition] & (otherPieces[PieceType::ROOK] | otherPieces[PieceType::QUEEN]);

    //Scan through our Rook attacks to see if any of them are not blocked
    while (BitScanForward64((std::uint32_t *)&src, rookAttacks)) {
        rookAttacks = ResetLowestSetBit(rookAttacks);

        inBetween = InBetween[kingPosition][src] & this->allPieces;
        inBetweenSquares |= InBetween[kingPosition][src];

        if (inBetween == EmptyBitboard) {
            checkingPieces |= OneShiftedBy(src);
        }
        else {
            blockedPieces |= OneShiftedBy(src);
            if (popCountIsOne(inBetween)) { pinnedPieces |= inBetween; }
        }
    }

    this->blockedPieces = blockedPieces;
    this->checkingPieces = checkingPieces;
    this->pinnedPieces = pinnedPieces;
    this->inBetweenSquares = inBetweenSquares;
}

void ChessBoard::buildBitboardsFromMailbox()
{
    //Clear all bitboards
    this->whitePieces[PieceType::PAWN] = this->whitePieces[PieceType::KNIGHT] = this->whitePieces[PieceType::BISHOP] = this->whitePieces[PieceType::ROOK] = this->whitePieces[PieceType::QUEEN] = this->whitePieces[PieceType::KING] = EmptyBitboard;
    this->blackPieces[PieceType::PAWN] = this->blackPieces[PieceType::KNIGHT] = this->blackPieces[PieceType::BISHOP] = this->blackPieces[PieceType::ROOK] = this->blackPieces[PieceType::QUEEN] = this->blackPieces[PieceType::KING] = EmptyBitboard;

    //Build easy bitboards
    this->allPieces = this->whitePieces[PieceType::ALL] | this->blackPieces[PieceType::ALL];

    //Loop through mailbox building each individual bitboard
    for (Square src = Square::FIRST_SQUARE; src < Square::SQUARE_COUNT; src++) {
        if (this->pieces[src] == PieceType::NO_PIECE) { continue; }

        Bitboard b = OneShiftedBy(src);
        bool whitepiece = (this->whitePieces[PieceType::ALL] & b) != EmptyBitboard;

        PieceType piece = this->pieces[src];

        if (whitepiece) {
            this->whitePieces[piece] |= b;
        }
        else {
            this->blackPieces[piece] |= b;
        }

        if (this->pieces[src] == PieceType::KING) {
            if (whitepiece) {
                this->whiteKingPosition = src;
            }
            else {
                this->blackKingPosition = src;
            }
        }
    }
}

Hash ChessBoard::calculateHash()
{
    Hash result = EmptyHash;

    Bitboard srcSquares;

    for (Color color = Color::COLOR_START; color < Color::COLOR_COUNT; color++) {
        bool whiteColor = color == Color::WHITE;

        Bitboard* sideToMovePieces = whiteColor ? this->whitePieces : this->blackPieces;

        for (PieceType piece = PieceType::PAWN; piece < PieceType::ALL; piece++) {
            srcSquares = sideToMovePieces[piece];

            Square src;
            while (BitScanForward64((std::uint32_t *)&src, srcSquares)) {
                srcSquares = ResetLowestSetBit(srcSquares);

                result ^= PieceHashValues[color][piece][src];
            }
        }
    }

    result ^= CastleRightsHashValues[this->castleRights];

    bool whiteToMove = this->sideToMove == Color::WHITE;
    result ^= whiteToMove ? WhiteToMoveHash : EmptyHash;

    if (this->enPassant != Square::NO_SQUARE) {
        result ^= EnPassantHashValues[this->enPassant];
    }

    return result;
}

Hash ChessBoard::calculateMaterialHash()
{
    Hash result = EmptyHash;

    for (Color color = Color::WHITE; color < Color::COLOR_COUNT; color++) {
        bool whiteColor = color == Color::WHITE;

        Bitboard* pieces = whiteColor ? this->whitePieces : this->blackPieces;

        for (PieceType pieceType = PieceType::PAWN; pieceType <= PieceType::KING; pieceType++) {
            int pieceTypeCount = popCount(pieces[pieceType]);

            result ^= PieceHashValues[color][pieceType][pieceTypeCount];
        }
    }

    return result;
}

Evaluation ChessBoard::calculateMaterialEvaluation()
{
    Evaluation result = { NO_SCORE, NO_SCORE };

    for (PieceType piece = PieceType::PAWN; piece < PieceType::KING; piece++) {
        result += MaterialParameters[piece] * static_cast<std::int32_t>(popCountSparse(this->whitePieces[piece]));
        result -= MaterialParameters[piece] * static_cast<std::int32_t>(popCountSparse(this->blackPieces[piece]));
    }

    return result;
}

Hash ChessBoard::calculatePawnHash()
{
    Hash result = EmptyHash;

    for (Color color = Color::COLOR_START; color < Color::COLOR_COUNT; color++) {
        bool whiteColor = color == Color::WHITE;

        Bitboard* sideToMovePieces = whiteColor ? this->whitePieces : this->blackPieces;
        Bitboard srcSquares = sideToMovePieces[PieceType::PAWN];

        Square src;
        while (BitScanForward64((std::uint32_t *)&src, srcSquares)) {
            srcSquares = ResetLowestSetBit(srcSquares);

            result ^= PieceHashValues[color][PieceType::PAWN][src];
        }
    }

    return result;
}

Evaluation ChessBoard::calculatePstEvaluation()
{
    Evaluation result = { NO_SCORE, NO_SCORE };

    Bitboard srcSquares;

    for (Color color = Color::COLOR_START; color < Color::COLOR_COUNT; color++) {
        bool whiteColor = color == Color::WHITE;
        std::int32_t multiplier = whiteColor ? 1 : -1;

        Bitboard* sideToMovePieces = whiteColor ? this->whitePieces : this->blackPieces;

        for (PieceType piece = PieceType::PAWN; piece < PieceType::ALL; piece++) {
            srcSquares = sideToMovePieces[piece];

            Square src;
            while (BitScanForward64((std::uint32_t *)&src, srcSquares)) {
                srcSquares = ResetLowestSetBit(srcSquares);

                if (!whiteColor) {
                    src = FlipSqY(src);
                }

                result += multiplier * PstParameters[piece][src];
            }
        }
    }

    return result;
}

void ChessBoard::clearEverything()
{
    Square src;
    PieceType piece;

    for (src = Square::FIRST_SQUARE; src < Square::SQUARE_COUNT; src++) {
        this->pieces[src] = PieceType::NO_PIECE;
    }

    for (piece = PieceType::NO_PIECE; piece <= PieceType::ALL; piece++) {
        this->whitePieces[piece] = EmptyBitboard;
        this->blackPieces[piece] = EmptyBitboard;
    }

    this->allPieces = EmptyBitboard;

    this->sideToMove = Color::WHITE;
    this->castleRights = CastleRights::CASTLE_ALL;
    this->enPassant = Square::NO_SQUARE;
    this->fiftyMoveCount = 0;
    this->fullMoveCount = 1;

    this->whiteKingPosition = Square::NO_SQUARE;
    this->blackKingPosition = Square::NO_SQUARE;

    this->blockedPieces = EmptyBitboard;
    this->checkingPieces = EmptyBitboard;
    this->inBetweenSquares = EmptyBitboard;
    this->pinnedPieces = EmptyBitboard;

    this->hashValue = EmptyHash;
    this->materialHashValue = EmptyHash;
    this->pawnHashValue = EmptyHash;

    this->materialEvaluation = { NO_SCORE, NO_SCORE };
    this->pstEvaluation = { NO_SCORE, NO_SCORE };

    this->nullMove = false;
}

template<bool performPreCalculations>
void ChessBoard::doMoveImplementation(ChessMove& move)
{
    bool whiteToMove = this->sideToMove == Color::WHITE;

    Color colorToMove = whiteToMove ? Color::WHITE : Color::BLACK;
    Color otherColor = whiteToMove ? Color::BLACK : Color::WHITE;

    std::int32_t multiplier = whiteToMove ? 1 : -1;

    Square src = move.src;
    Square dst = move.dst;

    Bitboard* piecesToMove = whiteToMove ? this->whitePieces : this->blackPieces;
    Bitboard* otherPieces = whiteToMove ? this->blackPieces : this->whitePieces;

    Bitboard enPassantPieces;

    Square oldEnPassant = this->enPassant;

    //1) If this is en passant, move the captured pawn back one piece
    PieceType movingPiece = move.movedPiece = this->pieces[src];

    if ((dst == oldEnPassant) && (movingPiece == PAWN)) {
        //This direction is the direction the captured pawn is to the destination en passant square
        Direction dir = whiteToMove ? Direction::DOWN : Direction::UP;

        otherPieces[PieceType::PAWN] = (otherPieces[PieceType::PAWN] ^ OneShiftedBy(dst + dir)) | OneShiftedBy(dst);
        otherPieces[PieceType::ALL] = (otherPieces[PieceType::ALL] ^ OneShiftedBy(dst + dir)) | OneShiftedBy(dst);

        this->pieces[dst] = PAWN;
        this->pieces[dst + dir] = PieceType::NO_PIECE;

        if (performPreCalculations) {
            this->hashValue ^= PieceHashValues[otherColor][PieceType::PAWN][dst];
            this->hashValue ^= PieceHashValues[otherColor][PieceType::PAWN][dst + dir];

            this->pawnHashValue ^= PieceHashValues[otherColor][PieceType::PAWN][dst];
            this->pawnHashValue ^= PieceHashValues[otherColor][PieceType::PAWN][dst + dir];

            if (whiteToMove) {
                this->pstEvaluation += multiplier * PstParameters[PieceType::PAWN][FlipSqY(dst + dir)];
                this->pstEvaluation -= multiplier * PstParameters[PieceType::PAWN][FlipSqY(dst)];
            }
            else {
                this->pstEvaluation += multiplier * PstParameters[PieceType::PAWN][dst + dir];
                this->pstEvaluation -= multiplier * PstParameters[PieceType::PAWN][dst];
            }
        }
    }

    //2) If this is a capture move, save the captured piece
    PieceType capturedPiece = move.capturedPiece = this->pieces[dst];

    //4) Do the actual move
    this->pieces[dst] = movingPiece;
    this->pieces[src] = PieceType::NO_PIECE;

    if (performPreCalculations) {
        if (whiteToMove) {
            this->pstEvaluation += multiplier * PstParameters[movingPiece][dst];
            this->pstEvaluation -= multiplier * PstParameters[movingPiece][src];
        }
        else {
            this->pstEvaluation += multiplier * PstParameters[movingPiece][FlipSqY(dst)];
            this->pstEvaluation -= multiplier * PstParameters[movingPiece][FlipSqY(src)];
        }

        this->hashValue ^= PieceHashValues[colorToMove][movingPiece][src];
        this->hashValue ^= PieceHashValues[colorToMove][movingPiece][dst];
    }

    piecesToMove[movingPiece] = (piecesToMove[movingPiece] ^ OneShiftedBy(src)) | OneShiftedBy(dst);
    piecesToMove[PieceType::ALL] = (piecesToMove[PieceType::ALL] ^ OneShiftedBy(src)) | OneShiftedBy(dst);

    //Reset the en_passant status.  If this is an en_passant move, it will be changed later
    this->enPassant = Square::NO_SQUARE;

    //5) Perform side effects from special moves
    CastleRights oldCastleRights = this->castleRights;

    switch (movingPiece) {
    case PAWN:
    {
        Direction dir = whiteToMove ? Direction::UP : Direction::DOWN;
        Direction twoDir = whiteToMove ? Direction::TWO_UP : Direction::TWO_DOWN;

        enPassantPieces = EnPassant[src] & otherPieces[PieceType::PAWN];
        if (((src + twoDir) == dst) && (enPassantPieces != EmptyBitboard)) {
            this->enPassant = src + dir;
        }

        if (performPreCalculations) {
            this->pawnHashValue ^= PieceHashValues[colorToMove][PieceType::PAWN][src];
            this->pawnHashValue ^= PieceHashValues[colorToMove][PieceType::PAWN][dst];
        }
    }
    break;
    case ROOK:
        if (whiteToMove) {
            if (src == Square::A1) {
                this->castleRights = (this->castleRights | CastleRights::WHITE_OOO) ^ CastleRights::WHITE_OOO;
            }
            else if (src == Square::H1) {
                this->castleRights = (this->castleRights | CastleRights::WHITE_OO) ^ CastleRights::WHITE_OO;
            }
        }
        else {
            if (src == Square::A8) {
                this->castleRights = (this->castleRights | CastleRights::BLACK_OOO) ^ CastleRights::BLACK_OOO;
            }
            else if (src == Square::H8) {
                this->castleRights = (this->castleRights | CastleRights::BLACK_OO) ^ CastleRights::BLACK_OO;
            }
        }
        break;
    case KING:
        if (whiteToMove) {
            this->whiteKingPosition = dst;
            this->castleRights &= CastleRights::BLACK_ALL;

            //8) If this is a castle, move the associated rook and destroy castle rights for that side.
            if ((src == Square::E1) && (dst == Square::G1)) {
                this->pieces[Square::F1] = PieceType::ROOK;
                this->pieces[Square::H1] = PieceType::NO_PIECE;

                if (performPreCalculations) {
                    this->pstEvaluation += multiplier * PstParameters[PieceType::ROOK][Square::F1];
                    this->pstEvaluation -= multiplier * PstParameters[PieceType::ROOK][Square::H1];

                    this->hashValue ^= PieceHashValues[Color::WHITE][PieceType::ROOK][Square::F1];
                    this->hashValue ^= PieceHashValues[Color::WHITE][PieceType::ROOK][Square::H1];
                }

                piecesToMove[PieceType::ROOK] = (piecesToMove[PieceType::ROOK] ^ OneShiftedBy(Square::H1)) | OneShiftedBy(Square::F1);
                piecesToMove[PieceType::ALL] = (piecesToMove[PieceType::ALL] ^ OneShiftedBy(Square::H1)) | OneShiftedBy(Square::F1);
            }
            else if ((src == Square::E1) && (dst == Square::C1)) {
                this->pieces[Square::D1] = PieceType::ROOK;
                this->pieces[Square::A1] = PieceType::NO_PIECE;

                if (performPreCalculations) {
                    this->pstEvaluation += multiplier * PstParameters[PieceType::ROOK][Square::D1];
                    this->pstEvaluation -= multiplier * PstParameters[PieceType::ROOK][Square::A1];

                    this->hashValue ^= PieceHashValues[Color::WHITE][PieceType::ROOK][Square::D1];
                    this->hashValue ^= PieceHashValues[Color::WHITE][PieceType::ROOK][Square::A1];
                }

                piecesToMove[PieceType::ROOK] = (piecesToMove[PieceType::ROOK] ^ OneShiftedBy(Square::A1)) | OneShiftedBy(Square::D1);
                piecesToMove[PieceType::ALL] = (piecesToMove[PieceType::ALL] ^ OneShiftedBy(Square::A1)) | OneShiftedBy(Square::D1);
            }
        }
        else {
            this->blackKingPosition = dst;
            this->castleRights &= CastleRights::WHITE_ALL;

            //8) If this is a castle, move the associated rook and destroy castle rights for that side.
            if ((src == Square::E8) && (dst == Square::G8)) {
                this->pieces[Square::F8] = PieceType::ROOK;
                this->pieces[Square::H8] = PieceType::NO_PIECE;

                if (performPreCalculations) {
                    this->pstEvaluation += multiplier * PstParameters[PieceType::ROOK][FlipSqY(Square::F8)];
                    this->pstEvaluation -= multiplier * PstParameters[PieceType::ROOK][FlipSqY(Square::H8)];

                    this->hashValue ^= PieceHashValues[Color::BLACK][PieceType::ROOK][Square::F8];
                    this->hashValue ^= PieceHashValues[Color::BLACK][PieceType::ROOK][Square::H8];
                }

                piecesToMove[PieceType::ROOK] = (piecesToMove[PieceType::ROOK] ^ OneShiftedBy(Square::H8)) | OneShiftedBy(Square::F8);
                piecesToMove[PieceType::ALL] = (piecesToMove[PieceType::ALL] ^ OneShiftedBy(Square::H8)) | OneShiftedBy(Square::F8);
            }
            else if ((src == Square::E8) && (dst == Square::C8)) {
                this->pieces[Square::D8] = PieceType::ROOK;
                this->pieces[Square::A8] = PieceType::NO_PIECE;

                if (performPreCalculations) {
                    this->pstEvaluation += multiplier * PstParameters[PieceType::ROOK][FlipSqY(Square::D8)];
                    this->pstEvaluation -= multiplier * PstParameters[PieceType::ROOK][FlipSqY(Square::A8)];

                    this->hashValue ^= PieceHashValues[Color::BLACK][PieceType::ROOK][Square::D8];
                    this->hashValue ^= PieceHashValues[Color::BLACK][PieceType::ROOK][Square::A8];
                }

                piecesToMove[PieceType::ROOK] = (piecesToMove[PieceType::ROOK] ^ OneShiftedBy(Square::A8)) | OneShiftedBy(Square::D8);
                piecesToMove[PieceType::ALL] = (piecesToMove[PieceType::ALL] ^ OneShiftedBy(Square::A8)) | OneShiftedBy(Square::D8);
            }
        }
        break;
    }

    //6) Change the capturedpiece bitboard
    if (capturedPiece != PieceType::NO_PIECE) {
        if (performPreCalculations) {
            this->materialEvaluation += multiplier * MaterialParameters[capturedPiece];

            std::uint32_t pieceTypeCount = popCount(otherPieces[capturedPiece]);
            this->materialHashValue ^= PieceHashValues[otherColor][capturedPiece][pieceTypeCount] ^ PieceHashValues[otherColor][capturedPiece][pieceTypeCount - 1];

            if (whiteToMove) {
                this->pstEvaluation += multiplier * PstParameters[capturedPiece][FlipSqY(dst)];
            }
            else {
                this->pstEvaluation += multiplier * PstParameters[capturedPiece][dst];
            }

            this->hashValue ^= PieceHashValues[otherColor][capturedPiece][dst];
        }

        otherPieces[capturedPiece] = otherPieces[capturedPiece] ^ OneShiftedBy(dst);
        otherPieces[PieceType::ALL] = otherPieces[PieceType::ALL] ^ OneShiftedBy(dst);

        switch (capturedPiece) {
        case PieceType::PAWN:
            if (performPreCalculations) {
                this->pawnHashValue ^= PieceHashValues[otherColor][PieceType::PAWN][dst];
            }
            break;
        case PieceType::ROOK:
            if (whiteToMove) {
                if (dst == Square::A8) {
                    this->castleRights = (this->castleRights | CastleRights::BLACK_OOO) ^ CastleRights::BLACK_OOO;
                }
                else if (dst == Square::H8) {
                    this->castleRights = (this->castleRights | CastleRights::BLACK_OO) ^ CastleRights::BLACK_OO;
                }
            }
            else {
                if (dst == Square::A1) {
                    this->castleRights = (this->castleRights | CastleRights::WHITE_OOO) ^ CastleRights::WHITE_OOO;
                }
                else if (dst == Square::H1) {
                    this->castleRights = (this->castleRights | CastleRights::WHITE_OO) ^ CastleRights::WHITE_OO;
                }
            }
            break;
        }
    }

    //7) If this is a promotion, promote the moved pawn
    PieceType promotionPiece = move.promotionPiece;
    if (movingPiece == PieceType::PAWN && promotionPiece != PieceType::NO_PIECE) {
        this->pieces[dst] = promotionPiece;

        if (performPreCalculations) {
            this->materialEvaluation += multiplier * MaterialParameters[promotionPiece];
            this->materialEvaluation -= multiplier * MaterialParameters[PieceType::PAWN];

            std::uint32_t pieceTypeCount = popCount(piecesToMove[promotionPiece]);
            this->materialHashValue ^= PieceHashValues[colorToMove][promotionPiece][pieceTypeCount] ^ PieceHashValues[colorToMove][promotionPiece][pieceTypeCount + 1];

            pieceTypeCount = popCount(piecesToMove[PieceType::PAWN]);
            this->materialHashValue ^= PieceHashValues[colorToMove][PieceType::PAWN][pieceTypeCount] ^ PieceHashValues[colorToMove][PieceType::PAWN][pieceTypeCount - 1];

            if (whiteToMove) {
                this->pstEvaluation += multiplier * PstParameters[promotionPiece][dst];
                this->pstEvaluation -= multiplier * PstParameters[PieceType::PAWN][dst];
            }
            else {
                this->pstEvaluation += multiplier * PstParameters[promotionPiece][FlipSqY(dst)];
                this->pstEvaluation -= multiplier * PstParameters[PieceType::PAWN][FlipSqY(dst)];
            }

            this->hashValue ^= PieceHashValues[colorToMove][PieceType::PAWN][dst];
            this->hashValue ^= PieceHashValues[colorToMove][promotionPiece][dst];

            this->pawnHashValue ^= PieceHashValues[colorToMove][PieceType::PAWN][dst];
        }

        piecesToMove[promotionPiece] = piecesToMove[promotionPiece] | OneShiftedBy(dst);
        piecesToMove[PieceType::PAWN] = piecesToMove[PieceType::PAWN] ^ OneShiftedBy(dst);
    }

    //9) Switch side to move
    this->sideToMove = ~this->sideToMove;

    if (performPreCalculations) {
        this->hashValue ^= WhiteToMoveHash;
    }

    //11) Update miscellaneous thingies
    if (performPreCalculations) {
        if (oldEnPassant != Square::NO_SQUARE) {
            this->hashValue ^= EnPassantHashValues[oldEnPassant];
        }

        if (this->enPassant != Square::NO_SQUARE) {
            this->hashValue ^= EnPassantHashValues[this->enPassant];
        }

        if (this->castleRights != oldCastleRights) {
            this->hashValue ^= CastleRightsHashValues[oldCastleRights];
            this->hashValue ^= CastleRightsHashValues[this->castleRights];
        }
    }

    if ((capturedPiece == PieceType::NO_PIECE) && (movingPiece != PieceType::PAWN)) {
        this->fiftyMoveCount++;
    }
    else {
        this->fiftyMoveCount = 0;
    }

    //10) Set all pieces bitboard
    this->allPieces = this->whitePieces[PieceType::ALL] | this->blackPieces[PieceType::ALL];

    this->buildAttackBoards();
}

void ChessBoard::doNullMove()
{
    this->nullMove = true;

    this->hashValue ^= WhiteToMoveHash;
    this->sideToMove = this->sideToMove == Color::WHITE ? Color::BLACK : Color::WHITE;

    if (this->enPassant != Square::NO_SQUARE) {
        this->hashValue ^= EnPassantHashValues[this->enPassant];

        this->enPassant = Square::NO_SQUARE;
    }

    this->buildAttackBoards();
}

bool ChessBoard::hasMadeNullMove()
{
    return this->nullMove;
}

void ChessBoard::initFromFen(const std::string& fen)
{
    char token;
    Square src = Square::FIRST_SQUARE;
    PieceType piece;
    std::string enPassant;

    this->clearEverything();

    std::stringstream ss;
    ss.str(fen);

    ss >> std::noskipws;

    while ((ss >> token) && !isspace(token)) {
        if (isdigit(token)) {
            src = src + Direction::RIGHT * int(token - '0');
        }
        else if (token == '/') {
        }
        else if ((piece = PieceType(PieceToChar.find(token))) != std::string::npos) {
            Color color = isupper(token) ? Color::WHITE : Color::BLACK;
            piece = color == Color::WHITE ? PieceType(piece) : PieceType(piece - 8);

            this->pieces[src] = piece;

            if (color == Color::WHITE) {
                this->whitePieces[PieceType::ALL] |= src;
            }
            else {
                this->blackPieces[PieceType::ALL] |= src;
            }

            src++;
        }
    }

    ss >> token;
    this->sideToMove = token == 'w' ? Color::WHITE : Color::BLACK;
    ss >> token;

    this->castleRights = CastleRights::CASTLE_NONE;

    while ((ss >> token) && !isspace(token)) {
        switch (token) {
        case 'K':
            this->castleRights |= CastleRights::WHITE_OO;
            break;
        case 'Q':
            this->castleRights |= CastleRights::WHITE_OOO;
            break;
        case 'k':
            this->castleRights |= CastleRights::BLACK_OO;
            break;
        case 'q':
            this->castleRights |= CastleRights::BLACK_OOO;
            break;
        }
    }

    ss >> enPassant;

    if (enPassant != "-") {
        this->enPassant = StringToSquare(enPassant);
    }
    else {
        this->enPassant = Square::NO_SQUARE;
    }

    ss >> std::skipws >> this->fiftyMoveCount >> this->fullMoveCount;

    this->buildBitboardsFromMailbox();
    this->buildAttackBoards();

    this->materialEvaluation = this->calculateMaterialEvaluation();
    this->pstEvaluation = this->calculatePstEvaluation();

    this->hashValue = this->calculateHash();
    this->materialHashValue = this->calculateMaterialHash();
    this->pawnHashValue = this->calculatePawnHash();
}

void ChessBoard::resetSpecificPositionImplementation(const std::string& fen)
{
    this->initFromFen(fen);
}

void ChessBoard::resetStartingPositionImplementation()
{
    this->initFromFen(startingPositionFen);
}

template void ChessBoard::doMoveImplementation<false>(ChessMove& move);
template void ChessBoard::doMoveImplementation<true>(ChessMove& move);
