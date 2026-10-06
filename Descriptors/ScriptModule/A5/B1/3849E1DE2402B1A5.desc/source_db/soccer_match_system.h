#pragma once

// The match: its clock. Kickoff (everybody takes their places) -> Playing -> Goal (a short celebration) -> a new Kickoff, and the number of the round, which goes up at every kickoff: the
// others (the players, the ball) see it change and go back to their places. The scores are the goal system's (soccer_goal_system.h): it is the physics that tells that the ball is in a goal.
//
// The clock of the match runs with the game's Dt (scaled by the speed slider): two seconds of game time are two seconds, however fast the frames come. The whole query of this system is
// the function below: the match component, the one entity that holds the rules.
#include "soccer_components.h"

namespace soccer
{
    struct match_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Match" };

        void operator()(match& Rules) noexcept
        {
            const auto* pGame = xlioncore::game::From(getGameMgr());
            if (!pGame) return;
            const float Dt = pGame->m_Time.m_DeltaTime;                 // game time: the speed slider slows or speeds up the match

            if (Rules.m_Phase != phase::PLAYING) Rules.m_Timer -= Dt;

            if (Rules.m_Phase == phase::KICKOFF && Rules.m_Timer <= 0.0f)
            {
                Rules.m_Phase = phase::PLAYING;
                Rules.m_Timer = 0.0f;
            }
            else if (Rules.m_Phase == phase::GOAL && Rules.m_Timer <= 0.0f)
            {
                Rules.m_Phase = phase::KICKOFF;
                Rules.m_Timer = Rules.m_KickoffDelay;
                ++Rules.m_Round;
            }
        }
    };
    XSCRIPT_REGISTER_SYSTEM(match_system)
}
