#pragma once

#include "../../game/book/book.h"

#include "../board/board.h"

#include "../types/move.h"

struct ChessBookPosition
{
	Hash hashValue;
	GameResultCount winCount;
	GameResultCount drawCount;
	GameResultCount lossCount;
	GameResultCount gameCount;
};

static ChessBookPosition& operator += (ChessBookPosition& bp1, ChessBookPosition bp2)
{
	bp1.winCount += bp2.winCount;
	bp1.drawCount += bp2.drawCount;
	bp1.lossCount += bp2.lossCount;
	bp1.gameCount += bp2.gameCount;

	return bp1;
}

typedef Book<ChessBoard, ChessBookPosition> ChessBook;
