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

#include "../function.h"

static ChessEndgame::EndgameFunctionType krpkr = weakKingDrawishEndgameFunction;

static ChessEndgame::EndgameFunctionType krnkn = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType krnkb = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType krnkr = weakKingEndgameFunction;

static ChessEndgame::EndgameFunctionType krbkn = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType krbkb = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType krbkr = weakKingEndgameFunction;

static ChessEndgame::EndgameFunctionType krrkn = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType krrkb = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType krrkr = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType krrkq = weakKingEndgameFunction;

//static ChessEndgame::EndgameFunctionType krknn = weakKingEndgameFunction;

//static ChessEndgame::EndgameFunctionType krkbn = weakKingEndgameFunction;
//static ChessEndgame::EndgameFunctionType krkbb = weakKingEndgameFunction;
