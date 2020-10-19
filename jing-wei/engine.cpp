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

#include "../src/chess/engine/chessengine.h"

int main(int argc, char** argv)
{
    ChessEngine engine;

    std::cout << "Jing Wei Copyright(C) 2019-2020 Chris Florin" << std::endl;
    std::cout << "This program comes with ABSOLUTELY NO WARRANTY." << std::endl;
    std::cout << "This is free software, and you are welcome to redistribute it" << std::endl;
    std::cout << "under certain conditions" << std::endl;

    engine.start(argc, argv);

    return 0;
}
