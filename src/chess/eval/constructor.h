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

#include "../../game/types/score.h"

#include "../types/square.h"

struct PstConstruct {
    struct {
        Score rank;
        Score filecenter, rankcenter;
        Score center;
    } mg;

    struct {
        Score rank;
        Score filecenter, rankcenter;
        Score center;
    } eg;
};

struct QuadraticConstruct {
    struct {
        Score quadratic;
        Score slope;
        Score yintercept;
    } mg;
    
    struct {
        Score quadratic;
        Score slope;
        Score yintercept;
    } eg;
};

class ScoreConstructor
{
public:
    ScoreConstructor();
    ~ScoreConstructor();

    void construct(PstConstruct pstConstruct, Evaluation(&pst)[Square::SQUARE_COUNT], Evaluation defaultEvaluation = { ZERO_SCORE, ZERO_SCORE });
    void construct(QuadraticConstruct quadraticConstruct, Evaluation* line, std::int32_t size);
};
