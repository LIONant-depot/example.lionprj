#pragma once

// The rules of the match.
//
// The ball itself belongs to the physics (a dynamic body with a sphere collider: it rolls, bounces off the lines' walls, the posts and the
// net). What is left for this system is the clock of the match - Kickoff (everybody takes their places) -> Playing -> Goal (a short
// celebration) -> a new Kickoff - spotting the goal, and putting the ball back on the center spot.
//
// The clock of the match runs with the real Dt: two seconds are two seconds, however fast the frames come.
#include "soccer_components.h"

namespace soccer
{
    struct ball_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Ball" };
        using query = std::tuple<xecs::query::must<xlioncore::transform, ball>>;

        ball_system(xecs::game_mgr::instance& GameMgr) noexcept : xecs::system::instance(GameMgr), m_Mgr(GameMgr) {}

        xecs::game_mgr::instance& m_Mgr;                                   // the physics call below needs the world
        frame_clock               m_Clock;

        void OnUpdate(void) noexcept
        {
            const float Dt = m_Clock.Tick();

            //
            // The rules (copied out: nothing is held while the others are visited)
            //
            match Rules;
            bool  bHaveRules = false;
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<match>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, const match& M) noexcept { Rules = M; bHaveRules = true; });
            }
            if (!bHaveRules) return;

            //
            // The clock of the match
            //
            bool bNewRound = false;
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
                bNewRound = true;
            }

            //
            // The ball: a goal is the ball past the goal line, between the posts
            //
            bool bScored = false;
            team Scorer  = team::BLUE;
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, ball, xlioncore::physics::physics_dynamics>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity& Entity, xlioncore::transform& T, ball& B, const xlioncore::physics::physics_dynamics& Body) noexcept
                {
                    B.m_Velocity = Body.m_LinearVelocity;                  // so the Inspector (and the players) can see it

                    if (bNewRound)                                         // back to the center spot, standing still
                    {
                        xlioncore::physics::TeleportDynamicBody(m_Mgr, Entity, xmath::fvec3(0.0f, T.m_Position.m_Y, 0.0f), xmath::fquat::fromIdentity());
                        return;
                    }

                    if (Rules.m_Phase != phase::PLAYING) return;           // (in the net the ball just stays in the net)

                    const float Mouth = Rules.m_GoalHalfWidth;
                    if (std::fabs(T.m_Position.m_Z) < Mouth)
                    {
                        if (T.m_Position.m_X >  Rules.m_HalfLength) { bScored = true; Scorer = team::BLUE; }
                        if (T.m_Position.m_X < -Rules.m_HalfLength) { bScored = true; Scorer = team::RED;  }
                    }
                });
            }

            if (bScored)
            {
                Rules.m_Phase      = phase::GOAL;
                Rules.m_Timer      = Rules.m_GoalPause;
                Rules.m_LastScorer = Scorer;
                (Scorer == team::BLUE ? Rules.m_ScoreBlue : Rules.m_ScoreRed) += 1;
            }

            //
            // The rules go back
            //
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<match>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, match& M) noexcept { M = Rules; });
            }
        }
    };
    XSCRIPT_REGISTER_SYSTEM(ball_system)
}
