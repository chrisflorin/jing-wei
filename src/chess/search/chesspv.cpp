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

#include "chesspv.h"

#include <iostream>

static const std::string FilePrint = "abcdefgh";
static const std::string PiecePrint = ".pnbrqk";
static const std::string RankPrint = "87654321";

ChessPrincipalVariation::ChessPrincipalVariation()
{

}

ChessPrincipalVariation::~ChessPrincipalVariation()
{

}

void ChessPrincipalVariation::printMoveToConsoleImplementation(ChessMove& move)
{
    Square src = move.src, dst = move.dst;

    File srcFile = getFile(src), dstFile = getFile(dst);
    Rank srcRank = getRank(src), dstRank = getRank(dst);

    PieceType promotionPiece = move.promotionPiece;

    const char* file = FilePrint.c_str();
    const char* rank = RankPrint.c_str();

    std::cout << file[srcFile] << rank[srcRank] << file[dstFile] << rank[dstRank];

    if (promotionPiece != PieceType::NO_PIECE) {
        const char* print = PiecePrint.c_str();

        std::cout << print[promotionPiece];
    }
}

void ChessPrincipalVariation::stringToMoveImplementation(std::string& moveString, ChessMove& move)
{
    const char* moveChars = moveString.c_str();

    File srcFile, dstFile;
    Rank srcRank, dstRank;

    srcFile = File(FilePrint.find(moveChars[0]));
    srcRank = Rank(RankPrint.find(moveChars[1]));

    move.src = srcFile * srcRank;

    bool isCapture = moveChars[2] == 'x';

    dstFile = File(FilePrint.find(moveChars[isCapture ? 3 : 2]));
    dstRank = Rank(RankPrint.find(moveChars[isCapture ? 4 : 3]));

    move.dst = dstFile * dstRank;

    char promotion = moveChars[isCapture ? 5 : 4];
    if (promotion != '\0') {
        promotion = tolower(promotion);

        move.promotionPiece = PieceType(PiecePrint.find(promotion));
    }
    else {
        move.promotionPiece = PieceType::NO_PIECE;
    }

    move.ordinal = ChessMoveOrdinal::NO_CHESS_MOVE_ORDINAL;
}
