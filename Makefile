CHESS_BOARD = "src/chess/board/attack.cpp" "src/chess/board/board.cpp" "src/chess/board/movegen.cpp" "src/chess/board/moves.cpp"

CHESS_COMM = "src/chess/comm/xboard.cpp"

CHESS_ENDGAME = "src/chess/endgame/endgame.cpp"

CHESS_EVAL = "src/chess/eval/constructor.cpp" "src/chess/eval/evaluator.cpp" "src/chess/eval/parameters.cpp" "src/chess/eval/pawnevaluator.cpp"

CHESS_HASH = "src/chess/hash/hash.cpp"

CHESS_PLAYER = "src/chess/player/player.cpp"

CHESS_SEARCH = "src/chess/search/chesspv.cpp" "src/chess/search/movehistory.cpp" "src/chess/search/searcher.cpp"

CHESS_TYPES = "src/chess/types/square.cpp"

GAME_CLOCK = "src/game/clock/clock.cpp"

GAME_PERSONALITY = "src/game/personality/personality.cpp"

GAME_SEARCH = "src/game/search/hashtable.cpp"

ENGINE = "jing-wei/engine.cpp"

ENGINE_FILES = $(ENGINE) $(CHESS_BOARD) $(CHESS_COMM) $(CHESS_ENDGAME) $(CHESS_EVAL) $(CHESS_HASH) $(CHESS_PLAYER) $(CHESS_SEARCH) $(CHESS_TYPES) $(GAME_CLOCK) $(GAME_PERSONALITY) $(GAME_SEARCH)

build:
	if [ ! -d "bin" ]; then mkdir bin; fi
	
	g++ -o bin/jing-wei $(ENGINE_FILES) -std=c++17 -DUSE_M128I -DNDEBUG -O3 -m64 -mbmi2 -mpopcnt -msse4.2
