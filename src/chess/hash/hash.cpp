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

#include "hash.h"

#include "../../game/math/random.h"

#include "../../game/types/color.h"
#include "../../game/types/hash.h"

#include "../types/castlerights.h"
#include "../types/piece.h"
#include "../types/square.h"

Hash PieceHashValues[Color::COLOR_COUNT][PieceType::PIECETYPE_COUNT][Square::SQUARE_COUNT];
Hash WhiteToMoveHash = EmptyHash;
Hash CastleRightsHashValues[CastleRights::CASTLERIGHTS_COUNT];
Hash EnPassantHashValues[Square::SQUARE_COUNT];

static bool isHashInitialized = false;
constexpr std::uint64_t HashRandomSeed = 0x45a88b3744a0624d;

void InitializeHashValues()
{
    if (isHashInitialized) {
        return;
    }

    PseudoRandomSeed(HashRandomSeed);

    for (Color color = Color::COLOR_START; color < Color::COLOR_COUNT; color++) {
        for (PieceType piece = PieceType::PAWN; piece < PieceType::PIECETYPE_COUNT; piece++) {
            for (Square src = Square::FIRST_SQUARE; src < Square::SQUARE_COUNT; src++) {
                PieceHashValues[color][piece][src] = PseudoRandomValue();
            }
        }
    }

    for (Square src = Square::FIRST_SQUARE; src < Square::SQUARE_COUNT; src++) {
        EnPassantHashValues[src] = PseudoRandomValue();
    }

    for (CastleRights castleRights = CastleRights::CASTLERIGHTS_START; castleRights < CastleRights::CASTLERIGHTS_COUNT; castleRights++) {
        CastleRightsHashValues[castleRights] = PseudoRandomValue();
    }

    WhiteToMoveHash = PseudoRandomValue();

    isHashInitialized = true;
}
