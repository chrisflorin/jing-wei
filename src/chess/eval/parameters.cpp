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

#include <cmath>

#include "../../game/personality/parametermap.h"

#include "../../game/types/score.h"

#include "../eval/constructor.h"

#include "../types/piece.h"
#include "../types/score.h"
#include "../types/square.h"

Evaluation MaterialParameters[PieceType::PIECETYPE_COUNT] = {
	{ ZERO_SCORE, ZERO_SCORE },
	{ PAWN_SCORE, PAWN_SCORE },
	{ KNIGHT_SCORE, KNIGHT_SCORE },
	{ BISHOP_SCORE, BISHOP_SCORE },
	{ ROOK_SCORE, ROOK_SCORE },
	{ QUEEN_SCORE, QUEEN_SCORE },
	{ WIN_SCORE, WIN_SCORE }	//The King's value is used in Static Exchange Evaluation; it needs to be a "win" to capture it
};

Evaluation PiecePairs[PieceType::PIECETYPE_COUNT];

Evaluation LateMoveReductions[4];

Evaluation BoardControlPstParameters[Square::SQUARE_COUNT];
Evaluation KingControlPstParameters[Square::SQUARE_COUNT];

Evaluation DoubledRooks;
Evaluation EmptyFileQueen;
Evaluation EmptyFileRook;
Evaluation GoodBishopPawns[8];
Evaluation QueenBehindPassedPawnPst[Square::SQUARE_COUNT];
Evaluation RookBehindPassedPawnPst[Square::SQUARE_COUNT];

Evaluation PawnChainBackDefault;
Evaluation PawnChainFrontDefault;
Evaluation PawnDoubledDefault;
Evaluation PawnPassedDefault;
Evaluation PawnTripledDefault;

Evaluation PawnChainBackPstParameters[Square::SQUARE_COUNT];
Evaluation PawnChainFrontPstParameters[Square::SQUARE_COUNT];
Evaluation PawnDoubledPstParameters[Square::SQUARE_COUNT];
Evaluation PawnPassedPstParameters[Square::SQUARE_COUNT];
Evaluation PawnTripledPstParameters[Square::SQUARE_COUNT];

Evaluation PstParameters[PieceType::PIECETYPE_COUNT][Square::SQUARE_COUNT];

static ScoreConstructor scoreConstructor;

PstConstruct pstConstruct[PieceType::PIECETYPE_COUNT];

PstConstruct boardControlPstConstruct;
PstConstruct kingControlPstConstruct;

PstConstruct pawnChainBackPstConstruct;
PstConstruct pawnChainFrontPstConstruct;
PstConstruct pawnDoubledPstConstruct;
PstConstruct pawnPassedPstConstruct;
PstConstruct pawnTripledPstConstruct;

Evaluation queenBehindPassedPawnDefault;
PstConstruct queenBehindPassedPawnPstConstruct;

Evaluation rookBehindPassedPawnDefault;
PstConstruct rookBehindPassedPawnPstConstruct;

Evaluation AttackParameters[PieceType::PIECETYPE_COUNT][PieceType::PIECETYPE_COUNT];
Evaluation BetterMobilityParameters[PieceType::PIECETYPE_COUNT][32];
Evaluation MobilityParameters[PieceType::PIECETYPE_COUNT][32];
Evaluation SafeMobilityParameters[PieceType::PIECETYPE_COUNT][32];
Evaluation TropismParameters[PieceType::PIECETYPE_COUNT][16];

QuadraticConstruct goodBishopPawnConstructor;
QuadraticConstruct betterMobilityConstructor[PieceType::PIECETYPE_COUNT];
QuadraticConstruct mobilityConstructor[PieceType::PIECETYPE_COUNT];
QuadraticConstruct safeMobilityConstructor[PieceType::PIECETYPE_COUNT];
QuadraticConstruct tropismConstructor[PieceType::PIECETYPE_COUNT];

std::uint32_t Distance[File::FILE_COUNT][Rank::RANK_COUNT];

ParameterMap chessEngineParameterMap = {
	{ "material-pawn-mg", &MaterialParameters[PieceType::PAWN].mg },
	{ "material-pawn-eg", &MaterialParameters[PieceType::PAWN].eg },
	{ "material-knight-mg", &MaterialParameters[PieceType::KNIGHT].mg },
	{ "material-knight-eg", &MaterialParameters[PieceType::KNIGHT].eg },
	{ "material-bishop-mg", &MaterialParameters[PieceType::BISHOP].mg },
	{ "material-bishop-eg", &MaterialParameters[PieceType::BISHOP].eg },
	{ "material-rook-mg", &MaterialParameters[PieceType::ROOK].mg },
	{ "material-rook-eg", &MaterialParameters[PieceType::ROOK].eg },
	{ "material-queen-mg", &MaterialParameters[PieceType::QUEEN].mg },
	{ "material-queen-eg", &MaterialParameters[PieceType::QUEEN].eg },

	{ "material-knight-pair-mg", &PiecePairs[PieceType::KNIGHT].mg},
	{ "material-knight-pair-eg", &PiecePairs[PieceType::KNIGHT].eg},
	{ "material-bishop-pair-mg", &PiecePairs[PieceType::BISHOP].mg},
	{ "material-bishop-pair-eg", &PiecePairs[PieceType::BISHOP].eg},
	{ "material-rook-pair-mg", &PiecePairs[PieceType::ROOK].mg},
	{ "material-rook-pair-eg", &PiecePairs[PieceType::ROOK].eg},
	{ "material-queen-pair-mg", &PiecePairs[PieceType::QUEEN].mg},
	{ "material-queen-pair-eg", &PiecePairs[PieceType::QUEEN].eg},

	{ "pst-pawn-rank-mg", &pstConstruct[PieceType::PAWN].mg.rank },
	{ "pst-pawn-rank-eg",& pstConstruct[PieceType::PAWN].eg.rank },
	{ "pst-pawn-file-center-mg", &pstConstruct[PieceType::PAWN].mg.filecenter },
	{ "pst-pawn-file-center-eg",&pstConstruct[PieceType::PAWN].eg.filecenter },

	{ "pst-knight-rank-mg", &pstConstruct[PieceType::KNIGHT].mg.rank },
	{ "pst-knight-rank-eg", &pstConstruct[PieceType::KNIGHT].eg.rank },
	{ "pst-knight-file-center-mg", &pstConstruct[PieceType::KNIGHT].mg.filecenter },
	{ "pst-knight-file-center-eg", &pstConstruct[PieceType::KNIGHT].eg.filecenter },
	{ "pst-knight-rank-center-mg", &pstConstruct[PieceType::KNIGHT].mg.rankcenter },
	{ "pst-knight-rank-center-eg", &pstConstruct[PieceType::KNIGHT].eg.rankcenter },
	{ "pst-knight-center-mg", &pstConstruct[PieceType::KNIGHT].mg.center },
	{ "pst-knight-center-eg", &pstConstruct[PieceType::KNIGHT].eg.center },

	{ "pst-bishop-rank-mg", &pstConstruct[PieceType::BISHOP].mg.rank },
	{ "pst-bishop-rank-eg", &pstConstruct[PieceType::BISHOP].eg.rank },
	{ "pst-bishop-file-center-mg", &pstConstruct[PieceType::BISHOP].mg.filecenter },
	{ "pst-bishop-file-center-eg", &pstConstruct[PieceType::BISHOP].eg.filecenter },
	{ "pst-bishop-rank-center-mg", &pstConstruct[PieceType::BISHOP].mg.rankcenter },
	{ "pst-bishop-rank-center-eg", &pstConstruct[PieceType::BISHOP].eg.rankcenter },
	{ "pst-bishop-center-mg", &pstConstruct[PieceType::BISHOP].mg.center },
	{ "pst-bishop-center-eg", &pstConstruct[PieceType::BISHOP].eg.center },

	{ "pst-rook-rank-mg", &pstConstruct[PieceType::ROOK].mg.rank },
	{ "pst-rook-rank-eg", &pstConstruct[PieceType::ROOK].eg.rank },
	{ "pst-rook-file-center-mg", &pstConstruct[PieceType::ROOK].mg.filecenter },
	{ "pst-rook-file-center-eg", &pstConstruct[PieceType::ROOK].eg.filecenter },
	{ "pst-rook-rank-center-mg", &pstConstruct[PieceType::ROOK].mg.rankcenter },
	{ "pst-rook-rank-center-eg", &pstConstruct[PieceType::ROOK].eg.rankcenter },
	{ "pst-rook-center-mg", &pstConstruct[PieceType::ROOK].mg.center },
	{ "pst-rook-center-eg", &pstConstruct[PieceType::ROOK].eg.center },

	{ "pst-queen-rank-mg", &pstConstruct[PieceType::QUEEN].mg.rank },
	{ "pst-queen-rank-eg", &pstConstruct[PieceType::QUEEN].eg.rank },
	{ "pst-queen-file-center-mg", &pstConstruct[PieceType::QUEEN].mg.filecenter },
	{ "pst-queen-file-center-eg", &pstConstruct[PieceType::QUEEN].eg.filecenter },
	{ "pst-queen-rank-center-mg", &pstConstruct[PieceType::QUEEN].mg.rankcenter },
	{ "pst-queen-rank-center-eg", &pstConstruct[PieceType::QUEEN].eg.rankcenter },
	{ "pst-queen-center-mg", &pstConstruct[PieceType::QUEEN].mg.center },
	{ "pst-queen-center-eg", &pstConstruct[PieceType::QUEEN].eg.center },

	{ "pst-king-rank-mg", &pstConstruct[PieceType::KING].mg.rank },
	{ "pst-king-rank-eg", &pstConstruct[PieceType::KING].eg.rank },
	{ "pst-king-file-center-mg", &pstConstruct[PieceType::KING].mg.filecenter },
	{ "pst-king-file-center-eg", &pstConstruct[PieceType::KING].eg.filecenter },
	{ "pst-king-rank-center-mg", &pstConstruct[PieceType::KING].mg.rankcenter },
	{ "pst-king-rank-center-eg", &pstConstruct[PieceType::KING].eg.rankcenter },
	{ "pst-king-center-mg", &pstConstruct[PieceType::KING].mg.center },
	{ "pst-king-center-eg", &pstConstruct[PieceType::KING].eg.center },

	{ "pawn-chain-back-default-mg", &PawnChainBackDefault.mg },
	{ "pawn-chain-back-default-eg", &PawnChainBackDefault.eg },
	{ "pawn-chain-back-rank-mg", &pawnChainBackPstConstruct.mg.rank },
	{ "pawn-chain-back-rank-eg", &pawnChainBackPstConstruct.eg.rank },
	{ "pawn-chain-back-file-center-mg", &pawnChainBackPstConstruct.mg.filecenter },
	{ "pawn-chain-back-file-center-eg", &pawnChainBackPstConstruct.eg.filecenter },

	{ "pawn-chain-front-default-mg", &PawnChainFrontDefault.mg },
	{ "pawn-chain-front-default-eg", &PawnChainFrontDefault.eg },
	{ "pawn-chain-front-rank-mg", &pawnChainFrontPstConstruct.mg.rank },
	{ "pawn-chain-front-rank-eg", &pawnChainFrontPstConstruct.eg.rank },
	{ "pawn-chain-front-file-center-mg", &pawnChainFrontPstConstruct.mg.filecenter },
	{ "pawn-chain-front-file-center-eg", &pawnChainFrontPstConstruct.eg.filecenter },

	{ "pawn-doubled-default-mg", &PawnDoubledDefault.mg },
	{ "pawn-doubled-default-eg", &PawnDoubledDefault.eg },
	{ "pawn-doubled-rank-mg", &pawnDoubledPstConstruct.mg.rank },
	{ "pawn-doubled-rank-eg", &pawnDoubledPstConstruct.eg.rank },
	{ "pawn-doubled-file-center-mg", &pawnDoubledPstConstruct.mg.filecenter },
	{ "pawn-doubled-file-center-eg", &pawnDoubledPstConstruct.eg.filecenter },

	{ "pawn-passed-default-mg", &PawnPassedDefault.mg },
	{ "pawn-passed-default-eg", &PawnPassedDefault.eg },
	{ "pawn-passed-rank-mg", &pawnPassedPstConstruct.mg.rank },
	{ "pawn-passed-rank-eg", &pawnPassedPstConstruct.eg.rank },
	{ "pawn-passed-file-center-mg", &pawnPassedPstConstruct.mg.filecenter },
	{ "pawn-passed-file-center-eg", &pawnPassedPstConstruct.eg.filecenter },

	{ "pawn-tripled-default-mg", &PawnTripledDefault.mg },
	{ "pawn-tripled-default-eg", &PawnTripledDefault.eg },
	{ "pawn-tripled-rank-mg", &pawnTripledPstConstruct.mg.rank },
	{ "pawn-tripled-rank-eg", &pawnTripledPstConstruct.eg.rank },
	{ "pawn-tripled-file-center-mg", &pawnTripledPstConstruct.mg.filecenter },
	{ "pawn-tripled-file-center-eg", &pawnTripledPstConstruct.eg.filecenter },

	{ "mobility-knight-quadratic-mg", &mobilityConstructor[PieceType::KNIGHT].mg.quadratic },
	{ "mobility-knight-quadratic-eg", &mobilityConstructor[PieceType::KNIGHT].eg.quadratic },
	{ "mobility-knight-slope-mg", &mobilityConstructor[PieceType::KNIGHT].mg.slope },
	{ "mobility-knight-slope-eg", &mobilityConstructor[PieceType::KNIGHT].eg.slope },
	{ "mobility-knight-yintercept-mg", &mobilityConstructor[PieceType::KNIGHT].mg.yintercept },
	{ "mobility-knight-yintercept-eg", &mobilityConstructor[PieceType::KNIGHT].eg.yintercept },

	{ "mobility-knight-0-mg", &MobilityParameters[PieceType::KNIGHT][0].mg },
	{ "mobility-knight-0-eg", &MobilityParameters[PieceType::KNIGHT][0].eg },

	{ "mobility-better-knight-quadratic-mg", &betterMobilityConstructor[PieceType::KNIGHT].mg.quadratic },
	{ "mobility-better-knight-quadratic-eg", &betterMobilityConstructor[PieceType::KNIGHT].eg.quadratic },
	{ "mobility-better-knight-slope-mg", &betterMobilityConstructor[PieceType::KNIGHT].mg.slope },
	{ "mobility-better-knight-slope-eg", &betterMobilityConstructor[PieceType::KNIGHT].eg.slope },
	{ "mobility-better-knight-yintercept-mg", &betterMobilityConstructor[PieceType::KNIGHT].mg.yintercept },
	{ "mobility-better-knight-yintercept-eg", &betterMobilityConstructor[PieceType::KNIGHT].eg.yintercept },

	{ "mobility-safe-knight-quadratic-mg", &safeMobilityConstructor[PieceType::KNIGHT].mg.quadratic },
	{ "mobility-safe-knight-quadratic-eg", &safeMobilityConstructor[PieceType::KNIGHT].eg.quadratic },
	{ "mobility-safe-knight-slope-mg", &safeMobilityConstructor[PieceType::KNIGHT].mg.slope },
	{ "mobility-safe-knight-slope-eg", &safeMobilityConstructor[PieceType::KNIGHT].eg.slope },
	{ "mobility-safe-knight-yintercept-mg", &safeMobilityConstructor[PieceType::KNIGHT].mg.yintercept },
	{ "mobility-safe-knight-yintercept-eg", &safeMobilityConstructor[PieceType::KNIGHT].eg.yintercept },

	{ "mobility-bishop-quadratic-mg", &mobilityConstructor[PieceType::BISHOP].mg.quadratic },
	{ "mobility-bishop-quadratic-eg", &mobilityConstructor[PieceType::BISHOP].eg.quadratic },
	{ "mobility-bishop-slope-mg", &mobilityConstructor[PieceType::BISHOP].mg.slope },
	{ "mobility-bishop-slope-eg", &mobilityConstructor[PieceType::BISHOP].eg.slope },
	{ "mobility-bishop-yintercept-mg", &mobilityConstructor[PieceType::BISHOP].mg.yintercept },
	{ "mobility-bishop-yintercept-eg", &mobilityConstructor[PieceType::BISHOP].eg.yintercept },

	{ "mobility-bishop-0-mg", &MobilityParameters[PieceType::BISHOP][0].mg },
	{ "mobility-bishop-0-eg", &MobilityParameters[PieceType::BISHOP][0].eg },

	{ "mobility-better-bishop-quadratic-mg", &betterMobilityConstructor[PieceType::BISHOP].mg.quadratic },
	{ "mobility-better-bishop-quadratic-eg", &betterMobilityConstructor[PieceType::BISHOP].eg.quadratic },
	{ "mobility-better-bishop-slope-mg", &betterMobilityConstructor[PieceType::BISHOP].mg.slope },
	{ "mobility-better-bishop-slope-eg", &betterMobilityConstructor[PieceType::BISHOP].eg.slope },
	{ "mobility-better-bishop-yintercept-mg", &betterMobilityConstructor[PieceType::BISHOP].mg.yintercept },
	{ "mobility-better-bishop-yintercept-eg", &betterMobilityConstructor[PieceType::BISHOP].eg.yintercept },

	{ "mobility-safe-bishop-quadratic-mg", &safeMobilityConstructor[PieceType::BISHOP].mg.quadratic },
	{ "mobility-safe-bishop-quadratic-eg", &safeMobilityConstructor[PieceType::BISHOP].eg.quadratic },
	{ "mobility-safe-bishop-slope-mg", &safeMobilityConstructor[PieceType::BISHOP].mg.slope },
	{ "mobility-safe-bishop-slope-eg", &safeMobilityConstructor[PieceType::BISHOP].eg.slope },
	{ "mobility-safe-bishop-yintercept-mg", &safeMobilityConstructor[PieceType::BISHOP].mg.yintercept },
	{ "mobility-safe-bishop-yintercept-eg", &safeMobilityConstructor[PieceType::BISHOP].eg.yintercept },

	{ "mobility-rook-quadratic-mg", &mobilityConstructor[PieceType::ROOK].mg.quadratic },
	{ "mobility-rook-quadratic-eg", &mobilityConstructor[PieceType::ROOK].eg.quadratic },
	{ "mobility-rook-slope-mg", &mobilityConstructor[PieceType::ROOK].mg.slope },
	{ "mobility-rook-slope-eg", &mobilityConstructor[PieceType::ROOK].eg.slope },
	{ "mobility-rook-yintercept-mg", &mobilityConstructor[PieceType::ROOK].mg.yintercept },
	{ "mobility-rook-yintercept-eg", &mobilityConstructor[PieceType::ROOK].eg.yintercept },

	{ "mobility-rook-0-mg", &MobilityParameters[PieceType::ROOK][0].mg },
	{ "mobility-rook-0-eg", &MobilityParameters[PieceType::ROOK][0].eg },

	{ "mobility-better-rook-quadratic-mg", &betterMobilityConstructor[PieceType::ROOK].mg.quadratic },
	{ "mobility-better-rook-quadratic-eg", &betterMobilityConstructor[PieceType::ROOK].eg.quadratic },
	{ "mobility-better-rook-slope-mg", &betterMobilityConstructor[PieceType::ROOK].mg.slope },
	{ "mobility-better-rook-slope-eg", &betterMobilityConstructor[PieceType::ROOK].eg.slope },
	{ "mobility-better-rook-yintercept-mg", &betterMobilityConstructor[PieceType::ROOK].mg.yintercept },
	{ "mobility-better-rook-yintercept-eg", &betterMobilityConstructor[PieceType::ROOK].eg.yintercept },

	{ "mobility-safe-rook-quadratic-mg", &safeMobilityConstructor[PieceType::ROOK].mg.quadratic },
	{ "mobility-safe-rook-quadratic-eg", &safeMobilityConstructor[PieceType::ROOK].eg.quadratic },
	{ "mobility-safe-rook-slope-mg", &safeMobilityConstructor[PieceType::ROOK].mg.slope },
	{ "mobility-safe-rook-slope-eg", &safeMobilityConstructor[PieceType::ROOK].eg.slope },
	{ "mobility-safe-rook-yintercept-mg", &safeMobilityConstructor[PieceType::ROOK].mg.yintercept },
	{ "mobility-safe-rook-yintercept-eg", &safeMobilityConstructor[PieceType::ROOK].eg.yintercept },

	{ "mobility-queen-quadratic-mg", &mobilityConstructor[PieceType::QUEEN].mg.quadratic },
	{ "mobility-queen-quadratic-eg", &mobilityConstructor[PieceType::QUEEN].eg.quadratic },
	{ "mobility-queen-slope-mg", &mobilityConstructor[PieceType::QUEEN].mg.slope },
	{ "mobility-queen-slope-eg", &mobilityConstructor[PieceType::QUEEN].eg.slope },
	{ "mobility-queen-yintercept-mg", &mobilityConstructor[PieceType::QUEEN].mg.yintercept },
	{ "mobility-queen-yintercept-eg", &mobilityConstructor[PieceType::QUEEN].eg.yintercept },

	{ "mobility-queen-0-mg", &MobilityParameters[PieceType::QUEEN][0].mg },
	{ "mobility-queen-0-eg", &MobilityParameters[PieceType::QUEEN][0].eg },
	
	{ "mobility-better-queen-quadratic-mg", &betterMobilityConstructor[PieceType::QUEEN].mg.quadratic },
	{ "mobility-better-queen-quadratic-eg", &betterMobilityConstructor[PieceType::QUEEN].eg.quadratic },
	{ "mobility-better-queen-slope-mg", &betterMobilityConstructor[PieceType::QUEEN].mg.slope },
	{ "mobility-better-queen-slope-eg", &betterMobilityConstructor[PieceType::QUEEN].eg.slope },
	{ "mobility-better-queen-yintercept-mg", &betterMobilityConstructor[PieceType::QUEEN].mg.yintercept },
	{ "mobility-better-queen-yintercept-eg", &betterMobilityConstructor[PieceType::QUEEN].eg.yintercept },

	{ "mobility-safe-queen-quadratic-mg", &safeMobilityConstructor[PieceType::QUEEN].mg.quadratic },
	{ "mobility-safe-queen-quadratic-eg", &safeMobilityConstructor[PieceType::QUEEN].eg.quadratic },
	{ "mobility-safe-queen-slope-mg", &safeMobilityConstructor[PieceType::QUEEN].mg.slope },
	{ "mobility-safe-queen-slope-eg", &safeMobilityConstructor[PieceType::QUEEN].eg.slope },
	{ "mobility-safe-queen-yintercept-mg", &safeMobilityConstructor[PieceType::QUEEN].mg.yintercept },
	{ "mobility-safe-queen-yintercept-eg", &safeMobilityConstructor[PieceType::QUEEN].eg.yintercept },

	{ "attack-pawn-knight-mg", &AttackParameters[PieceType::PAWN][PieceType::KNIGHT].mg },
	{ "attack-pawn-knight-eg", &AttackParameters[PieceType::PAWN][PieceType::KNIGHT].eg },
	{ "attack-pawn-bishop-mg", &AttackParameters[PieceType::PAWN][PieceType::BISHOP].mg },
	{ "attack-pawn-bishop-eg", &AttackParameters[PieceType::PAWN][PieceType::BISHOP].eg },
	{ "attack-pawn-rook-mg", &AttackParameters[PieceType::PAWN][PieceType::ROOK].mg },
	{ "attack-pawn-rook-eg", &AttackParameters[PieceType::PAWN][PieceType::ROOK].eg },
	{ "attack-pawn-queen-mg", &AttackParameters[PieceType::PAWN][PieceType::QUEEN].mg },
	{ "attack-pawn-queen-eg", &AttackParameters[PieceType::PAWN][PieceType::QUEEN].eg },

	{ "attack-knight-pawn-mg", &AttackParameters[PieceType::KNIGHT][PieceType::PAWN].mg },
	{ "attack-knight-pawn-eg", &AttackParameters[PieceType::KNIGHT][PieceType::PAWN].eg },
	{ "attack-knight-bishop-mg", &AttackParameters[PieceType::KNIGHT][PieceType::BISHOP].mg },
	{ "attack-knight-bishop-eg", &AttackParameters[PieceType::KNIGHT][PieceType::BISHOP].eg },
	{ "attack-knight-rook-mg", &AttackParameters[PieceType::KNIGHT][PieceType::ROOK].mg },
	{ "attack-knight-rook-eg", &AttackParameters[PieceType::KNIGHT][PieceType::ROOK].eg },
	{ "attack-knight-queen-mg", &AttackParameters[PieceType::KNIGHT][PieceType::QUEEN].mg },
	{ "attack-knight-queen-eg", &AttackParameters[PieceType::KNIGHT][PieceType::QUEEN].eg },

	{ "attack-bishop-pawn-mg", &AttackParameters[PieceType::BISHOP][PieceType::PAWN].mg },
	{ "attack-bishop-pawn-eg", &AttackParameters[PieceType::BISHOP][PieceType::PAWN].eg },
	{ "attack-bishop-knight-mg", &AttackParameters[PieceType::BISHOP][PieceType::KNIGHT].mg },
	{ "attack-bishop-knight-eg", &AttackParameters[PieceType::BISHOP][PieceType::KNIGHT].eg },
	{ "attack-bishop-rook-mg", &AttackParameters[PieceType::BISHOP][PieceType::ROOK].mg },
	{ "attack-bishop-rook-eg", &AttackParameters[PieceType::BISHOP][PieceType::ROOK].eg },
	{ "attack-bishop-queen-mg", &AttackParameters[PieceType::BISHOP][PieceType::QUEEN].mg },
	{ "attack-bishop-queen-eg", &AttackParameters[PieceType::BISHOP][PieceType::QUEEN].eg },

	{ "attack-rook-pawn-mg", &AttackParameters[PieceType::ROOK][PieceType::PAWN].mg },
	{ "attack-rook-pawn-eg", &AttackParameters[PieceType::ROOK][PieceType::PAWN].eg },
	{ "attack-rook-knight-mg", &AttackParameters[PieceType::ROOK][PieceType::KNIGHT].mg },
	{ "attack-rook-knight-eg", &AttackParameters[PieceType::ROOK][PieceType::KNIGHT].eg },
	{ "attack-rook-bishop-mg", &AttackParameters[PieceType::ROOK][PieceType::BISHOP].mg },
	{ "attack-rook-bishop-eg", &AttackParameters[PieceType::ROOK][PieceType::BISHOP].eg },
	{ "attack-rook-queen-mg", &AttackParameters[PieceType::ROOK][PieceType::QUEEN].mg },
	{ "attack-rook-queen-eg", &AttackParameters[PieceType::ROOK][PieceType::QUEEN].eg },

	{ "attack-queen-pawn-mg", &AttackParameters[PieceType::QUEEN][PieceType::PAWN].mg },
	{ "attack-queen-pawn-eg", &AttackParameters[PieceType::QUEEN][PieceType::PAWN].eg },
	{ "attack-queen-knight-mg", &AttackParameters[PieceType::QUEEN][PieceType::KNIGHT].mg },
	{ "attack-queen-knight-eg", &AttackParameters[PieceType::QUEEN][PieceType::KNIGHT].eg },
	{ "attack-queen-bishop-mg", &AttackParameters[PieceType::QUEEN][PieceType::BISHOP].mg },
	{ "attack-queen-bishop-eg", &AttackParameters[PieceType::QUEEN][PieceType::BISHOP].eg },
	{ "attack-queen-rook-mg", &AttackParameters[PieceType::QUEEN][PieceType::ROOK].mg },
	{ "attack-queen-rook-eg", &AttackParameters[PieceType::QUEEN][PieceType::ROOK].eg },

	{ "search-reductions-current-depth-mg", &LateMoveReductions[0].mg },
	{ "search-reductions-depth-left-mg", &LateMoveReductions[1].mg },
	{ "search-reductions-searched-moves-mg", &LateMoveReductions[2].mg },
	{ "search-reductions-all-mg", &LateMoveReductions[3].mg },

	{ "tropism-knight-quadratic-mg", &tropismConstructor[PieceType::KNIGHT].mg.quadratic },
	{ "tropism-knight-quadratic-eg", &tropismConstructor[PieceType::KNIGHT].eg.quadratic },
	{ "tropism-knight-slope-mg", &tropismConstructor[PieceType::KNIGHT].mg.slope },
	{ "tropism-knight-slope-eg", &tropismConstructor[PieceType::KNIGHT].eg.slope },
	{ "tropism-knight-yintercept-mg", &tropismConstructor[PieceType::KNIGHT].mg.yintercept },
	{ "tropism-knight-yintercept-eg", &tropismConstructor[PieceType::KNIGHT].eg.yintercept },

	{ "tropism-bishop-quadratic-mg", &tropismConstructor[PieceType::BISHOP].mg.quadratic },
	{ "tropism-bishop-quadratic-eg", &tropismConstructor[PieceType::BISHOP].eg.quadratic },
	{ "tropism-bishop-slope-mg", &tropismConstructor[PieceType::BISHOP].mg.slope },
	{ "tropism-bishop-slope-eg", &tropismConstructor[PieceType::BISHOP].eg.slope },
	{ "tropism-bishop-yintercept-mg", &tropismConstructor[PieceType::BISHOP].mg.yintercept },
	{ "tropism-bishop-yintercept-eg", &tropismConstructor[PieceType::BISHOP].eg.yintercept },

	{ "tropism-rook-quadratic-mg", &tropismConstructor[PieceType::ROOK].mg.quadratic },
	{ "tropism-rook-quadratic-eg", &tropismConstructor[PieceType::ROOK].eg.quadratic },
	{ "tropism-rook-slope-mg", &tropismConstructor[PieceType::ROOK].mg.slope },
	{ "tropism-rook-slope-eg", &tropismConstructor[PieceType::ROOK].eg.slope },
	{ "tropism-rook-yintercept-mg", &tropismConstructor[PieceType::ROOK].mg.yintercept },
	{ "tropism-rook-yintercept-eg", &tropismConstructor[PieceType::ROOK].eg.yintercept },

	{ "tropism-queen-quadratic-mg", &tropismConstructor[PieceType::QUEEN].mg.quadratic },
	{ "tropism-queen-quadratic-eg", &tropismConstructor[PieceType::QUEEN].eg.quadratic },
	{ "tropism-queen-slope-mg", &tropismConstructor[PieceType::QUEEN].mg.slope },
	{ "tropism-queen-slope-eg", &tropismConstructor[PieceType::QUEEN].eg.slope },
	{ "tropism-queen-yintercept-mg", &tropismConstructor[PieceType::QUEEN].mg.yintercept },
	{ "tropism-queen-yintercept-eg", &tropismConstructor[PieceType::QUEEN].eg.yintercept },

	{ "pst-control-rank-mg", &boardControlPstConstruct.mg.rank },
	{ "pst-control-rank-eg", &boardControlPstConstruct.eg.rank },
	{ "pst-control-file-center-mg", &boardControlPstConstruct.mg.filecenter },
	{ "pst-control-file-center-eg", &boardControlPstConstruct.eg.filecenter },
	{ "pst-control-rank-center-mg", &boardControlPstConstruct.mg.rankcenter },
	{ "pst-control-rank-center-eg", &boardControlPstConstruct.eg.rankcenter },
	{ "pst-control-center-mg", &boardControlPstConstruct.mg.center },
	{ "pst-control-center-eg", &boardControlPstConstruct.eg.center },

	{ "pst-king-control-rank-mg", &kingControlPstConstruct.mg.rank },
	{ "pst-king-control-rank-eg", &kingControlPstConstruct.eg.rank },
	{ "pst-king-control-file-center-mg", &kingControlPstConstruct.mg.filecenter },
	{ "pst-king-control-file-center-eg", &kingControlPstConstruct.eg.filecenter },
	{ "pst-king-control-rank-center-mg", &kingControlPstConstruct.mg.rankcenter },
	{ "pst-king-control-rank-center-eg", &kingControlPstConstruct.eg.rankcenter },
	{ "pst-king-control-center-mg", &kingControlPstConstruct.mg.center },
	{ "pst-king-control-center-eg", &kingControlPstConstruct.eg.center },

	{ "doubled-rooks-mg", &DoubledRooks.mg },
	{ "doubled-rooks-eg", &DoubledRooks.eg },

	{ "empty-file-queen-mg", &EmptyFileQueen.mg },
	{ "empty-file-queen-eg", &EmptyFileQueen.eg },

	{ "empty-file-rook-mg", &EmptyFileRook.mg },
	{ "empty-file-rook-eg", &EmptyFileRook.eg },

	{ "good-bishop-pawns-quadratic-mg", &goodBishopPawnConstructor.mg.quadratic },
	{ "good-bishop-pawns-quadratic-eg", &goodBishopPawnConstructor.eg.quadratic },
	{ "good-bishop-pawns-slope-mg", &goodBishopPawnConstructor.mg.slope },
	{ "good-bishop-pawns-slope-eg", &goodBishopPawnConstructor.eg.slope },
	{ "good-bishop-pawns-yintercept-mg", &goodBishopPawnConstructor.mg.yintercept },
	{ "good-bishop-pawns-yintercept-eg", &goodBishopPawnConstructor.eg.yintercept },

	{ "queen-behind-passed-pawn-default-mg", &queenBehindPassedPawnDefault.mg },
	{ "queen-behind-passed-pawn-default-eg", &queenBehindPassedPawnDefault.eg },
	{ "queen-behind-passed-pawn-rank-mg", &queenBehindPassedPawnPstConstruct.mg.rank },
	{ "queen-behind-passed-pawn-rank-eg", &queenBehindPassedPawnPstConstruct.eg.rank },
	{ "queen-behind-passed-pawn-file-center-mg", &queenBehindPassedPawnPstConstruct.mg.filecenter },
	{ "queen-behind-passed-pawn-file-center-eg", &queenBehindPassedPawnPstConstruct.eg.filecenter },

	{ "rook-behind-passed-pawn-default-mg", &rookBehindPassedPawnDefault.mg },
	{ "rook-behind-passed-pawn-default-eg", &rookBehindPassedPawnDefault.eg },
	{ "rook-behind-passed-pawn-rank-mg", &rookBehindPassedPawnPstConstruct.mg.rank },
	{ "rook-behind-passed-pawn-rank-eg", &rookBehindPassedPawnPstConstruct.eg.rank },
	{ "rook-behind-passed-pawn-file-center-mg", &rookBehindPassedPawnPstConstruct.mg.filecenter },
	{ "rook-behind-passed-pawn-file-center-eg", &rookBehindPassedPawnPstConstruct.eg.filecenter },
};

void InitializeParameters()
{
	for (PieceType pieceType = PieceType::PAWN; pieceType < PieceType::PIECETYPE_COUNT; pieceType++) {
		scoreConstructor.construct(pstConstruct[pieceType], PstParameters[pieceType]);

		scoreConstructor.construct(betterMobilityConstructor[pieceType], &(BetterMobilityParameters[pieceType][0]), 32);
		scoreConstructor.construct(mobilityConstructor[pieceType], &(MobilityParameters[pieceType][0]), 32);
		scoreConstructor.construct(safeMobilityConstructor[pieceType], &(SafeMobilityParameters[pieceType][0]), 32);

		scoreConstructor.construct(tropismConstructor[pieceType], &(TropismParameters[pieceType][0]), 16);
	}

	scoreConstructor.construct(boardControlPstConstruct, BoardControlPstParameters);
	scoreConstructor.construct(kingControlPstConstruct, KingControlPstParameters);

	scoreConstructor.construct(goodBishopPawnConstructor, &GoodBishopPawns[0], 8);
	scoreConstructor.construct(queenBehindPassedPawnPstConstruct, QueenBehindPassedPawnPst, queenBehindPassedPawnDefault);
	scoreConstructor.construct(rookBehindPassedPawnPstConstruct, RookBehindPassedPawnPst, rookBehindPassedPawnDefault);

	scoreConstructor.construct(pawnChainBackPstConstruct, PawnChainBackPstParameters, PawnChainBackDefault);
	scoreConstructor.construct(pawnChainFrontPstConstruct, PawnChainFrontPstParameters, PawnChainFrontDefault);
	scoreConstructor.construct(pawnDoubledPstConstruct, PawnDoubledPstParameters, PawnDoubledDefault);
	scoreConstructor.construct(pawnPassedPstConstruct, PawnPassedPstParameters, PawnPassedDefault);
	scoreConstructor.construct(pawnTripledPstConstruct, PawnTripledPstParameters, PawnTripledDefault);

	for (Square src = Square::FIRST_SQUARE; src < Square::SQUARE_COUNT; src++) {
		File file = getFile(src);
		Rank rank = getRank(src);

		Distance[file][rank] = std::uint32_t(std::sqrt(file * file + rank * rank));
	}
}
