#pragma once

// The scoreboard: a bar for each team beside the pitch that gets longer with every goal, the number of goals written beside each one, and a GOAL! over the pitch while a goal is celebrated.
#include "soccer_components.h"

namespace soccer
{
    struct scoreboard_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Scoreboard" };
        using query = std::tuple<xecs::query::must<xlioncore::transform, score_bar>>;

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

            // the words (written only when they change: a Text that is not touched is not laid out again)
            const auto Say = [](xlionrender::text& Tx, const std::wstring& Words) noexcept { if (Tx.m_Text != Words) Tx.m_Text = Words; };
            {
                xecs::query::instance Labels;
                Labels.m_Must.AddFromComponents<xlionrender::text, score_label>();
                auto L = Search(Labels);
                Foreach(L, [&](xlionrender::text& Tx, const score_label& Label) noexcept
                {
                    const bool bBlue = Label.m_Team == team::BLUE;
                    Say(Tx, std::wstring(bBlue ? L"BLUE " : L"RED ") + std::to_wstring(Goals[bBlue ? 0 : 1]));
                });
            }
            {
                xecs::query::instance Banners;
                Banners.m_Must.AddFromComponents<xlionrender::text, banner>();
                auto B = Search(Banners);
                const std::wstring Goal = std::wstring(L"GOAL!\n") + (Rules.m_LastScorer == team::BLUE ? L"BLUE SCORES" : L"RED SCORES");
                Foreach(B, [&](xlionrender::text& Tx) noexcept { Say(Tx, Rules.m_Phase == phase::GOAL ? Goal : std::wstring()); });
            }
        }
    };
    XSCRIPT_REGISTER_SYSTEM(scoreboard_system)
}
