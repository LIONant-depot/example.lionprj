#pragma once

// The rules of the match.
//
// The ball itself belongs to the physics (a dynamic body with a sphere collider: it rolls, bounces off the lines' walls, the posts and the
// net). What is left for this system is the clock of the match - Kickoff (everybody takes their places) -> Playing -> Goal (a short
// celebration) -> a new Kickoff - and putting the ball back on the center spot. The goal itself is spotted by the physics: the trigger box in the
// mouth of each goal tells the game when the ball is in it (soccer_goal_system.h).
//
// The clock of the match runs with the game's Dt (scaled by the speed slider): two seconds of game time are two seconds, however fast the frames come.
#include "soccer_components.h"

namespace soccer
{
    struct ball_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Ball" };
        using query = std::tuple<xecs::query::must<xlioncore::transform, ball>>;

        ball_system(xecs::game_mgr::instance& GameMgr) noexcept : xecs::system::instance(GameMgr) {}

        void OnUpdate(void) noexcept
        {
            const auto* pGame = xlioncore::game::From(getGameMgr());
            if (!pGame) return;
            const float Dt = pGame->m_Time.m_DeltaTime;                 // game time: the speed slider slows or speeds up the match

            //
            // The rules (copied out: nothing is held while the others are visited)
            //
            match Rules;
            if (!ReadMatch(*this, Rules)) return;

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
            // The ball: back to the center spot at a new round, and its velocity where everybody can see it
            //
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, ball, xlioncore::physics::physics_dynamics>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity& Entity, xlioncore::transform& T, ball& B, const xlioncore::physics::physics_dynamics& Body) noexcept
                {
                    B.m_Velocity = Body.m_LinearVelocity;                  // so the Inspector (and the players) can see it

                    if (bNewRound)                                         // back to the center spot, standing still
                        xlioncore::physics::TeleportDynamicBody(getGameMgr(), Entity, xmath::fvec3(0.0f, T.m_Position.m_Y, 0.0f), xmath::fquat::fromIdentity());
                });
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
