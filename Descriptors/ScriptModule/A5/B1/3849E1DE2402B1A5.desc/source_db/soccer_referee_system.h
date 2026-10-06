#pragma once

// The referee: he walks along the touchline (the one he is on) and goes where the ball goes in x, never in z: the ball is always across from him. He is neutral: when a goal is scored
// he blows the whistle and stands on the spot until the celebration is over (the players are the ones that celebrate).
#include "soccer_components.h"

namespace soccer
{
    struct referee_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Referee" };
        using query = std::tuple<xecs::query::must<xlioncore::transform, referee>>;

        // Needs a place that runs it once for each fixed step (every run is one fixed delta time long): it takes ONE step each time it runs and never counts the steps itself. Placed in
        // the Before Step connector of the Physics system (how the example is set up) it runs right before the world takes the step.
        using constraints = std::tuple<xlioncore::constraint::fixed_delta_time>;

        void operator()(xlioncore::transform& T, referee& R, xlioncore::physics::physics_dynamics& Body) noexcept
        {
            vec2 BallPos;
            QForeach([&](const xlioncore::transform& BallT, const ball&) noexcept { BallPos = Flat(BallT.m_Position); });

            QForeach( [&](const match& Rules) noexcept
            {
                const vec2 Pos = Flat(T.m_Position);
                const vec2 Vel = Flat(Body.m_LinearVelocity);

                // a spot on his line, across from the ball: the line is inside the touchline he is on (the walls are on the other side of it), and he only ever moves along it
                const float Line = (T.m_Position.m_Z < 0.0f ? -1.0f : 1.0f) * (Rules.m_HalfWidth - 1.0f);
                const vec2  Target = { Clamp(BallPos.x, -Rules.m_HalfLength + 1.0f, Rules.m_HalfLength - 1.0f), Line };

                // the velocity he wants (none after a goal: he stands to blow the whistle) and the force that gets him there, F = m * a,
                // with the acceleration of a man who is not sprinting
                vec2 Desired = {};
                if (Rules.m_Phase != phase::GOAL)
                {
                    Desired.x = R.m_Speed * Clamp(Target.x - Pos.x, -1.0f, 1.0f);                   // along the line toward the ball (slowing down as he arrives)
                    Desired.z = R.m_Speed * 0.5f * Clamp(Target.z - Pos.z, -1.0f, 1.0f);            // and back onto it if he was pushed off
                    const vec2  FromBall = Pos - BallPos;                                           // never too near the ball: out of its way along the line
                    if (Length(FromBall) < R.m_Distance * 0.63f) Desired.x += (FromBall.x < 0.0f ? -3.0f : 3.0f);
                }
                if (Length(Desired) > R.m_Speed) Desired = Normalized(Desired) * R.m_Speed;           // nobody runs faster than his top speed
                vec2 Accel = (Desired - Vel) * (1.0f / kFixedDt);
                const float MaxAccel = 8.0f;                                                        // m/s2 (80 kg: 640 N, a jog; less than that and the push does not beat the friction of the ground and he stands still)
                if (Length(Accel) > MaxAccel) Accel = Normalized(Accel) * MaxAccel;
                const vec2 Push = LimitedPush(Accel * Body.m_Mass, Vel, R.m_Speed);                    // (at his top speed: no more push ahead)
                AddForce(Body, Push.x, 0.0f, Push.z, kMaxRunForce);
            });
        }
    };
    XSCRIPT_REGISTER_SYSTEM(referee_system)
}
