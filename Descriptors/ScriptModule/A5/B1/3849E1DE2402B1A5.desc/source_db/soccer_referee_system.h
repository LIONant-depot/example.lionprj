#pragma once

// The referee: he keeps up with the ball from the side and a little behind it, and keeps out of the players' way. When a goal is scored he
// blows the whistle: he hops on the spot until the celebration is over.
#include "soccer_components.h"

namespace soccer
{
    struct referee_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Referee" };
        using query = std::tuple<xecs::query::must<xlioncore::transform, referee>>;

        referee_system(xecs::game_mgr::instance& GameMgr) noexcept : xecs::system::instance(GameMgr) {}

        // Connected to the "Before Step" connector of the Physics system this runs once for each step; not connected, it takes the fixed steps
        // the game's time says are due this frame itself.
        void OnUpdate(void) noexcept
        {
            if (isConnected()) { FixedStep(); return; }
            if (const auto* pGame = xlioncore::game::From(getGameMgr()))
                for (int n = pGame->m_Time.m_FixedSteps; n > 0; --n) FixedStep();
        }

        void FixedStep(void) noexcept
        {
            const float Dt = kFixedDt;

            match Rules;
            if (!ReadMatch(*this, Rules)) return;

            vec2 BallPos;
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, ball>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, const xlioncore::transform& T, const ball&) noexcept { BallPos = Flat(T.m_Position); });
            }

            xecs::query::instance Query;
            Query.m_Must.AddFromComponents<xlioncore::transform, referee, xlioncore::physics::physics_dynamics>();
            auto S = Search(Query);
            Foreach(S, [&](const xecs::component::entity&, xlioncore::transform& T, referee& R, xlioncore::physics::physics_dynamics& Body) noexcept
            {
                const vec2 Pos = Flat(T.m_Position);
                const vec2 Vel = Flat(Body.m_LinearVelocity);

                // a spot beside the ball, toward the middle of the pitch, so the players do not run into him
                vec2 Back = Normalized(vec2{} - BallPos);
                if (Length(Back) < 0.5f) Back = { 1.0f, 0.0f };
                const vec2 Side   = { -Back.z, Back.x };
                const vec2 Target = BallPos + Back * (R.m_Distance * 0.5f) + Side * (R.m_Distance * 0.85f);

                // the velocity he wants (none after a goal: he stands to blow the whistle) and the force that gets him there, F = m * a,
                // with the acceleration of a man who is not sprinting
                vec2 Desired = {};
                if (Rules.m_Phase != phase::GOAL)
                {
                    const vec2  To   = Target - Pos;
                    const float Dist = Length(To);
                    Desired = Normalized(To) * (R.m_Speed * Clamp(Dist / 1.0f, 0.0f, 1.0f));
                    const vec2  FromBall = Pos - BallPos;                                           // never too near the ball
                    if (Length(FromBall) < 2.2f) Desired = Desired + Normalized(FromBall) * 3.0f;
                }
                if (Length(Desired) > R.m_Speed) Desired = Normalized(Desired) * R.m_Speed;           // nobody runs faster than his top speed
                vec2 Accel = (Desired - Vel) * (1.0f / kFixedDt);
                const float MaxAccel = 5.0f;                                                        // m/s2
                if (Length(Accel) > MaxAccel) Accel = Normalized(Accel) * MaxAccel;
                const vec2 Push = LimitedPush(Accel * Body.m_Mass, Vel, R.m_Speed);                    // (at his top speed: no more push ahead)
                AddForce(Body, Push.x, 0.0f, Push.z, kMaxRunForce);

                // he hops when there is a goal, as the players do
                const bool bGrounded = T.m_Position.m_Y < T.m_Scale.m_Y * 0.5f + 0.12f && Body.m_LinearVelocity.m_Y < 3.0f;
                if (Rules.m_Phase == phase::GOAL && bGrounded && std::sin(Rules.m_Timer * 5.0f) > 0.0f) AddForce(Body, 0.0f, 2200.0f, 0.0f, kMaxJumpForce);

                R.m_Velocity = Body.m_LinearVelocity;                                               // (to be seen in the Inspector)
            });
        }
    };
    XSCRIPT_REGISTER_SYSTEM(referee_system)
}
