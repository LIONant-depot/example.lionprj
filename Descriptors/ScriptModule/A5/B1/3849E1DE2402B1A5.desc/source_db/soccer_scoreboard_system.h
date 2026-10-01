#pragma once

// The scoreboard: a bar for each team beside the pitch that gets longer with every goal (there is no text on screen to write numbers with).
#include "soccer_components.h"

namespace soccer
{
    struct scoreboard_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Scoreboard" };
        using query = std::tuple<xecs::query::must<xlioncore::transform, score_bar>>;

        scoreboard_system(xecs::game_mgr::instance& GameMgr) noexcept : xecs::system::instance(GameMgr) {}

        void OnUpdate(void) noexcept
        {
            int Goals[2] = { 0, 0 };
            bool bHaveRules = false;
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<match>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, const match& M) noexcept { Goals[0] = M.m_ScoreBlue; Goals[1] = M.m_ScoreRed; bHaveRules = true; });
            }
            if (!bHaveRules) return;

            xecs::query::instance Query;
            Query.m_Must.AddFromComponents<xlioncore::transform, score_bar>();
            auto S = Search(Query);
            Foreach(S, [&](const xecs::component::entity&, xlioncore::transform& T, const score_bar& Bar) noexcept
            {
                const float Length = std::max(0.1f, static_cast<float>(Goals[Bar.m_Team == team::BLUE ? 0 : 1]) * Bar.m_PerGoal);
                T.m_Scale.m_X    = Length;
                T.m_Position.m_X = Bar.m_Start + Length * 0.5f;                                      // it grows from its start
            });
        }
    };
    XSCRIPT_REGISTER_SYSTEM(scoreboard_system)
}
