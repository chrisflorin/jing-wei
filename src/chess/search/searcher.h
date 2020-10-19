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

#include "../../game/search/hashtable.h"
#include "../../game/search/searcher.h"

#include "../board/movegen.h"

#include "../eval/evaluator.h"

#include "butterfly.h"
#include "movehistory.h"
#include "chesspv.h"

#include "../types/nodetype.h"
#include "../types/search.h"

static constexpr std::uint32_t SearchStackSize = Depth::MAX + 2;

class ChessSearcher : public Searcher<ChessSearcher, ChessEvaluator, ChessMoveGenerator, ChessMoveHistory, ChessPrincipalVariation>
{
protected:
    ChessAttackGenerator attackGenerator;
    ChessButterflyTable butterflyTable;
    Hashtable hashtable;

    MoveList<MoveType> rootMoveList;
    SearchStack searchStack[SearchStackSize];

    template <NodeType nodeType>
    HashtableEntryType checkHashtable(BoardType& board, Score& hashScore, Depth depthLeft, Depth currentDepth);

    template <NodeType nodeType>
    Score quiescenceSearch(BoardType& board, Score alpha, Score beta, Depth currentDepth, Depth maxDepth);

    template <NodeType nodeType>
    Score search(BoardType& board, Score alpha, Score beta, Depth maxDepth, Depth currentDepth);

    template <NodeType nodeType>
    Score searchLoop(BoardType& board, Score alpha, Score beta, Depth maxDepth, Depth currentDepth);
public:
    ChessSearcher();
    ~ChessSearcher();

    TwoPlayerGameResult checkBoardGameResult(BoardType& board, ChessMoveHistory moveHistory, bool checkMoveCount);

    void initializeSearchImplementation(BoardType& board);

    void resetHashtable();

    Score rootSearchImplementation(BoardType& board, ChessPrincipalVariation& pv, Depth maxDepth, Score alpha, Score beta);

    Score staticExchangeEvaluation(BoardType& board, Square src, Square dst);
};
