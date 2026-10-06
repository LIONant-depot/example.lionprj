#pragma once

// The components of the soccer game. They hold data only; the systems (soccer_*_system.h) do the work. Each one appears in the editor's
// "Add Component" list under the category "Soccer", and every property below is editable in the Inspector.
//
//      match       the rules and the score: pitch size, goal width, who scored. One entity, the pitch (it also carries the green floor's Primitive).
//      goal        a trigger box in the mouth of a goal: which team defends it (the physics tells the game when the ball enters it).
//      ball        who kicked the ball last (the ball itself is the engine's: a PhysicsDynamics body with a sphere collider).
//      player      a member of a team: which team, which role, where he stands when he has nothing better to do.
//      referee     the referee: he follows the play at a distance (he is not a player, he never kicks).
//      identity    a number that is unique among the players, the referee and the ball.
//      score_bar   a bar on the scoreboard that grows with a team's score.
//      score_label a Text (the engine's) that says a team's name and its goals.
//      banner      a Text that says GOAL! while the goal is celebrated.
#include "soccer_common.h"

namespace soccer
{
    //------------------------------------------------------------------------------------------------------------
    // The state of a match: what the systems agree on.
    //------------------------------------------------------------------------------------------------------------
    enum class phase : std::uint8_t { KICKOFF, PLAYING, GOAL };

    inline constexpr auto phase_list_v = std::array
    { xproperty::settings::enum_item{ "Kickoff", phase::KICKOFF }
    , xproperty::settings::enum_item{ "Playing", phase::PLAYING }
    , xproperty::settings::enum_item{ "Goal",    phase::GOAL    }
    };

    struct match
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "SoccerMatch" };

        // The rules (edit these).
        float   m_HalfLength    = 12.0f;        // the goals are at x = -HalfLength (Blue defends) and +HalfLength (Red defends)
        float   m_HalfWidth     = 7.0f;         // the touchlines are at z = -HalfWidth and +HalfWidth
        float   m_GoalHalfWidth = 2.0f;         // the mouth of each goal is this far to each side of its center
        float   m_KickoffDelay  = 2.0f;         // seconds everybody waits before the ball is in play
        float   m_GoalPause     = 2.5f;         // seconds the celebration lasts after a goal

        // What is going on (the systems write these; the Inspector shows them while it plays).
        phase   m_Phase         = phase::KICKOFF;
        float   m_Timer         = 2.0f;         // seconds left in the current phase
        int     m_ScoreBlue     = 0;
        int     m_ScoreRed      = 0;
        team    m_LastScorer    = team::BLUE;
        int     m_Round         = 0;            // goes up at every kickoff: the others see it change and walk back to their places

        XPROPERTY_DEF
        ( "SoccerMatch", match
        , obj_member<"HalfLength",    &match::m_HalfLength>
        , obj_member<"HalfWidth",     &match::m_HalfWidth>
        , obj_member<"GoalHalfWidth", &match::m_GoalHalfWidth>
        , obj_member<"KickoffDelay",  &match::m_KickoffDelay>
        , obj_member<"GoalPause",     &match::m_GoalPause>
        , obj_member<"Phase",         &match::m_Phase, member_enum_span<phase_list_v>, member_flags<flags::SHOW_READONLY>>
        , obj_member<"Timer",         &match::m_Timer,                                  member_flags<flags::SHOW_READONLY>>
        , obj_member<"ScoreBlue",     &match::m_ScoreBlue,                              member_flags<flags::SHOW_READONLY>>
        , obj_member<"ScoreRed",      &match::m_ScoreRed,                               member_flags<flags::SHOW_READONLY>>
        , obj_member<"LastScorer",    &match::m_LastScorer, member_enum_span<team_list_v>, member_flags<flags::SHOW_READONLY>>
        , obj_member<"Round",         &match::m_Round,                                  member_flags<flags::SHOW_READONLY>>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(match, "Soccer", 100)

    // The rules of the match, copied out of the pitch entity (there is one match) so that nothing is held while the systems visit the others. False when the scene has no match.
    template< typename T_SYSTEM >
    inline bool ReadMatch(T_SYSTEM& System, match& Rules) noexcept
    {
        xecs::query::instance Query;
        Query.m_Must.AddFromComponents<match>();
        bool bFound = false;
        System.Foreach(System.Search(Query), [&](const xecs::component::entity&, const match& M) noexcept { Rules = M; bFound = true; });
        return bFound;
    }

    //------------------------------------------------------------------------------------------------------------
    // A goal: the entity is a static box with a sensor collider in the mouth of the goal, from the goal line to the back of the net. The ball entering it is a goal for the team that does not defend it.
    //------------------------------------------------------------------------------------------------------------
    struct goal
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "SoccerGoal" };

        team            m_Defender      = team::RED;                    // the team that defends this goal (Blue defends the one at -x, Red the one at +x)

        XPROPERTY_DEF
        ( "SoccerGoal", goal
        , obj_member<"Defender", &goal::m_Defender, member_enum_span<team_list_v>>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(goal, "Soccer", 105)

    //------------------------------------------------------------------------------------------------------------
    // The ball: it rolls on the ground, slows down, and bounces off the touchlines and the end lines. How fast it goes is its PhysicsDynamics' (LinearVelocity): it is not kept twice.
    //------------------------------------------------------------------------------------------------------------
    struct ball
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "SoccerBall" };

        team            m_LastTouch     = team::BLUE;                   // the team that kicked it last
        int             m_LastKicker    = -1;                           // the identity of the player that kicked it last
        int             m_Round         = -1;                           // the round of the match it last went back to the center spot for (-1: it has not seen the match yet)
        bool            m_bHeld         = false;                        // a goalkeeper has it in his hands: it goes where he goes, and nobody else can play it

        XPROPERTY_DEF
        ( "SoccerBall", ball
        , obj_member<"LastTouch",  &ball::m_LastTouch, member_enum_span<team_list_v>, member_flags<flags::SHOW_READONLY>>
        , obj_member<"LastKicker", &ball::m_LastKicker,                            member_flags<flags::SHOW_READONLY>>
        , obj_member<"Round",      &ball::m_Round,                                 member_flags<flags::SHOW_READONLY>>
        , obj_member<"Held",       &ball::m_bHeld,                                 member_flags<flags::SHOW_READONLY>>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(ball, "Soccer", 110)

    //------------------------------------------------------------------------------------------------------------
    // A player. The same component makes a field player and a goalkeeper; the role says which.
    //------------------------------------------------------------------------------------------------------------
    struct player
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "SoccerPlayer" };

        team            m_Team          = team::BLUE;
        role            m_Role          = role::FIELD;
        xmath::fvec3    m_Home          = xmath::fvec3::fromZero();     // where he stands when the ball is far (a goalkeeper: the middle of his goal line)
        float           m_Speed         = 5.5f;                         // top running speed, meters per second
        float           m_KickSpeed     = 11.0f;                        // how hard he kicks, meters per second of ball speed
        float           m_Skill         = 0.85f;                        // 0 .. 1: how well he aims (1 never misses his target)

        float           m_Cooldown      = 0.0f;                         // seconds before he may kick again
        int             m_Round         = -1;                           // the kickoff he last walked to his place for
        bool            m_bHolding      = false;                        // a goalkeeper that has the ball in his hands
        float           m_HoldTime      = 0.0f;                         // seconds left before he kicks it away

        XPROPERTY_DEF
        ( "SoccerPlayer", player
        , obj_member<"Team",      &player::m_Team, member_enum_span<team_list_v>>
        , obj_member<"Role",      &player::m_Role, member_enum_span<role_list_v>>
        , obj_member<"Home",      &player::m_Home>
        , obj_member<"Speed",     &player::m_Speed>
        , obj_member<"KickSpeed", &player::m_KickSpeed>
        , obj_member<"Skill",     &player::m_Skill>
        , obj_member<"Holding",   &player::m_bHolding, member_flags<flags::SHOW_READONLY>>
        , obj_member<"HoldTime",  &player::m_HoldTime, member_flags<flags::SHOW_READONLY>>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(player, "Soccer", 120)

    //------------------------------------------------------------------------------------------------------------
    // The referee: a man in dark grey who stays near the ball without getting in the way.
    //------------------------------------------------------------------------------------------------------------
    struct referee
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "SoccerReferee" };

        float           m_Distance      = 3.5f;                         // how far from the ball he stands
        float           m_Speed         = 4.5f;

        XPROPERTY_DEF
        ( "SoccerReferee", referee
        , obj_member<"Distance", &referee::m_Distance>
        , obj_member<"Speed",    &referee::m_Speed>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(referee, "Soccer", 130)

    //------------------------------------------------------------------------------------------------------------
    // Identity: who is who. The shadow and the name tag of a player are CHILDREN of the player (their Transform is relative to his: they go where he goes), so they need no number.
    //------------------------------------------------------------------------------------------------------------
    struct identity
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "SoccerIdentity" };

        int             m_Id            = 0;                            // unique among the players, the referee and the ball

        XPROPERTY_DEF
        ( "SoccerIdentity", identity
        , obj_member<"Id", &identity::m_Id>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(identity, "Soccer", 140)

    //------------------------------------------------------------------------------------------------------------
    // The scoreboard: a bar per team that gets longer with every goal.
    //------------------------------------------------------------------------------------------------------------
    struct score_bar
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "SoccerScoreBar" };

        team            m_Team          = team::BLUE;
        float           m_Start         = 0.0f;                         // where the bar starts along x
        float           m_PerGoal       = 0.8f;                         // how much longer it gets with each goal, meters

        XPROPERTY_DEF
        ( "SoccerScoreBar", score_bar
        , obj_member<"Team",    &score_bar::m_Team, member_enum_span<team_list_v>>
        , obj_member<"Start",   &score_bar::m_Start>
        , obj_member<"PerGoal", &score_bar::m_PerGoal>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(score_bar, "Soccer", 160)

    //------------------------------------------------------------------------------------------------------------
    // Words on the screen. These two go with an engine Text on the same entity: the game writes what it says, the engine draws it.
    //------------------------------------------------------------------------------------------------------------
    struct score_label
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "SoccerScoreLabel" };

        team            m_Team          = team::BLUE;                   // whose goals it says: "BLUE 2"

        XPROPERTY_DEF
        ( "SoccerScoreLabel", score_label
        , obj_member<"Team", &score_label::m_Team, member_enum_span<team_list_v>>
        )
    };
    XSCRIPT_REGISTER_COMPONENT(score_label, "Soccer", 170)

    struct banner
    {
        constexpr static auto typedef_v = xecs::component::type::data{ .m_pName = "SoccerBanner" };

        XPROPERTY_DEF
        ( "SoccerBanner", banner )
    };
    XSCRIPT_REGISTER_COMPONENT(banner, "Soccer", 190)
}
