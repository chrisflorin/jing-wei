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
#include <cmath>
#include <iomanip>
#include <iostream>

#include "searcher.h"

#include "../types/score.h"

#include "../../game/math/bitreset.h"
#include "../../game/math/bitscan.h"
#include "../../game/math/sort.h"

static constexpr bool enableButterflyTable = enableAllSearchFeatures && true;
static constexpr bool enableFutilityPruning = enableAllSearchFeatures && true;
static constexpr bool enableSearchExtensions = enableAllSearchFeatures && true;
static constexpr bool enableSearchHashtable = enableAllSearchFeatures && true;
static constexpr bool enableSearchReductions = enableAllSearchFeatures && true;
static constexpr bool enableIID = enableAllSearchFeatures && true;
static constexpr bool enableMateDistancePruning = enableAllSearchFeatures && true;
static constexpr bool enableNullMove = enableAllSearchFeatures && true;
static constexpr bool enableQuiescenceEarlyExit = enableAllSearchFeatures && true;
static constexpr bool enableQuiscenceSearchHashtable = enableAllSearchFeatures && false;
static constexpr bool enableQuiescenceStaticExchangeEvaluation = enableAllSearchFeatures && true;

extern Bitboard BlackPawnCaptures[Square::SQUARE_COUNT];
extern Bitboard InBetween[Square::SQUARE_COUNT][Square::SQUARE_COUNT];
extern Bitboard PieceMoves[PieceType::PIECETYPE_COUNT][Square::SQUARE_COUNT];
extern Bitboard WhitePawnCaptures[Square::SQUARE_COUNT];

extern Evaluation MaterialParameters[PieceType::PIECETYPE_COUNT];
extern Evaluation LateMoveReductions[4];

ChessSearcher::ChessSearcher()
{
    if (enableSearchHashtable) {
        this->hashtable.initialize(65536);
    }

    this->moveHistory.reserve(4096);
}

ChessSearcher::~ChessSearcher()
{

}

TwoPlayerGameResult ChessSearcher::checkBoardGameResult(BoardType& board, ChessMoveHistory moveHistory, bool checkMoveCount)
{
    if (checkMoveCount) {
        MoveList<MoveType> moveList;
        NodeCount moveCount = this->moveGenerator.generateAllMoves(board, moveList, true);

        if (moveCount == ZeroNodes) {
            bool isInCheck = this->attackGenerator.isInCheck(board);

            if (isInCheck) {
                return TwoPlayerGameResult::LOSS;
            }
            else {
                return TwoPlayerGameResult::DRAW;
            }
        }
    }

    //50 Move Count Draw detection
    if (board.fiftyMoveCount >= 100) {
        return TwoPlayerGameResult::DRAW;
    }

    //Repetition Draw detection
    std::uint32_t repetitionCount = moveHistory.checkForDuplicateHash(board.hashValue);
    if (repetitionCount > 1) {
        return TwoPlayerGameResult::DRAW;
    }

    //Check for InsufficientMaterial
    if (this->evaluator.checkBoardForInsufficientMaterial(board)) {
        return TwoPlayerGameResult::DRAW;
    }

    return TwoPlayerGameResult::NO_GAMERESULT;
}

template <NodeType nodeType>
HashtableEntryType ChessSearcher::checkHashtable(BoardType& board, Score& hashScore, Depth depthLeft, Depth currentDepth)
{
    HashtableEntryType hashtableEntryType = HASHENTRYTYPE_NONE;
    Depth hashDepthLeft;
    std::uint8_t custom;

    if (nodeType != NodeType::PV_NODETYPE) {
        hashtableEntryType = this->hashtable.search(board.hashValue, hashScore, currentDepth, hashDepthLeft, custom);

        if (hashtableEntryType != HASHENTRYTYPE_NONE
            && hashDepthLeft >= depthLeft) {
            return hashtableEntryType;
        }
    }

    return HASHENTRYTYPE_NONE;
}

void ChessSearcher::initializeSearchImplementation(BoardType& board)
{
    if (enableButterflyTable) {
        this->butterflyTable.reset();
    }

    this->hashtable.incrementAge();

    this->moveGenerator.generateAllMoves(board, this->rootMoveList);

    this->nodeCount = ZeroNodes;
}

template <NodeType nodeType>
Score ChessSearcher::quiescenceSearch(BoardType& board, Score alpha, Score beta, Depth currentDepth, Depth maxDepth)
{
    //1) Check for instant abort conditions
    if (currentDepth >= (Depth::MAX - Depth::ONE)) {
        this->abortedSearch = true;
        return alpha;
    }

    //2) Increment quiescent node count
    this->nodeCount++;

    //3) Search Hashtable
    Depth depthLeft = maxDepth - currentDepth;
    HashtableEntryType hashtableEntryType = HASHENTRYTYPE_NONE;
    Score hashScore;

    if (enableQuiscenceSearchHashtable) {
        hashtableEntryType = this->checkHashtable<nodeType>(board, hashScore, depthLeft, currentDepth);

        switch (hashtableEntryType) {
        case HASHENTRYTYPE_EXACT_VALUE:
            return hashScore;
            break;
        case HASHENTRYTYPE_LOWER_BOUND:
            if (hashScore >= alpha) {
                return hashScore;
            }
            break;
        case HASHENTRYTYPE_UPPER_BOUND:
            if (hashScore <= alpha) {
                return hashScore;
            }
            break;
        }
    }

    //4) Evaluate board statically, for a stand-pat option
    bool isInCheck = this->attackGenerator.isInCheck(board);
    Score staticScore;

    if (isInCheck) {
        staticScore = -WIN_SCORE + currentDepth;
    }
    else {
        staticScore = this->evaluator.evaluate(board, alpha, beta);

        if (staticScore > alpha) {
            if (staticScore >= beta) {
                return staticScore;
            }

            alpha = staticScore;
        }
    }

    //5) Generate Moves.  Return if Checkmate.
    SearchStack& searchStack = this->searchStack[currentDepth];
    MoveList<MoveType>& moveList = searchStack.moveList;
    NodeCount moveCount = this->moveGenerator.generateAllCaptures(board, moveList);

    if (moveCount == ZeroNodes) {
        if (isInCheck) {
            return -WIN_SCORE + currentDepth;
        }

        return staticScore;
    }

    //6) Attempt to reorder moves to improve the probability of finding the best move first
    if (isInCheck) {
        this->moveGenerator.reorderMoves<nodeType>(board, moveList, searchStack, this->butterflyTable);
    }
    else {
        this->moveGenerator.reorderQuiescenceMoves<nodeType>(board, moveList, searchStack);
    }

    //7) MoveList loop
    Score bestScore = staticScore;
    NodeCount movesSearched = ZeroNodes;

    for (MoveList<MoveType>::iterator it = moveList.begin(); it != moveList.end(); ++it) {
        MoveType& move = *it;

        Square src = move.src;
        Square dst = move.dst;

        PieceType capturedPiece = board.pieces[dst];

        //8) If capturing this piece won't bring us close to alpha, skip the capture
        if (enableQuiescenceEarlyExit
            && !isInCheck) {
            Score capturedPieceScore = MaterialParameters[capturedPiece].mg;
            Score lazyScore = staticScore + capturedPieceScore;

            constexpr Score earlyExitThreshold = Score(2 * PAWN_SCORE);

            if (lazyScore + earlyExitThreshold < alpha) {
                continue;
            }
        }

        //9) Static Exchange Evaluation
        constexpr Score staticExchangeEvaluationExitThreshold = Score(1 * PAWN_SCORE);

        if (enableQuiescenceStaticExchangeEvaluation
            && !isInCheck
            && (this->staticExchangeEvaluation(board, src, dst) < staticExchangeEvaluationExitThreshold)) {
            continue;
        }

        //10) DoMove
        BoardType nextBoard = board;
        nextBoard.doMove(move);

        //11) Recurse to next depth
        Score nextScore;

        switch (nodeType) {
        case NodeType::PV_NODETYPE:
            if (movesSearched == ZeroNodes) {
                nextScore = -this->quiescenceSearch<NodeType::PV_NODETYPE>(nextBoard, -beta, -alpha, currentDepth + Depth::ONE, maxDepth);
            }
            else {
                nextScore = -this->quiescenceSearch<NodeType::CUT_NODETYPE>(nextBoard, -(alpha + 1), -alpha, currentDepth + Depth::ONE, maxDepth);

                if (nextScore > alpha && nextScore < beta) {
                    nextScore = -this->quiescenceSearch<NodeType::PV_NODETYPE>(nextBoard, -beta, -alpha, currentDepth + Depth::ONE, maxDepth);
                }
            }
            break;
        case NodeType::CUT_NODETYPE:
            nextScore = -this->quiescenceSearch<NodeType::ALL_NODETYPE>(nextBoard, -(alpha + 1), -alpha, currentDepth + Depth::ONE, maxDepth);
            break;
        case NodeType::ALL_NODETYPE:
            nextScore = -this->quiescenceSearch<NodeType::CUT_NODETYPE>(nextBoard, -(alpha + 1), -alpha, currentDepth + Depth::ONE, maxDepth);
            break;
        }

        //12) Undo Move

        //13) Compare returned value to alpha/beta
        if (nextScore > bestScore) {
            bestScore = nextScore;
        }

        if (nextScore > alpha) {
            if (nextScore >= beta) {
                break;
            }

            alpha = nextScore;
        }

        movesSearched++;
    }

    //14) Store Result in Hashtable
    if (enableQuiscenceSearchHashtable) {
        HashtableEntryType hashtableEntryType = HASHENTRYTYPE_NONE;

        if (bestScore >= beta) {
            hashtableEntryType = HASHENTRYTYPE_LOWER_BOUND;
        }
        else if (bestScore < alpha) {
            hashtableEntryType = HASHENTRYTYPE_UPPER_BOUND;
        }

        if (hashtableEntryType != HASHENTRYTYPE_NONE) {
            this->hashtable.insert(board.hashValue, bestScore, currentDepth, depthLeft, hashtableEntryType, 0);
        }
    }

    return bestScore;
}

void ChessSearcher::resetHashtable()
{
    this->hashtable.reset();
}

Score ChessSearcher::rootSearchImplementation(BoardType& board, ChessPrincipalVariation& principalVariation, Depth maxDepth, Score alpha, Score beta)
{
    Score bestScore = -WIN_SCORE;
    constexpr Depth currentDepth = Depth::ZERO;

    Score score;

    NodeCount movesSearched = ZeroNodes;

    for (MoveList<MoveType>::iterator it = this->rootMoveList.begin(); it != this->rootMoveList.end(); ++it) {
        BoardType nextBoard = board;
        MoveType& move = (*it);

        nextBoard.doMove(move);
        this->addMoveToHistory(nextBoard, move);

        if (movesSearched == ZeroNodes) {
            score = -this->search<NodeType::PV_NODETYPE>(nextBoard, -beta, -alpha, maxDepth, Depth::ONE);
        }
        else {
            score = -this->search<NodeType::CUT_NODETYPE>(nextBoard, -(alpha + 1), -alpha, maxDepth, Depth::ONE);

            if (score > alpha) {
                score = -this->search<NodeType::PV_NODETYPE>(nextBoard, -beta, -alpha, maxDepth, Depth::ONE);
            }
        }

        this->removeLastMoveFromHistory();

        if (this->abortedSearch) {
            break;
        }

        move.ordinal = ChessMoveOrdinal(score);

        if (score > bestScore) {
            bestScore = score;

            if (bestScore >= beta) {
                break;
            }
        }

        if (score > alpha
            || movesSearched == ZeroNodes) {
            alpha = score;

            std::cout << int(maxDepth / Depth::ONE) << " " << std::fixed << std::setprecision(2);
            if (IsMateScore(score)) {
                if (score > (WIN_SCORE - Depth::MAX)) {
                    score = 10000 - (WIN_SCORE - score);
                }
                else if (score < (-WIN_SCORE + Depth::MAX)) {
                    score = -10000 + (WIN_SCORE + score);
                }

                std::cout << score / 100.0f;
            }
            else {
                std::cout << int(score / (PAWN_SCORE / 100.0f));
            }

            std::time_t time = clock.getElapsedTime(this->nodeCount);

            std::cout << " " << time / 10 << " " << (this->nodeCount) << " ";

            ChessPrincipalVariation& nextPrincipalVariation = this->searchStack[currentDepth + Depth::ONE].principalVariation;

            principalVariation.copyBackward(nextPrincipalVariation, move);
            principalVariation.print();

            std::cout << std::endl;
        }

        movesSearched++;
    }

    MoveType move = principalVariation[0];
    score = Score(move.ordinal);

    std::cout << int(maxDepth / Depth::ONE) << " " << std::fixed << std::setprecision(2);
    if (IsMateScore(score)) {
        if (score > (WIN_SCORE - Depth::MAX)) {
            score = 10000 - (WIN_SCORE - score);
        }
        else if (score < (-WIN_SCORE + Depth::MAX)) {
            score = -10000 + (WIN_SCORE + score);
        }

        std::cout << score / 100.0f;
    }
    else {
        std::cout << int(score / (PAWN_SCORE / 100.0f));
    }

    std::time_t time = clock.getElapsedTime(this->nodeCount);
    std::cout << " " << time / 10 << " " << (this->nodeCount) << " ";

    principalVariation.print();

    std::cout << std::endl;

    std::stable_sort(this->rootMoveList.begin(), this->rootMoveList.begin() + this->rootMoveList.size(), greater<MoveType>);

    return bestScore;
}

template <NodeType nodeType>
Score ChessSearcher::search(BoardType& board, Score alpha, Score beta, Depth maxDepth, Depth currentDepth)
{
    bool whiteToMove = board.sideToMove == Color::WHITE;

    //1) Check for instant abort conditions
    if (currentDepth >= (Depth::MAX - Depth::ONE)) {
        this->abortedSearch = true;

        return NO_SCORE;
    }

    if (!this->clock.shouldContinueSearch(Depth::ZERO, this->getNodeCount())) {
        this->abortedSearch = true;

        return NO_SCORE;
    }

    //2) Check for Draw condition
    SearchStack& searchStack = this->searchStack[currentDepth];
    ChessPrincipalVariation& currentPrincipalVariation = searchStack.principalVariation;

    TwoPlayerGameResult gameResult = this->checkBoardGameResult(board, this->moveHistory, false);
    if (gameResult == TwoPlayerGameResult::DRAW) {
        currentPrincipalVariation.clear();

        return DRAW_SCORE;
    }

    //3) Mate Distance Pruning
    if (enableMateDistancePruning) {
        alpha = std::max(-WIN_SCORE + currentDepth, alpha);
        beta = std::min(WIN_SCORE - (currentDepth + Depth::ONE), beta);

        if (alpha >= beta) {
            currentPrincipalVariation.clear();

            return alpha;
        }
    }

    //4) If we're at max Depth, Enter Quiescence Search
    bool isInCheck = this->attackGenerator.isInCheck(board);
    if (!isInCheck && (currentDepth >= maxDepth)) {
        currentPrincipalVariation.clear();

        return this->quiescenceSearch<nodeType>(board, alpha, beta, currentDepth, maxDepth);
    }

    //5) Increment the Node Count
    this->nodeCount++;

    //6) Search Hashtable
    Depth depthLeft = maxDepth - currentDepth;
    HashtableEntryType hashtableEntryType = HASHENTRYTYPE_NONE;
    Score hashScore;

    if (enableSearchHashtable) {
        hashtableEntryType = this->checkHashtable<nodeType>(board, hashScore, depthLeft, currentDepth);

        switch (hashtableEntryType) {
        case HASHENTRYTYPE_LOWER_BOUND:
            if (hashScore >= alpha) {
                currentPrincipalVariation.clear();

                return hashScore;
            }
            break;
        case HASHENTRYTYPE_UPPER_BOUND:
            if (hashScore <= alpha) {
                currentPrincipalVariation.clear();

                return hashScore;
            }
            break;
        default:
            break;
        }
    }

    //7) Null Move Search
    bool isMateSearch = IsMateScore(alpha);
    bool isMateThreat = false;

    if (enableNullMove
        && !isMateSearch
        && hashtableEntryType == HASHENTRYTYPE_NONE
        && !board.hasMadeNullMove()
        && nodeType != NodeType::PV_NODETYPE
        && !isInCheck
        && depthLeft > Depth::TWO) {
        BoardType nextBoard = board;
        nextBoard.doNullMove();

        constexpr Depth nullReduction = Depth::THREE;
        Score nullScore = -this->search<NodeType::ALL_NODETYPE>(nextBoard, -beta, -beta + 1, maxDepth - nullReduction, currentDepth + Depth::ONE);

        isMateThreat = IsMateScore(nullScore);
        if (!isMateThreat
            && (nullScore >= beta)) {
            Score verifiedNullScore = this->search<nodeType>(board, beta - 1, beta, maxDepth - nullReduction, currentDepth);

            isMateThreat = IsMateScore(verifiedNullScore);
            if (!isMateThreat
                && (verifiedNullScore >= beta)) {
                currentPrincipalVariation.clear();

                return nullScore;
            }
        }
    }

    //8) Futility Pruning
    searchStack.staticEvaluation = isInCheck ? -WIN_SCORE + currentDepth : this->evaluator.evaluate(board, alpha, beta);

    if (enableFutilityPruning
        && !isMateThreat
        && !isMateSearch
        && hashtableEntryType == HASHENTRYTYPE_NONE
        && nodeType != NodeType::PV_NODETYPE
        && !isInCheck
        && depthLeft < Depth::FOUR) {
        Score futilityMargin = PAWN_SCORE * depthLeft;//GetFutilityMargin(depthLeft);
        Score staticEvaluation = searchStack.staticEvaluation;

        if ((staticEvaluation - futilityMargin) >= beta) {
            currentPrincipalVariation.clear();
            
            return staticEvaluation;
        }
    }

    //9) Generate Moves.  Return if Checkmate.
    MoveList<MoveType>& moveList = searchStack.moveList;

    this->moveGenerator.generateAllMoves(board, moveList);

    if (moveList.size() == ZeroNodes) {
        currentPrincipalVariation.clear();

        if (isInCheck) {
            return -WIN_SCORE + currentDepth;
        }
        else {
            return DRAW_SCORE;
        }
    }

    //10) Begin Search Loop
    Score resultScore = this->searchLoop<nodeType>(board, alpha, beta, maxDepth, currentDepth);

    //11) Store Result in Hashtable
    if (enableSearchHashtable) {
        HashtableEntryType hashtableEntryType = HASHENTRYTYPE_NONE;

        if (resultScore >= beta) {
            hashtableEntryType = HASHENTRYTYPE_LOWER_BOUND;
        }
        else if (resultScore < alpha) {
            hashtableEntryType = HASHENTRYTYPE_UPPER_BOUND;
        }

        if (hashtableEntryType != HASHENTRYTYPE_NONE) {
            this->hashtable.insert(board.hashValue, resultScore, currentDepth, depthLeft, hashtableEntryType, 0);
        }
    }

    return resultScore;
}

template <NodeType nodeType>
Score ChessSearcher::searchLoop(BoardType& board, Score alpha, Score beta, Depth maxDepth, Depth currentDepth)
{
    Depth depthLeft = maxDepth - currentDepth;
    SearchStack& searchStack = this->searchStack[currentDepth];

    MoveList<MoveType>& moveList = searchStack.moveList;

    //1) Internal Iterative Deepening
    if (enableIID
        //&& nodeType != NodeType::PV_NODE
        && depthLeft > Depth::THREE) {
        Depth iirReduction = Depth::THREE;
        this->searchLoop<nodeType>(board, alpha, beta, maxDepth - iirReduction, currentDepth);

        std::stable_sort(moveList.begin(), moveList.end(), greater<MoveType>);
    }
    else {
        //Sort Moves by expected value
        this->moveGenerator.reorderMoves<nodeType>(board, moveList, searchStack, this->butterflyTable);
    }

    ChessPrincipalVariation& currentPrincipalVariation = searchStack.principalVariation;
    ChessPrincipalVariation& nextPrincipalVariation = this->searchStack[currentDepth + Depth::ONE].principalVariation;

    //2) Calculate Position Extensions (independent of type of move)
    Depth positionExtensions = Depth::ZERO;

    if (enableSearchExtensions
        && currentDepth >= Depth::TWO) {
        bool isInCheck = this->attackGenerator.isInCheck(board);

        //***Don't use this reference if currentDepth <= 1***
//        SearchStack& ourLastSearchStack = this->searchStack[currentDepth - Depth::TWO];

        if (isInCheck) {
            positionExtensions += Depth::ONE;
        }
        //else if (!isInCheck
        //    && currentDepth >= 2
        //    && searchStack.staticEvaluation >= BASICALLY_WINNING_SCORE
        //    && ourLastSearchStack.staticEvaluation < BASICALLY_WINNING_SCORE) {
        //    positionExtensions += Depth::ONE;
        //}
    }

    //3) MoveList loop
    NodeCount searchedMoves = ZeroNodes;
    NodeCount quietMoves = ZeroNodes;
    Score bestScore = -WIN_SCORE;

    for (MoveList<MoveType>::iterator it = moveList.begin(); it != moveList.end(); ++it) {
        MoveType& move = (*it);

        Square src = move.src;
        Square dst = move.dst;

        PieceType movingPiece = board.pieces[src];
        PieceType capturedPiece = board.pieces[dst];
        PieceType promotionPiece = move.promotionPiece;

        //4) Calculate Reductions
        Depth extensions = positionExtensions;

        if (enableSearchReductions
            && nodeType != NodeType::PV_NODETYPE
            && extensions == Depth::ZERO
            && searchedMoves > ZeroNodes) {
            float l1 = (1.0f + LateMoveReductions[0].mg / 100.0f) * std::log(float(currentDepth + Depth::ONE));
            float l2 = (1.0f + LateMoveReductions[1].mg / 100.0f) * std::log(float(depthLeft + Depth::ONE));
            float l3 = (1.0f + LateMoveReductions[2].mg / 100.0f) * std::log(float(searchedMoves + 1));

            float reduction = (1.0f + LateMoveReductions[3].mg) * std::log(l1 * l2 * l3 + 1.0f);

            extensions -= Depth::ONE * reduction;

            constexpr Score staticExchangeEvaluationReductionThreshold = Score(1 * PAWN_SCORE);
            if (this->staticExchangeEvaluation(board, src, dst) < staticExchangeEvaluationReductionThreshold) {
                extensions -= Depth::ONE;
            }
        }

        //5) DoMove
        BoardType nextBoard = board;
        nextBoard.doMove(move);
        this->addMoveToHistory(nextBoard, move);

        //6) Recurse to next depth
        Score nextScore;

        switch (nodeType) {
        case NodeType::PV_NODETYPE:
            if (searchedMoves == ZeroNodes) {
                nextScore = -this->search<NodeType::PV_NODETYPE>(nextBoard, -beta, -alpha, maxDepth + extensions, currentDepth + Depth::ONE);
            }
            else {
                nextScore = -this->search<NodeType::CUT_NODETYPE>(nextBoard, -(alpha + 1), -alpha, maxDepth + extensions, currentDepth + Depth::ONE);

                if (nextScore > alpha && nextScore < beta) {
                    nextScore = -this->search<NodeType::PV_NODETYPE>(nextBoard, -beta, -alpha, maxDepth + extensions, currentDepth + Depth::ONE);
                }
            }
            break;
        case NodeType::CUT_NODETYPE:
            nextScore = -this->search<NodeType::ALL_NODETYPE>(nextBoard, -(alpha + 1), -alpha, maxDepth + extensions, currentDepth + Depth::ONE);

            if ((nextScore > alpha)
                && (extensions < 0)) {
                nextScore = -this->search<NodeType::ALL_NODETYPE>(nextBoard, -(alpha + 1), -alpha, maxDepth, currentDepth + Depth::ONE);
            }
            break;
        case NodeType::ALL_NODETYPE:
            nextScore = -this->search<NodeType::CUT_NODETYPE>(nextBoard, -(alpha + 1), -alpha, maxDepth + extensions, currentDepth + Depth::ONE);

            if ((nextScore > alpha)
                && (extensions < 0)) {
                nextScore = -this->search<NodeType::CUT_NODETYPE>(nextBoard, -(alpha + 1), -alpha, maxDepth, currentDepth + Depth::ONE);
            }
            break;
        }

        //7) Undo Move
        this->removeLastMoveFromHistory();

        //8) Compare returned value to alpha/beta
        move.ordinal = ChessMoveOrdinal(nextScore);

        if (nextScore > bestScore) {
            searchStack.bestMove = move;

            bestScore = nextScore;
        }

        if (nextScore > alpha) {
            if (nextScore >= beta) {
                if (enableButterflyTable) {
                    this->butterflyTable.add(movingPiece, dst, 1);
                }

                if ((capturedPiece == PieceType::NO_PIECE) && (promotionPiece == PieceType::NO_PIECE)) {
                    if (searchStack.killer1 != move) {
                        searchStack.killer2 = searchStack.killer1;
                        searchStack.killer1 = move;
                    }
                }

                currentPrincipalVariation.clear();

                return nextScore;
            }

            alpha = nextScore;

            //If not Internal Iterative Deepening, copy the PV back
            currentPrincipalVariation.copyBackward(nextPrincipalVariation, move);
            searchStack.pvMove = move;
        }

        if ((capturedPiece == PieceType::NO_PIECE) && (promotionPiece == PieceType::NO_PIECE)) {
            quietMoves++;
        }

        searchedMoves++;
    }

    return bestScore;
}

#define SeeMaterialValue(x) (MaterialParameters[x].mg)

Score ChessSearcher::staticExchangeEvaluation(BoardType& board, Square src, Square dst)
{
    PieceType movingPiece = board.pieces[src];
    PieceType capturedPiece = board.pieces[dst];

    bool whiteToMove = board.sideToMove == Color::WHITE;

    Bitboard* piecesToMove = whiteToMove ? board.whitePieces : board.blackPieces;
    Bitboard* otherPieces = whiteToMove ? board.blackPieces : board.whitePieces;

    Score gain[32];

    PieceType bestKnown[Color::COLOR_COUNT] = { PieceType::PAWN, PieceType::PAWN };

    //1) If the captured piece value > the moving piece value, return a quick SEE score
    if (SeeMaterialValue(capturedPiece) > SeeMaterialValue(movingPiece)) {
        return SeeMaterialValue(capturedPiece) - SeeMaterialValue(movingPiece);
    }

    //2) Get all of the valid pieces on the board that can be used for SEE
    Bitboard validAttackers = WhitePawnCaptures[dst] & board.blackPieces[PieceType::PAWN]
        | BlackPawnCaptures[dst] & board.whitePieces[PieceType::PAWN];

    for (PieceType piece = PieceType::KNIGHT; piece <= PieceType::KING; piece++) {
        validAttackers |= PieceMoves[piece][dst] & (board.whitePieces[piece] | board.blackPieces[piece]);
    }

    if ((validAttackers & board.allPieces) == EmptyBitboard) {
        return ZERO_SCORE;
    }

    //3) Exclude the original captured piece and the first attacker on the source square
    validAttackers ^= OneShiftedBy(src) ^ OneShiftedBy(dst);

    //4) Set Side To Move Attackers
    whiteToMove = !whiteToMove;
    std::swap(piecesToMove, otherPieces);

    Bitboard sideToMoveAttackers = validAttackers & piecesToMove[PieceType::ALL];

    //5) If the other side has no pieces to recapture, return the value of the captured piece
        //without capturing the moved piece
    if (sideToMoveAttackers == EmptyBitboard) {
        return SeeMaterialValue(capturedPiece);
    }

    //6) Begin the loop
    gain[0] = SeeMaterialValue(capturedPiece);
    std::int32_t depth = 1;

    capturedPiece = movingPiece;

    Bitboard attackingPieces;

    bool found = false;
    do {
        found = false;

        //7) Get the least valuable attacker for the side to move
        PieceType currentPiece = bestKnown[whiteToMove];

        while (!found && currentPiece <= PieceType::KING) {
            attackingPieces = piecesToMove[currentPiece];

            if ((sideToMoveAttackers & attackingPieces) != EmptyBitboard) {
                found = true;
            }
            else {
                currentPiece++;
            }
        }

        if (!found) {
            break;
        }

        //7b) Save the least valueable attacker for the side to move
        bestKnown[whiteToMove] = currentPiece;

        //8) Remove the attacker from the attackers bitboard
        Bitboard b = sideToMoveAttackers & attackingPieces;
        Square attackSrc;

        BitScanForward64((std::uint32_t *)&attackSrc, b);
        validAttackers = ResetLowestSetBit(validAttackers);

        //9) If there are no pieces in between the attacker and the destination square, allow
        //  the piece to participate in the SEE
        if ((InBetween[attackSrc][dst] & board.allPieces) == EmptyBitboard) {
            gain[depth] = SeeMaterialValue(capturedPiece) - gain[depth - 1];

            if (capturedPiece == PieceType::KING) {
                break;
            }

            //10) Early pruning of SEE score
            if (-gain[depth - 1] < 0
                && gain[depth] < 0) {
                break;
            }

            depth++;

            capturedPiece = currentPiece;

            whiteToMove = !whiteToMove;
            std::swap(piecesToMove, otherPieces);
        }

        sideToMoveAttackers = validAttackers & piecesToMove[PieceType::ALL];
    } while ((sideToMoveAttackers != EmptyBitboard) || !found);

    //11) Finish calculating the SEE
    while (depth--) {
        gain[depth - 1] = std::min(-gain[depth], gain[depth - 1]);
    }

    return gain[0];
}
