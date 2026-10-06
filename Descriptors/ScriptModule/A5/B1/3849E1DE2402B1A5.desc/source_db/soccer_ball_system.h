#pragma once

// The ball.
//
// The ball itself belongs to the physics (a dynamic body with a sphere collider: it rolls, bounces off the lines' walls, the posts and the net). What is left for this system is the
// one thing that is the ball's own: at every new round (the number of the round is the match's, soccer_match_system.h) it goes back to the center spot. The goal itself is spotted by
// the physics: the trigger box in the mouth of each goal tells the game when the ball is in it (soccer_goal_system.h).
#include "soccer_components.h"

namespace soccer
{
    struct ball_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Ball" };
        using query = std::tuple<xecs::query::must<xlioncore::physics::physics_dynamics>>;

        void operator()(const xecs::component::entity& Entity, xlioncore::transform& T, ball& Ball) noexcept
        {
            QForeach([&](const match& Rules) noexcept
            {
                if (Ball.m_Round == Rules.m_Round) return;
                const bool bFirstTime = Ball.m_Round < 0;               // a ball that has not seen the match yet stays where it was made
                Ball.m_Round = Rules.m_Round;
                if (bFirstTime) return;
                Ball.m_bHeld = false;                                   // (a goalkeeper lets go of it: his round is over too)

                // back to the center spot, standing still
                xlioncore::physics::TeleportDynamicBody(getGameMgr(), Entity, xmath::fvec3(0.0f, T.m_Position.m_Y, 0.0f), xmath::fquat::fromIdentity());
            });
        }
    };
    XSCRIPT_REGISTER_SYSTEM(ball_system)
}
