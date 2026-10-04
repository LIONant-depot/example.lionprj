#pragma once

// The goals: the physics tells the game when something enters a sensor (xlioncore::physics::sensor_begin_event). The sensor of a goal is a trigger box in its mouth
// (the entity has a SoccerGoal); when the ball enters it during play, the team that does not defend that goal scores.
#include "soccer_components.h"

namespace soccer
{
    struct goal_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::global_event<xlioncore::physics::sensor_begin_event>{ .m_pName = "Soccer Goal" };

        void OnEvent(const xlioncore::physics::sensor_touch& Touch) noexcept
        {
            if (!hasComponents<goal>(Touch.m_Sensor) || !hasComponents<ball>(Touch.m_Visitor)) return;      // a sensor that is not a goal, or something other than the ball in it

            team Defender = team::BLUE;
            (void)findEntity(Touch.m_Sensor, [&](const goal& Goal) noexcept { Defender = Goal.m_Defender; });

            xecs::query::instance Query;
            Query.m_Must.AddFromComponents<match>();
            Foreach(Search(Query), [&](const xecs::component::entity&, match& Rules) noexcept
            {
                if (Rules.m_Phase != phase::PLAYING) return;                            // (in the net the ball just stays in the net)
                Rules.m_Phase      = phase::GOAL;
                Rules.m_Timer      = Rules.m_GoalPause;
                Rules.m_LastScorer = Opponent(Defender);
                (Rules.m_LastScorer == team::BLUE ? Rules.m_ScoreBlue : Rules.m_ScoreRed) += 1;
            });
        }
    };
    XSCRIPT_REGISTER_SYSTEM(goal_system)
}
