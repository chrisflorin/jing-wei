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

static ChessEndgame::EndgameFunctionType kqpkq = weakKingEndgameFunction;

static ChessEndgame::EndgameFunctionType kqnkq = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType kqbkq = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType kqrkq = weakKingEndgameFunction;

static ChessEndgame::EndgameFunctionType kqqkn = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType kqqkb = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType kqqkr = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType kqqkq = weakKingEndgameFunction;

static ChessEndgame::EndgameFunctionType kqknn = weakKingEndgameFunction;

static ChessEndgame::EndgameFunctionType kqkbn = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType kqkbb = weakKingEndgameFunction;

static ChessEndgame::EndgameFunctionType kqkrn = weakKingEndgameFunction;
static ChessEndgame::EndgameFunctionType kqkrb = weakKingEndgameFunction;
