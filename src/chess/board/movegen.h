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
#include <vector>

#include "../../game/board/movegen.h"

#include "attack.h"
#include "board.h"

#include "../search/butterfly.h"
#include "../search/chesspv.h"

#include "../types/search.h"

#include "../../game/search/pv.h"

#include "../../game/types/movelist.h"
#include "../../game/types/nodecount.h"

class ChessMoveGenerator : public MoveGenerator<ChessMoveGenerator, ChessBoard>
{
protected:
    ChessAttackGenerator attackGenerator;

    bool shouldDoubleCheckGeneratedMoves(BoardType& board);
public:
    using MoveType = typename ChessBoard::MoveType;

    ChessMoveGenerator();
    ~ChessMoveGenerator();

    NodeCount doubleCheckGeneratedMoves(BoardType& board, MoveList<MoveType>& moveList);

    NodeCount generateAllCaptures(BoardType& board, MoveList<MoveType>& moveList);
    NodeCount generateAllMovesImplementation(BoardType& board, MoveList<MoveType>& moveList, bool countOnly);
    NodeCount generateAttacksOnSquares(BoardType& board, MoveList<MoveType>& moveList, Bitboard dstSquares, Bitboard excludeSrcSquares);
    NodeCount generateCheckEvasions(BoardType& board, MoveList<MoveType>& moveList);
    NodeCount generateMovesToSquares(BoardType& board, MoveList<MoveType>& moveList, Bitboard dstSquares, Bitboard excludeSrcSquares);

    NodeCount perft(BoardType& board, Depth maxDepth, Depth currentDepth);

    template <NodeType nodeType>
    void reorderMoves(BoardType& board, MoveList<MoveType>& moveList, SearchStack& searchStack, ChessButterflyTable& butterflyTable);

    template <NodeType nodeType>
    void reorderQuiescenceMoves(BoardType& board, MoveList<MoveType>& moveList, SearchStack& searchStack);
};
