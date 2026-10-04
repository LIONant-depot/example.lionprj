// The Soccer script module: one translation unit that pulls in the components and the systems (every piece registers itself, see
// xscript_registration.h, so nothing else has to be listed anywhere).
//
// The systems run in the order they are included here: the scoreboard, the referee, the players and last the ball and the
// rules. Each one sees what the others did a frame ago, which at these speeds nobody can tell.
#include "soccer_components.h"

#include "soccer_scoreboard_system.h"
#include "soccer_referee_system.h"
#include "soccer_player_system.h"
#include "soccer_ball_system.h"
#include "soccer_goal_system.h"



