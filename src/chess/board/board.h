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

#include <string>

#include "../../game/board/board.h"

#include "../../game/types/color.h"
#include "../../game/types/hash.h"
#include "../../game/types/nodecount.h"
#include "../../game/types/result.h"
#include "../../game/types/score.h"

#include "../../game/types/bitboard.h"

#include "../types/castlerights.h"
#include "../types/move.h"
#include "../types/nodetype.h"
#include "../types/piece.h"

class ChessBoard : public GameBoard<ChessBoard, ChessMove>
{
protected:
    void buildAttackBoards();
    void buildBitboardsFromMailbox();
    void clearEverything();
public:
    Bitboard whitePieces[PieceType::PIECETYPE_COUNT];
    Bitboard blackPieces[PieceType::PIECETYPE_COUNT];

    Bitboard allPieces;

    PieceType pieces[Square::SQUARE_COUNT];

    NodeCount fiftyMoveCount;
    NodeCount fullMoveCount;

    Bitboard blockedPieces, checkingPieces, inBetweenSquares, pinnedPieces;

    Hash hashValue, materialHashValue, pawnHashValue;

    Evaluation materialEvaluation, pstEvaluation;
    
    CastleRights castleRights;
    Color sideToMove;

    Square enPassant;
    Square whiteKingPosition, blackKingPosition;
protected:
    bool nullMove;

public:
    ChessBoard();
	~ChessBoard();

    Hash calculateHash();
    Hash calculateMaterialHash();
    Evaluation calculateMaterialEvaluation();
    Hash calculatePawnHash();
    Evaluation calculatePstEvaluation();

    template<bool performPreCalculations = true>
    void doMoveImplementation(ChessMove& move);
    void doNullMove();

    bool hasMadeNullMove();

    void initFromFen(const std::string& fen);

    void resetSpecificPositionImplementation(const std::string& fen);
    void resetStartingPositionImplementation();
};
