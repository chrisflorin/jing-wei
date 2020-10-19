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

#include "constructor.h"

ScoreConstructor::ScoreConstructor()
{

}

ScoreConstructor::~ScoreConstructor()
{

}

void ScoreConstructor::construct(PstConstruct pstConstruct, Evaluation(&pst)[Square::SQUARE_COUNT], Evaluation defaultEvaluation)
{
    for (Square src = Square::FIRST_SQUARE; src < Square::SQUARE_COUNT; src++) {
        Rank rank = getRank(src);
        File file = getFile(src);

        Score rankDistance = std::abs(35 - 10 * rank);
        Score fileDistance = std::abs(35 - 10 * file);

        Score centerDistance = rankDistance * rankDistance + fileDistance * fileDistance;

        Evaluation result = defaultEvaluation;

        result.mg += pstConstruct.mg.rank * (~rank - 3) / 4;

        //Here, we subtract so that positive center values try to bring pieces toward the center
        result.mg -= pstConstruct.mg.filecenter * (fileDistance - 20) / 10;
        result.mg -= pstConstruct.mg.rankcenter * (rankDistance - 20) / 10;
        result.mg -= pstConstruct.mg.center * (centerDistance - 1200) / 1850;

        result.eg += pstConstruct.eg.rank * (~rank - 3) / 4;

        //Here, we subtract so that positive center values try to bring pieces toward the center
        result.eg -= pstConstruct.eg.filecenter * (fileDistance - 20) / 10;
        result.eg -= pstConstruct.eg.rankcenter * (rankDistance - 20) / 10;
        result.eg -= pstConstruct.eg.center * (centerDistance - 1200) / 1850;

        pst[src] = result;
    }
}

void ScoreConstructor::construct(QuadraticConstruct quadraticConstruct, Evaluation *line, std::int32_t size)
{
    for (std::int32_t x = 0; x < size; x++) {
        Evaluation result = { ZERO_SCORE, ZERO_SCORE };

        result.mg += quadraticConstruct.mg.quadratic * x * x / 100;
        result.mg += quadraticConstruct.mg.slope * x / 4;
        result.mg += quadraticConstruct.mg.yintercept;

        result.eg += quadraticConstruct.eg.quadratic * x * x / 100;
        result.eg += quadraticConstruct.eg.slope * x / 4;
        result.eg += quadraticConstruct.eg.yintercept;

        line[x] = result;
    }
}
