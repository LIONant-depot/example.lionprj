#pragma once

// Shadows: every player, the referee and the ball has a shadow under it, a flattened dark sphere that follows it on the ground.
//
// A shadow is an ordinary entity (a Primitive sphere, a Transform and a SoccerShadow component); the system looks for the thing with the same
// identity number and moves the shadow to the spot right under it. The shadow's color is chosen in the scene: the engine's Primitive has no
// transparency, so the prefab uses the pitch's green made darker, which looks the same on a flat green floor.
#include "soccer_components.h"

namespace soccer
{
    struct shadow_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Shadows" };
        using query = std::tuple<xecs::query::must<xlioncore::transform, shadow>>;

        shadow_system(xecs::game_mgr::instance& GameMgr) noexcept : xecs::system::instance(GameMgr) {}

        void OnUpdate(void) noexcept
        {
            // where everything that casts a shadow is
            struct caster { int m_Id; xmath::fvec3 m_Pos; };
            std::vector<caster> Casters;
            Casters.reserve(32);
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, identity>();
                Query.m_NoneOf.AddFromComponents<shadow>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, const xlioncore::transform& T, const identity& Id) noexcept { Casters.push_back({ Id.m_Id, T.m_Position }); });
            }

            xecs::query::instance Query;
            Query.m_Must.AddFromComponents<xlioncore::transform, shadow>();
            auto S = Search(Query);
            Foreach(S, [&](const xecs::component::entity&, xlioncore::transform& T, const shadow& Sh) noexcept
            {
                for (const caster& C : Casters)
                {
                    if (C.m_Id != Sh.m_OwnerId) continue;
                    T.m_Position = xmath::fvec3(C.m_Pos.m_X, Sh.m_Height, C.m_Pos.m_Z);
                    T.m_Scale    = xmath::fvec3(Sh.m_Size, 0.02f, Sh.m_Size);                       // a flat disc
                    break;
                }
            });
        }
    };
    XSCRIPT_REGISTER_SYSTEM(shadow_system)
}
