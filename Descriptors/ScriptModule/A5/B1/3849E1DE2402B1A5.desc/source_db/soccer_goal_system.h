#pragma once

// The goals: the physics tells the game when something enters a sensor (xlioncore::physics::sensor_begin_event). The sensor of a goal is a trigger box in its mouth
// (the entity has a SoccerGoal); when the ball enters it during play, the team that does not defend that goal scores. No position is compared any more.
#include "soccer_components.h"

namespace soccer
{
    // The data component of an entity, by handle (a system that is not iterating an entity reaches it this way).
    template< typename T >
    inline T* ComponentOf(xecs::game_mgr::instance& GameMgr, xecs::component::entity Entity) noexcept
    {
        auto& Details = GameMgr.m_ComponentMgr.getEntityDetails(Entity);
        if (!Details.m_pPool) return nullptr;
        const auto iType = Details.m_pPool->findIndexComponentFromInfo(xecs::component::type::info_v<T>);
        if (iType < 0) return nullptr;
        return reinterpret_cast<T*>(&Details.m_pPool->m_pComponent[iType][Details.m_PoolIndex.m_Value * xecs::component::type::info_v<T>.m_Size]);
    }

    struct goal_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::global_event<xlioncore::physics::sensor_begin_event>{ .m_pName = "Soccer Goal" };

        goal_system(xecs::game_mgr::instance& GameMgr) noexcept : xecs::system::instance(GameMgr), m_Mgr(GameMgr) {}

        xecs::game_mgr::instance& m_Mgr;

        void OnEvent(xecs::component::entity Sensor, xecs::component::entity Visitor) noexcept
        {
            const goal* pGoal = ComponentOf<goal>(m_Mgr, Sensor);
            if (!pGoal || !ComponentOf<ball>(m_Mgr, Visitor)) return;               // a sensor that is not a goal, or something other than the ball in it

            xecs::query::instance Query;
            Query.m_Must.AddFromComponents<match>();
            auto S = Search(Query);
            Foreach(S, [&](const xecs::component::entity&, match& Rules) noexcept
            {
                if (Rules.m_Phase != phase::PLAYING) return;                         // (in the net the ball just stays in the net)
                const team Scorer = pGoal->m_Defender == team::BLUE ? team::RED : team::BLUE;
                Rules.m_Phase      = phase::GOAL;
                Rules.m_Timer      = Rules.m_GoalPause;
                Rules.m_LastScorer = Scorer;
                (Scorer == team::BLUE ? Rules.m_ScoreBlue : Rules.m_ScoreRed) += 1;
            });
        }
    };
    XSCRIPT_REGISTER_SYSTEM(goal_system)
}
