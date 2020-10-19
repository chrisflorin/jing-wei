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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "xboard.h"

#include "../board/movegen.h"

#include "../../game/types/depth.h"
#include "../../game/types/nodecount.h"

static ChessMoveGenerator moveGenerator;

struct Command {
    std::string command;
    void (*function)(XBoardComm* xboard, std::stringstream& cmd);
};

static void xboardForce(XBoardComm* xboard, std::stringstream& cmd)
{
    xboard->setForce(true);
}

static void xboardGo(XBoardComm* xboard, std::stringstream& cmd)
{
    ChessMove playerMove;
    ChessPrincipalVariation principalVariation;

    xboard->getPlayerMove(playerMove);

    xboard->doPlayerMove(playerMove);

    std::cout << "move ";
    principalVariation.printMoveToConsole(playerMove);
    std::cout << std::endl;

    xboard->setForce(false);
}

static void xboardLevel(XBoardComm* xboard, std::stringstream& cmd)
{
    NodeCount moveCount = ZeroNodes;
    std::string tournamentTime;
    std::time_t increment = 0;

    struct tm tm = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    cmd >> moveCount;
    cmd >> tournamentTime;
    cmd >> increment;

    //Arena does not give a colon, and tournamentTime is in minutes
    //Cutechess-cli gives a colon and is in the M:S format
    const char* time = tournamentTime.c_str();
    int i = 0;

    if (time[i] == '/') {
        i++;
    }

    std::time_t minutes = 0, seconds = 0;

    while (isdigit(time[i])) {
        minutes = minutes * 10 + (time[i] - '0');
        i++;
    }

    if (time[i] == ':') {
        i++;

        while (isdigit(time[i])) {
            seconds = seconds * 10 + (time[i] - '0');
            i++;
        }
    }

    seconds = seconds + minutes * 60;

    xboard->getPlayerClock().setClockLevel(moveCount, 1000 * seconds, 1000 * increment);
}

static void xboardNew(XBoardComm* xboard, std::stringstream& cmd)
{
    xboard->resetStartingPosition();
}

static void xboardNps(XBoardComm* xboard, std::stringstream& cmd)
{
    NodeCount nps;
    cmd >> nps;

    xboard->getPlayerClock().setClockNps(nps);
}

static void xboardOtim(XBoardComm* xboard, std::stringstream& cmd)
{
    std::time_t centiseconds;
    cmd >> centiseconds;

    xboard->getPlayerClock().setClockOpponentTimeLeft(centiseconds * 10);
}

static void xboardPerft(XBoardComm* xboard, std::stringstream& cmd)
{
    Clock clock;

    //There's no operator to get a Depth from a std::stringstream
    int depth;
    cmd >> depth;

    NodeCount nodeCount;
    Depth maxDepth = Depth::ONE * depth;

    std::time_t time = 0;

    if (maxDepth == Depth::ZERO) {
        nodeCount = 1;
    }
    else {
        clock.startClock();

        nodeCount = xboard->perft(maxDepth);

        time = clock.getElapsedTime(ZeroNodes);
    }

    std::cout << "Total: " << nodeCount << " Moves" << std::endl;

    NodeCount nps;

    if (time == 0) {
        nps = nodeCount;
    }
    else {
        nps = 1000 * nodeCount / time;
    }

    std::cout << "Time: " << time << " ms (" << nps << " nps)" << std::endl;
}

static void xboardPersonality(XBoardComm* xboard, std::stringstream& cmd)
{
    std::string personalityFileName;
    cmd >> personalityFileName;

    xboard->loadPersonalityFile(personalityFileName);
}

static void xboardPing(XBoardComm* xboard, std::stringstream& cmd)
{
	int ping;
	cmd >> ping;

	std::cout << "pong " << ping << std::endl;
}

static void xboardQuit(XBoardComm* xboard, std::stringstream& cmd)
{
    xboard->finish();
}

static void xboardSd(XBoardComm* xboard, std::stringstream& cmd)
{
    int depth;
    cmd >> depth;

    xboard->getPlayerClock().setClockDepth(Depth::ONE * depth);
}

static void xboardSetBoard(XBoardComm* xboard, std::stringstream& cmd)
{
    std::string setboard = cmd.str();
    setboard = setboard.substr(setboard.find_first_of(' ') + 1);

    xboard->resetSpecificPosition(setboard);
}

static void xboardSetValue(XBoardComm* xboard, std::stringstream& cmd)
{
    std::string name;
    Score score;

    cmd >> name >> score;

    xboard->setParameter(name, score);
}

static void xboardSn(XBoardComm* xboard, std::stringstream& cmd)
{
    NodeCount nodes;
    cmd >> nodes;

    xboard->getPlayerClock().setClockNodes(nodes);
}

static void xboardSt(XBoardComm* xboard, std::stringstream& cmd)
{
    std::time_t seconds;
    cmd >> seconds;

    xboard->getPlayerClock().setClockSearchTime(seconds * 1000);
}

static void xboardTime(XBoardComm* xboard, std::stringstream& cmd)
{
    std::time_t centiseconds;
    cmd >> centiseconds;

    xboard->getPlayerClock().setClockEngineTimeLeft(centiseconds * 10);
}

static void xboardUndo(XBoardComm* xboard, std::stringstream& cmd)
{
    xboard->undoPlayerMove();
}

static void xboardUserMove(XBoardComm* xboard, std::stringstream& cmd)
{
    std::string moveString;
    cmd >> moveString;

    ChessMove move;
    ChessPrincipalVariation principalVariation;

    principalVariation.stringToMove(moveString, move);

    //Totally don't verify the move coming in from the interface
    xboard->doPlayerMove(move);

    if (xboard->isForced()) {
        return;
    }

    xboardGo(xboard, cmd);
}

static void xboardXboard(XBoardComm* xboard, std::stringstream& cmd)
{
    std::cout << "feature setboard=1 usermove=1 time=1 analyze=0 myname=\"Jing Wei\" name=1 nps=1 done=1\n";
}

static struct Command XBoardCommandList[] =
{
    { "force", xboardForce },
    { "go", xboardGo },
    { "level", xboardLevel },
    { "new", xboardNew },
    { "nps", xboardNps },
    { "otim", xboardOtim },
    { "perft", xboardPerft },
    { "personality", xboardPersonality },
    { "ping", xboardPing },
    { "quit", xboardQuit },
    { "sd", xboardSd },
    { "setboard", xboardSetBoard },
    { "setvalue", xboardSetValue },
    { "sn", xboardSn },
    { "st", xboardSt },
    { "time", xboardTime },
    { "undo", xboardUndo },
    { "usermove", xboardUserMove },
    { "xboard", xboardXboard },
    { "", nullptr }
};

XBoardComm::XBoardComm()
{
    this->force = false;
}

XBoardComm::~XBoardComm()
{

}

void XBoardComm::doPlayerMove(ChessMove& playerMove)
{
    this->player.doMove(playerMove);
}

Clock& XBoardComm::getPlayerClock()
{
    return this->player.getClock();
}

void XBoardComm::getPlayerMove(ChessMove& playerMove)
{
    this->player.getMove(playerMove);
}

bool XBoardComm::isForced()
{
    return this->force;
}

void XBoardComm::loadPersonalityFile(std::string& personalityFileName)
{
    std::fstream personalityFile;

    personalityFile.open(personalityFileName, std::fstream::ios_base::in);

    if (!personalityFile.is_open()) {
        return;
    }

    while (true) {
        std::string parameterName;
        Score parameterScore;

        personalityFile >> parameterName >> parameterScore;

        if (personalityFile.eof()) {
            break;
        }

        this->setParameter(parameterName, parameterScore);
    }

    personalityFile.close();
}

NodeCount XBoardComm::perft(Depth depth)
{
    return this->player.perft(depth);
}

void XBoardComm::processCommandImplementation(std::string& cmd)
{
	struct Command* c = XBoardCommandList;

	std::string command;

	std::stringstream ss(cmd);
	ss >> command;

	while (c->function != nullptr) {
		if (command == c->command) {
			(*c->function)(this, ss);

			return;
		}

		c++;
	}

	std::cout << "Unknown Command: " << command << std::endl;
}

void XBoardComm::resetSpecificPosition(std::string& fen)
{
    this->player.resetSpecificPosition(fen);
}

void XBoardComm::resetStartingPosition()
{
    this->player.resetStartingPosition();
}

void XBoardComm::setForce(bool force)
{
    this->force = force;
}

void XBoardComm::setParameter(std::string& name, Score score)
{
    this->player.setParameter(name, score);
}

void XBoardComm::undoPlayerMove()
{
    this->player.undoMove();
}
