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
            match Rules;
            if (!ReadMatch(*this, Rules)) return;
            const int Goals[2] = { Rules.m_ScoreBlue, Rules.m_ScoreRed };

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
