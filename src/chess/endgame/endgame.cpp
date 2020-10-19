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

#include <cassert>
#include <cmath>

#include <map>

#include "../hash/hash.h"

#include "endgame.h"
#include "function.h"

#include "eval/kk.h"
#include "eval/kxk.h"
#include "eval/kxkx.h"
#include "eval/kqxkx.h"
#include "eval/krxkr.h"
#include "eval/kxxk.h"

static std::map<std::string, ChessEndgame::EndgameFunctionType> j = {
    { "K7/8/8/8/8/8/8/7k w - - 0 1", kk },
    { "k7/8/8/8/8/8/8/7K w - - 0 1", kk },

    { "KN6/8/8/8/8/8/8/7k w - - 0 1", knk },
    { "kn6/8/8/8/8/8/8/7K w - - 0 1", knk },
    { "KB6/8/8/8/8/8/8/7k w - - 0 1", kbk },
    { "kb6/8/8/8/8/8/8/7K w - - 0 1", kbk },
    { "KR6/8/8/8/8/8/8/7k w - - 0 1", krk },
    { "kr6/8/8/8/8/8/8/7K w - - 0 1", krk },
    { "KQ6/8/8/8/8/8/8/7k w - - 0 1", kqk },
    { "kq6/8/8/8/8/8/8/7K w - - 0 1", kqk },

    { "KN6/8/8/8/8/8/8/6pk w - - 0 1", knkp },
    { "kn6/8/8/8/8/8/8/6PK w - - 0 1", knkp },

    { "KN6/8/8/8/8/8/8/6nk w - - 0 1", knkn },
//    { "kn6/8/8/8/8/8/8/6NK w - - 0 1", knkn },

    { "KB6/8/8/8/8/8/8/6pk w - - 0 1", kbkp },
    { "kb6/8/8/8/8/8/8/6PK w - - 0 1", kbkp },
    { "KB6/8/8/8/8/8/8/6nk w - - 0 1", kbkn },
    { "kb6/8/8/8/8/8/8/6NK w - - 0 1", kbkn },
    { "KB6/8/8/8/8/8/8/6bk w - - 0 1", kbkb },
//    { "kb6/8/8/8/8/8/8/6BK w - - 0 1", kbkb },

    { "KR6/8/8/8/8/8/8/6pk w - - 0 1", krkp },
    { "kr6/8/8/8/8/8/8/6PK w - - 0 1", krkp },
    { "KR6/8/8/8/8/8/8/6nk w - - 0 1", krkn },
    { "kr6/8/8/8/8/8/8/6NK w - - 0 1", krkn },
    { "KR6/8/8/8/8/8/8/6bk w - - 0 1", krkb },
    { "kr6/8/8/8/8/8/8/6BK w - - 0 1", krkb },
    { "KR6/8/8/8/8/8/8/6rk w - - 0 1", krkr },
//    { "kr6/8/8/8/8/8/8/6RK w - - 0 1", krkr },

    { "KRP5/8/8/8/8/8/8/7k w - - 0 1", krpk },
    { "krp5/8/8/8/8/8/8/7K w - - 0 1", krpk },
    { "KRN5/8/8/8/8/8/8/7k w - - 0 1", krnk },
    { "krn5/8/8/8/8/8/8/7K w - - 0 1", krnk },
    { "KRB5/8/8/8/8/8/8/7k w - - 0 1", krbk },
    { "krb5/8/8/8/8/8/8/7K w - - 0 1", krbk },
    { "KRR5/8/8/8/8/8/8/7k w - - 0 1", krrk },
    { "krr5/8/8/8/8/8/8/7K w - - 0 1", krrk },

    { "KQ6/8/8/8/8/8/8/6pk w - - 0 1", kqkp },
    { "kq6/8/8/8/8/8/8/6PK w - - 0 1", kqkp },
    { "KQ6/8/8/8/8/8/8/6nk w - - 0 1", kqkn },
    { "kq6/8/8/8/8/8/8/6NK w - - 0 1", kqkn },
    { "KQ6/8/8/8/8/8/8/6bk w - - 0 1", kqkb },
    { "kq6/8/8/8/8/8/8/6BK w - - 0 1", kqkb },
    { "KQ6/8/8/8/8/8/8/6rk w - - 0 1", kqkr },
    { "kq6/8/8/8/8/8/8/6RK w - - 0 1", kqkr },
    { "KQ6/8/8/8/8/8/8/6qk w - - 0 1", kqkq },
//    { "kq6/8/8/8/8/8/8/6QK w - - 0 1", kqkq },

    { "KQP5/8/8/8/8/8/8/7k w - - 0 1", kqpk },
    { "kqp5/8/8/8/8/8/8/7K w - - 0 1", kqpk },
    { "KQN5/8/8/8/8/8/8/7k w - - 0 1", kqnk },
    { "kqn5/8/8/8/8/8/8/7K w - - 0 1", kqnk },
    { "KQB5/8/8/8/8/8/8/7k w - - 0 1", kqbk },
    { "kqb5/8/8/8/8/8/8/7K w - - 0 1", kqbk },
    { "KQR5/8/8/8/8/8/8/7k w - - 0 1", kqrk },
    { "kqr5/8/8/8/8/8/8/7K w - - 0 1", kqrk },
    { "KQQ5/8/8/8/8/8/8/7k w - - 0 1", kqqk },
    { "kqq5/8/8/8/8/8/8/7K w - - 0 1", kqqk },

    { "KRP5/8/8/8/8/8/8/6rk w - - 0 1", krpkr },
    { "krp5/8/8/8/8/8/8/6RK w - - 0 1", krpkr },
    { "KRB5/8/8/8/8/8/8/6rk w - - 0 1", krnkb },
    { "krb5/8/8/8/8/8/8/6RK w - - 0 1", krnkb },
    { "KRN5/8/8/8/8/8/8/6rk w - - 0 1", krnkr },
    { "krn5/8/8/8/8/8/8/6RK w - - 0 1", krnkr },
    { "KRB5/8/8/8/8/8/8/6nk w - - 0 1", krbkn },
    { "krb5/8/8/8/8/8/8/6NK w - - 0 1", krbkn },
    { "KRB5/8/8/8/8/8/8/6bk w - - 0 1", krbkb },
    { "krb5/8/8/8/8/8/8/6BK w - - 0 1", krbkb },
    { "KRB5/8/8/8/8/8/8/6rk w - - 0 1", krbkr },
    { "krb5/8/8/8/8/8/8/6RK w - - 0 1", krbkr },

    { "KQP5/8/8/8/8/8/8/6qk w - - 0 1", kqpkq },
    { "kqp5/8/8/8/8/8/8/6QK w - - 0 1", kqpkq },
    { "KQN5/8/8/8/8/8/8/6qk w - - 0 1", kqnkq },
    { "kqn5/8/8/8/8/8/8/6QK w - - 0 1", kqnkq },
};

void InitializeEndgame(ChessEndgame& endgame)
{
    InitializeHashValues();

    ChessBoard board;

    std::map<std::string, ChessEndgame::EndgameFunctionType>::iterator it;
    for (it = j.begin(); it != j.end(); ++it) {
        board.initFromFen(it->first);
        endgame.add(board, it->second);
    }
}
