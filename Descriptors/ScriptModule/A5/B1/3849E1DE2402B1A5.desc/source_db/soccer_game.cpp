// The Soccer script module: one translation unit that pulls in the components and the systems (every piece registers itself, see
// xscript_registration.h, so nothing else has to be listed anywhere).
//
// Where and when each system runs is data, not the order of these includes: the System Registry of the editor places each one (at the top level of the frame, or in a connector of
// the physics: the players and the referee need a fixed step, and run right before it). The systems of this game: the match (the clock and the rounds), the goals (the score), the
// scoreboard (the banners), the referee, the players and the ball.
#include "soccer_components.h"

#include "soccer_scoreboard_system.h"
#include "soccer_referee_system.h"
#include "soccer_player_system.h"
#include "soccer_match_system.h"
#include "soccer_ball_system.h"
#include "soccer_goal_system.h"



