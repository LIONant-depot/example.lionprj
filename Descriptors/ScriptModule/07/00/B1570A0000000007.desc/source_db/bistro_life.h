#pragma once

// Bistro Life - what makes the Bistro Game level move like a game street: people walking up and down, lights that flicker, a lantern carried
// around, the sun going down. Components hold data (editable in the Inspector, category "Bistro"); one system each does the work, on the game's
// clock (its m_DeltaTime: the editor's speed slider scales it).
//
//      walker      walks its entity back and forth between A and B (x, z), facing the way it goes; its Animation keeps playing
//      (flicker: a LightPoint's intensity - waits until the render module's light header can be imported by a module: it pulls the engine's material loaders)
//      circler     moves its entity around a circle (a lantern, a phone light carried around the square)
//      day_cycle   turns the sun: its azimuth goes around, its elevation swings between a low and a high angle
#include "plugins/xscript_module.plugin/source/Runtime/xscript_registration.h"
#include "dependencies/xLIONCore/src/transform/xlioncore_transform.h"
#include "dependencies/xLIONCore/src/game/xlioncore_game.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

XSCRIPT_USES_COMPONENT(xlioncore::transform)

namespace bistro
{
    inline float Dt(xecs::game_mgr::instance& GameMgr) noexcept { const auto* pGame = xlioncore::game::From(GameMgr); return pGame ? pGame->m_Time.m_DeltaTime : 0.0f; }
    inline xmath::fquat Yaw(float Radians) noexcept { return xmath::fquat(0.0f, std::sin(Radians * 0.5f), 0.0f, std::cos(Radians * 0.5f)); }

    struct walker
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "BistroWalker" };
        float m_AX = 0, m_AZ = 0, m_BX = 4, m_BZ = 0;    // the two ends of the walk
        float m_Speed = 1.2f;                           // m/s (a stroll)
        float m_T     = 0.0f;                           // where it is: 0 at A, 1 at B, 2 back at A
        XPROPERTY_DEF
        ( "BistroWalker", walker
        , obj_member<"AX", &walker::m_AX>, obj_member<"AZ", &walker::m_AZ>, obj_member<"BX", &walker::m_BX>, obj_member<"BZ", &walker::m_BZ>
        , obj_member<"Speed", &walker::m_Speed>, obj_member<"T", &walker::m_T>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(walker, "Bistro", 200)

    struct circler
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "BistroCircler" };
        float m_CX = 0, m_CY = 1.2f, m_CZ = 0, m_Radius = 3.0f;
        float m_Speed = 0.4f;        // radians a second
        float m_Angle = 0.0f;
        XPROPERTY_DEF
        ( "BistroCircler", circler
        , obj_member<"CX", &circler::m_CX>, obj_member<"CY", &circler::m_CY>, obj_member<"CZ", &circler::m_CZ>, obj_member<"Radius", &circler::m_Radius>
        , obj_member<"Speed", &circler::m_Speed>, obj_member<"Angle", &circler::m_Angle>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(circler, "Bistro", 202)

    struct day_cycle
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "BistroDayCycle" };
        float m_AzimuthSpeed = 3.0f;     // degrees a second the sun goes around
        float m_ElevationLow = 6.0f, m_ElevationHigh = 40.0f, m_ElevationPeriod = 120.0f;     // degrees, and seconds for a full swing
        float m_Azimuth = -35.0f, m_Clock = 0.0f;
        XPROPERTY_DEF
        ( "BistroDayCycle", day_cycle
        , obj_member<"AzimuthSpeed", &day_cycle::m_AzimuthSpeed>, obj_member<"ElevationLow", &day_cycle::m_ElevationLow>, obj_member<"ElevationHigh", &day_cycle::m_ElevationHigh>
        , obj_member<"ElevationPeriod", &day_cycle::m_ElevationPeriod>, obj_member<"Azimuth", &day_cycle::m_Azimuth>, obj_member<"Clock", &day_cycle::m_Clock>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(day_cycle, "Bistro", 203)

    struct walker_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Bistro Walkers" };
        void operator()(xlioncore::transform& T, walker& W) noexcept
        {
            const float dx = W.m_BX - W.m_AX, dz = W.m_BZ - W.m_AZ, L = std::sqrt(dx * dx + dz * dz);
            if (L < 1e-3f) return;
            W.m_T = std::fmod(W.m_T + Dt(getGameMgr()) * W.m_Speed / L, 2.0f);
            const float s = W.m_T < 1.0f ? W.m_T : 2.0f - W.m_T, dir = W.m_T < 1.0f ? 1.0f : -1.0f;
            T.m_Position.m_X = W.m_AX + dx * s; T.m_Position.m_Z = W.m_AZ + dz * s;
            T.m_Rotation = Yaw(std::atan2(dx * dir, dz * dir));
        }
    };
    XSCRIPT_REGISTER_SYSTEM(walker_system)

    struct circler_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Bistro Circlers" };
        void operator()(xlioncore::transform& T, circler& C) noexcept
        {
            C.m_Angle = std::fmod(C.m_Angle + Dt(getGameMgr()) * C.m_Speed, 6.2831853f);
            T.m_Position = xmath::fvec3(C.m_CX + C.m_Radius * std::cos(C.m_Angle), C.m_CY, C.m_CZ + C.m_Radius * std::sin(C.m_Angle));
        }
    };
    XSCRIPT_REGISTER_SYSTEM(circler_system)

    struct day_cycle_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Bistro Day Cycle" };
        void operator()(xlioncore::transform& T, day_cycle& D) noexcept
        {
            const float dt = Dt(getGameMgr());
            D.m_Clock += dt; D.m_Azimuth = std::fmod(D.m_Azimuth + dt * D.m_AzimuthSpeed, 360.0f);
            const float k = 0.5f - 0.5f * std::cos(D.m_Clock / std::max(D.m_ElevationPeriod, 1.0f) * 6.2831853f);
            const float El = (D.m_ElevationLow + (D.m_ElevationHigh - D.m_ElevationLow) * k) * 0.01745329f, Az = D.m_Azimuth * 0.01745329f;
            // the light shines along the entity's -Y: the rotation that takes -Y to the sun's travel direction
            const xmath::fvec3 d(std::cos(El) * std::sin(Az), -std::sin(El), std::cos(El) * std::cos(Az)), a(0.0f, -1.0f, 0.0f);
            const xmath::fvec3 c(a.m_Y * d.m_Z - a.m_Z * d.m_Y, a.m_Z * d.m_X - a.m_X * d.m_Z, a.m_X * d.m_Y - a.m_Y * d.m_X);
            const float w = 1.0f + (a.m_X * d.m_X + a.m_Y * d.m_Y + a.m_Z * d.m_Z), n = std::sqrt(c.m_X * c.m_X + c.m_Y * c.m_Y + c.m_Z * c.m_Z + w * w);
            T.m_Rotation = xmath::fquat(c.m_X / n, c.m_Y / n, c.m_Z / n, w / n);
        }
    };
    XSCRIPT_REGISTER_SYSTEM(day_cycle_system)
}
