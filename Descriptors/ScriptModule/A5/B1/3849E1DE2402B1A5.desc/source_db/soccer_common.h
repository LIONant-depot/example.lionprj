#pragma once

// Soccer - a small five-a-side game made with the editor. Shared pieces: the two small enums, a bit of flat (x, z) math, and the frame clock.
//
// The pitch lies in the x/z plane (y is up): Blue defends the goal at -x and attacks +x, Red the other way round.
#include "plugins/xscript_module.plugin/source/Runtime/xscript_registration.h"
#include "dependencies/xLIONCore/src/transform/xlioncore_transform.h"

// The engine's physics: only its types (it is registered once, by the engine) and the call that moves a dynamic body.
#define XSCRIPT_IMPORT_ONLY
#include "dependencies/xLIONCore/src/physics/xlioncore_physics.h"
#undef  XSCRIPT_IMPORT_ONLY
#include "dependencies/xLIONCore/src/physics/xlioncore_physics_api.h"
#include "dependencies/xLIONCore/src/game/xlioncore_game.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>

// The systems of this module query the engine's Transform, which another binary registered: say so (see xscript_registration.h).
XSCRIPT_USES_COMPONENT(xlioncore::transform)
XSCRIPT_USES_COMPONENT(xlioncore::physics::physics_dynamics)

namespace soccer
{
    enum class team : std::uint8_t { BLUE, RED };
    enum class role : std::uint8_t { FIELD, KEEPER };

    inline constexpr auto team_list_v = std::array
    { xproperty::settings::enum_item{ "Blue", team::BLUE }
    , xproperty::settings::enum_item{ "Red",  team::RED  }
    };

    inline constexpr auto role_list_v = std::array
    { xproperty::settings::enum_item{ "Field",  role::FIELD  }
    , xproperty::settings::enum_item{ "Keeper", role::KEEPER }
    };

    // +1 when the team attacks toward +x (Blue), -1 toward -x (Red).
    inline constexpr float AttackDirection(team Team) noexcept { return Team == team::BLUE ? 1.0f : -1.0f; }
    inline constexpr team  Opponent(team Team)        noexcept { return Team == team::BLUE ? team::RED : team::BLUE; }

    // A point on the ground. The game is played flat, so most of the math is two-dimensional.
    struct vec2
    {
        float x = 0.0f, z = 0.0f;

        vec2 operator+(vec2 O) const noexcept { return { x + O.x, z + O.z }; }
        vec2 operator-(vec2 O) const noexcept { return { x - O.x, z - O.z }; }
        vec2 operator*(float S) const noexcept { return { x * S, z * S }; }
    };

    inline vec2  Flat(const xmath::fvec3& V) noexcept { return { V.m_X, V.m_Z }; }
    inline float Length(vec2 V) noexcept { return std::sqrt(V.x * V.x + V.z * V.z); }
    inline float Distance(vec2 A, vec2 B) noexcept { return Length(A - B); }
    inline float Dot(vec2 A, vec2 B) noexcept { return A.x * B.x + A.z * B.z; }

    // The direction of V with length 1; a zero vector stays zero.
    inline vec2 Normalized(vec2 V) noexcept
    {
        const float L = Length(V);
        return L > 1.0e-5f ? V * (1.0f / L) : vec2{};
    }

    inline float Clamp(float V, float Lo, float Hi) noexcept { return V < Lo ? Lo : (V > Hi ? Hi : V); }

    // What follows the physics runs in the fixed steps of the game's time (xlioncore::game_time: m_FixedSteps are due each frame, each
    // kFixedDt long), what belongs to the clock of the game (the phases of the match, the hops of a celebration) uses its m_DeltaTime, which the
    // speed slider of the editor scales. Both come from the game the systems belong to:
    //     const auto& Time = xlioncore::game::From(GameMgr)->m_Time;
    inline constexpr float kFixedDt = xlioncore::game_time::kDefaultFixedDeltaTime;

    // A push on a body for the coming step: the force is ADDED to what the body is already going to get, and the sum is never allowed to pass
    // MaxNewtons (a person is not a superhero, a foot is not a cannon).
    inline void AddForce(xlioncore::physics::physics_dynamics& Body, float X, float Y, float Z, float MaxNewtons) noexcept
    {
        Body.m_Force.m_X += X; Body.m_Force.m_Y += Y; Body.m_Force.m_Z += Z;
        const float L = std::sqrt(Body.m_Force.m_X * Body.m_Force.m_X + Body.m_Force.m_Y * Body.m_Force.m_Y + Body.m_Force.m_Z * Body.m_Force.m_Z);
        if (L > MaxNewtons)
        {
            const float k = MaxNewtons / L;
            Body.m_Force.m_X *= k; Body.m_Force.m_Y *= k; Body.m_Force.m_Z *= k;
        }
    }

    // The push a runner may be given: at his top speed the part of it that points the way he is already going is taken away (the sideways
    // part, which only steers him, stays), so no force ever makes a runner faster than he can be.
    inline vec2 LimitedPush(vec2 Push, vec2 Velocity, float TopSpeed) noexcept
    {
        const float Speed = Length(Velocity);
        if (Speed < TopSpeed) return Push;
        const vec2  Ahead = Velocity * (1.0f / Speed);
        const float Along = Dot(Push, Ahead);
        return Along > 0.0f ? Push - Ahead * Along : Push;
    }

    inline constexpr float kMaxRunForce  =  700.0f;      // N: 75 kg * 9 m/s2, the start of a sprint
    inline constexpr float kMaxJumpForce = 2800.0f;      // N: the push of two legs
    inline constexpr float kMaxKickForce =  600.0f;      // N for one fixed step: 0.45 kg to about 22 m/s, the hardest a foot kicks a football

    // The velocity change a kick asks of the ball, as the one force that makes it in one fixed step:   F = m * dV / dt
    inline xmath::fvec3 ForceFor(const xlioncore::physics::physics_dynamics& Ball, float TargetX, float TargetZ) noexcept
    {
        return xmath::fvec3( Ball.m_Mass * (TargetX - Ball.m_LinearVelocity.m_X) / kFixedDt
                           , 0.0f
                           , Ball.m_Mass * (TargetZ - Ball.m_LinearVelocity.m_Z) / kFixedDt );
    }

    // A tiny deterministic random source, so a match plays out the same way twice (and a test can rely on it).
    struct random
    {
        std::uint32_t m_State = 0x9E3779B9u;

        std::uint32_t Next() noexcept { m_State = m_State * 1664525u + 1013904223u; return m_State >> 8; }
        float         Unit() noexcept { return static_cast<float>(Next() & 0xFFFFu) / 65535.0f; }                   // 0 .. 1
        float         Range(float Lo, float Hi) noexcept { return Lo + (Hi - Lo) * Unit(); }
    };
}
