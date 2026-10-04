#pragma once

// Name tags: a Text (the engine's) over every player, saying his name. A name tag is an ordinary entity (a Transform, a Text and a SoccerNameTag); the system looks for the thing with the same
// identity number and moves the tag to float over it. The name itself is the Text's own (set in the scene), and the Text turns to face the camera.
#include "soccer_components.h"

namespace soccer
{
    struct name_tag_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Name Tags" };
        using query = std::tuple<xecs::query::must<xlioncore::transform, name_tag>>;

        void OnUpdate(void) noexcept
        {
            struct owner { int m_Id; xmath::fvec3 m_Pos; };
            std::vector<owner> Owners;
            Owners.reserve(32);
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, identity>();
                Query.m_NoneOf.AddFromComponents<name_tag>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, const xlioncore::transform& T, const identity& Id) noexcept { Owners.push_back({ Id.m_Id, T.m_Position }); });
            }

            xecs::query::instance Query;
            Query.m_Must.AddFromComponents<xlioncore::transform, name_tag>();
            auto S = Search(Query);
            Foreach(S, [&](const xecs::component::entity&, xlioncore::transform& T, const name_tag& Tag) noexcept
            {
                for (const owner& O : Owners)
                {
                    if (O.m_Id != Tag.m_OwnerId) continue;
                    T.m_Position = xmath::fvec3(O.m_Pos.m_X, O.m_Pos.m_Y + Tag.m_Height, O.m_Pos.m_Z);
                    break;
                }
            });
        }
    };
    XSCRIPT_REGISTER_SYSTEM(name_tag_system)
}
