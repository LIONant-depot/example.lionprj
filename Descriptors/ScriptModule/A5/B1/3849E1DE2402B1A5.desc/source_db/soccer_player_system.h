#pragma once

// The players: ten of them, five a side, one of them a goalkeeper.
//
// What a field player does, in order:
//   - the one closest to the ball in his team runs at it (he "chases");
//   - the second closest runs ahead along the side, ready for a pass (he "supports");
//   - the others keep their places, leaning toward the ball;
//   - a player that reaches the ball kicks it: a shot when he is close enough to the goal, otherwise a pass to the best placed teammate
//     (ahead of him, with no opponent near him or near the way), otherwise he pushes the ball forward.
// A goalkeeper stays on his goal line following the ball sideways; when the ball comes close he goes for it, and then passes it away.
//
// The match component says what is going on: at a kickoff everybody walks to his place, and after a goal the team that scored jumps.
#include "soccer_components.h"

namespace soccer
{
    namespace details
    {
        // Everything a player needs to know about another person on the pitch.
        struct person
        {
            int     m_Id        = 0;
            team    m_Team      = team::BLUE;
            role    m_Role      = role::FIELD;
            bool    m_bReferee  = false;
            vec2    m_Pos;
            vec2    m_Vel;
        };

        // Where to stand to play a ball: a little short of it, on the side he comes from, so he never walks into it (a body in the way
        // of the ball is a body that holds it).
        inline vec2 StandOff(vec2 From, vec2 BallPoint) noexcept
        {
            const vec2 Away = From - BallPoint;
            return Length(Away) > 1.0e-3f ? BallPoint + Normalized(Away) * 0.55f : BallPoint;
        }

        // How far point P is from the segment A-B.
        inline float DistanceToSegment(vec2 P, vec2 A, vec2 B) noexcept
        {
            const vec2  AB = B - A;
            const float L2 = Dot(AB, AB);
            const float t  = L2 > 1.0e-6f ? Clamp(Dot(P - A, AB) / L2, 0.0f, 1.0f) : 0.0f;
            return Distance(P, A + AB * t);
        }

        // The vector V turned by Angle radians.
        inline vec2 Turned(vec2 V, float Angle) noexcept
        {
            const float C = std::cos(Angle), S = std::sin(Angle);
            return { V.x * C - V.z * S, V.x * S + V.z * C };
        }
    }

    struct player_system : xecs::system::instance
    {
        constexpr static auto typedef_v = xecs::system::type::update{ .m_pName = "Soccer Players" };
        using query = std::tuple<xecs::query::must<xlioncore::transform, player, identity>>;

        player_system(xecs::game_mgr::instance& GameMgr) noexcept : xecs::system::instance(GameMgr), m_Mgr(GameMgr) {}

        xecs::game_mgr::instance& m_Mgr;                                   // the physics call that puts a person back on his place needs the world

        random      m_Random;

        // A kick decided this frame, applied to the ball once everybody has been visited.
        struct kick { bool m_bValid = false; vec2 m_Velocity; team m_Team = team::BLUE; int m_Id = -1; };

        // The people move with the physics: the fixed steps the game's time says are due this frame.
        void OnUpdate(void) noexcept
        {
            if (const auto* pGame = xlioncore::game::From(m_Mgr))
                for (int n = pGame->m_Time.m_FixedSteps; n > 0; --n) FixedStep();
        }

        void FixedStep(void) noexcept
        {
            using namespace details;
            const float Dt = kFixedDt;

            //
            // What everybody knows: the rules, the ball, and where every person is.
            //
            match Rules;
            bool  bHaveRules = false;
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<match>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, const match& M) noexcept { Rules = M; bHaveRules = true; });
            }
            if (!bHaveRules) return;

            vec2 BallPos, BallVel;
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, ball>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, const xlioncore::transform& T, const ball& B) noexcept { BallPos = Flat(T.m_Position); BallVel = Flat(B.m_Velocity); });
            }

            std::vector<person> All;
            All.reserve(24);
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, player, identity>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, const xlioncore::transform& T, const player& P, const identity& Id) noexcept
                {
                    All.push_back({ Id.m_Id, P.m_Team, P.m_Role, false, Flat(T.m_Position), Flat(P.m_Velocity) });
                });
            }
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, referee, identity>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, const xlioncore::transform& T, const referee& R, const identity& Id) noexcept
                {
                    All.push_back({ Id.m_Id, team::BLUE, role::FIELD, true, Flat(T.m_Position), Flat(R.m_Velocity) });
                });
            }

            //
            // Everybody decides and moves
            //
            kick Kick;
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, player, identity, xlioncore::physics::physics_dynamics>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity& Entity, xlioncore::transform& T, player& P, const identity& Id, xlioncore::physics::physics_dynamics& Body) noexcept
                {
                    Act(Dt, Rules, BallPos, BallVel, All, Kick, Entity, T, P, Id, Body);
                });
            }

            //
            // The kick, if there was one
            //
            if (Kick.m_bValid)
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<ball, xlioncore::physics::physics_dynamics>();
                auto S = Search(Query);
                Foreach(S, [&](const xecs::component::entity&, ball& B, xlioncore::physics::physics_dynamics& Body) noexcept
                {
                    const xmath::fvec3 F = ForceFor(Body, Kick.m_Velocity.x, Kick.m_Velocity.z);       // the one push that makes the speed asked for
                    AddForce(Body, F.m_X, 0.0f, F.m_Z, kMaxKickForce);
                    B.m_LastTouch  = Kick.m_Team;
                    B.m_LastKicker = Kick.m_Id;
                });
            }
        }

        // One player, one frame.
        void Act(float Dt, const match& Rules, vec2 BallPos, vec2 BallVel, const std::vector<details::person>& All, kick& Kick
               , const xecs::component::entity& Entity, xlioncore::transform& T, player& P, const identity& Id, xlioncore::physics::physics_dynamics& Body) noexcept
        {
            using namespace details;

            const float Dir      = AttackDirection(P.m_Team);
            const vec2  Home     = Flat(P.m_Home);
            vec2        Pos      = Flat(T.m_Position);
            vec2        Vel      = Flat(Body.m_LinearVelocity);                  // what the physics says he is doing

            // A new kickoff: back to the place, standing still.
            if (P.m_Round != Rules.m_Round)
            {
                P.m_Round    = Rules.m_Round;
                P.m_Cooldown = 0.0f;
                xlioncore::physics::TeleportDynamicBody(m_Mgr, Entity, xmath::fvec3(Home.x, T.m_Position.m_Y, Home.z), xmath::fquat::fromIdentity());
                return;
            }
            P.m_Cooldown = std::max(0.0f, P.m_Cooldown - Dt);

            //
            // Where does he want to be?
            //
            vec2 Target = Home;
            const float BallDist = Distance(Pos, BallPos);

            if (Rules.m_Phase == phase::PLAYING)
            {
                if (P.m_Role == role::FIELD)
                {
                    // how many of his team-mates (field players) are closer to the ball than he is
                    int Rank = 0;
                    for (const person& O : All)
                        if (!O.m_bReferee && O.m_Team == P.m_Team && O.m_Role == role::FIELD && O.m_Id != Id.m_Id && Distance(O.m_Pos, BallPos) < BallDist - 1.0e-3f) ++Rank;

                    // the kickoff belongs to the team that conceded: the ones that scored wait until the ball moves
                    const bool bWaiting = P.m_Team == Rules.m_LastScorer && Rules.m_Round > 0 && Length(BallPos) < 0.3f && Length(BallVel) < 0.1f;

                    if (bWaiting)       Target = Home;
                    else if (Rank == 0) Target = StandOff(Pos, BallPos + BallVel * 0.25f);                                                            // chase
                    else if (Rank == 1) Target = BallPos + vec2{ Dir * 3.5f, (Pos.z >= BallPos.z ? 1.0f : -1.0f) * 3.0f };            // support: ahead and to the side
                    else                                                                                                               // keep the place, lean to the ball
                    {
                        Target = Home + (BallPos - Home) * 0.3f;
                        if (BallPos.x * Dir > 0.0f) Target.x += Dir * 1.5f;                                                            // the ball is in their half: move up
                    }

                    if (!bWaiting && BallDist < kKickRange && P.m_Cooldown <= 0.0f) DecideKickField(Rules, Dir, Pos, BallPos, All, Kick, P, Id);
                }
                else                                                                                                                   // goalkeeper
                {
                    const float Reach = Rules.m_GoalHalfWidth - 0.3f;
                    Target = { Home.x, Clamp(BallPos.z * 0.55f, -Reach, Reach) };
                    const bool bComing = BallVel.x * Dir < 0.2f;                                                                       // not moving away from his goal
                    if (Distance(BallPos, Home) < 4.5f && bComing) Target = StandOff(Pos, BallPos);                                                   // rush out

                    if (BallDist < kKickRange && P.m_Cooldown <= 0.0f) DecideKickKeeper(Rules, Dir, Pos, BallPos, All, Kick, P, Id);
                }
            }

            //
            // Moving there: the velocity he wants, and the force that gets him there. F = m * a, with the acceleration of a person (a sprinter
            // gets about 9 m/s2, so a 75 kg player pushes with about 700 N at most), and V1 = V0 + a * dt, P1 = P0 + V1 * dt is the physics' job.
            //
            vec2 Desired = {};
            if (Rules.m_Phase != phase::GOAL)                                                                                          // (after a goal everybody stops)
            {
                const vec2  ToTarget = Target - Pos;
                const float Dist     = Length(ToTarget);
                Desired = Normalized(ToTarget) * (P.m_Speed * Clamp(Dist / 0.6f, 0.0f, 1.0f));                                         // slow down on arrival

                for (const person& O : All)                                                                                            // do not walk through each other
                {
                    if (O.m_Id == Id.m_Id) continue;
                    const vec2  Away = Pos - O.m_Pos;
                    const float L    = Length(Away);
                    if (L < 1.0f && L > 1.0e-3f) Desired = Desired + Away * ((1.0f - L) * 3.0f / L);
                }
            }
            if (Length(Desired) > P.m_Speed) Desired = Normalized(Desired) * P.m_Speed;                                                 // nobody runs faster than his top speed
            vec2 Accel = (Desired - Vel) * (1.0f / kFixedDt);                                                                         // (above it, this brakes him)
            const float MaxAccel = 9.0f;                                                                                               // m/s2
            if (Length(Accel) > MaxAccel) Accel = Normalized(Accel) * MaxAccel;
            const vec2 Push = LimitedPush(Accel * Body.m_Mass, Vel, P.m_Speed);                                                       // (at his top speed: no more push ahead)
            AddForce(Body, Push.x, 0.0f, Push.z, kMaxRunForce);

            // the team that just scored jumps: a push of the legs (about 2 kN, three times his weight) while his feet are on the ground
            const bool bGrounded = T.m_Position.m_Y < P.m_Home.m_Y + 0.12f && Body.m_LinearVelocity.m_Y < 3.0f;
            if (Rules.m_Phase == phase::GOAL && Rules.m_LastScorer == P.m_Team && bGrounded && std::sin(Rules.m_Timer * 5.0f + static_cast<float>(Id.m_Id)) > 0.0f)
                AddForce(Body, 0.0f, 2200.0f, 0.0f, kMaxJumpForce);

            P.m_Velocity = Body.m_LinearVelocity;                                                                                      // (to be seen in the Inspector)
        }

        static constexpr float kKickRange = 0.75f;          // how near the ball must be to be kicked

        // He has the ball at his feet: shoot, pass, or push it forward.
        void DecideKickField(const match& Rules, float Dir, vec2 Pos, vec2 BallPos, const std::vector<details::person>& All, kick& Kick, player& P, const identity& Id) noexcept
        {
            using namespace details;
            const vec2  Goal     = { Dir * Rules.m_HalfLength, 0.0f };
            const float GoalDist = Distance(Pos, Goal);

            vec2  Aim;
            float Speed;

            if (GoalDist < 6.5f)                                                                                                       // close enough: shoot
            {
                const float Side = m_Random.Range(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;                                                   // into a corner, away from the keeper
                Aim   = { Goal.x, Side * (Rules.m_GoalHalfWidth - 0.5f) };
                Speed = P.m_KickSpeed * 1.3f;
            }
            else
            {
                // the best placed team-mate: ahead of him, free of opponents, not too near and not too far
                const person* pBest  = nullptr;
                float         BestScore = -1.0e9f;
                for (const person& O : All)
                {
                    if (O.m_bReferee || O.m_Team != P.m_Team || O.m_Id == Id.m_Id || O.m_Role != role::FIELD) continue;
                    const float D = Distance(O.m_Pos, Pos);
                    if (D < 2.5f || D > 11.0f) continue;
                    const float Forward = (O.m_Pos.x - Pos.x) * Dir;
                    if (Forward < -1.5f) continue;                                                                                      // no passes backward

                    float Free = 1.0e9f;                                                                                                // how far the nearest opponent is from him and from the way
                    for (const person& Q : All)
                    {
                        if (Q.m_bReferee || Q.m_Team == P.m_Team) continue;
                        Free = std::min(Free, std::min(Distance(Q.m_Pos, O.m_Pos), DistanceToSegment(Q.m_Pos, Pos, O.m_Pos) + 0.6f));
                    }
                    if (Free < 1.6f) continue;

                    const float Score = Forward + Free * 0.6f - D * 0.15f;
                    if (Score > BestScore) { BestScore = Score; pBest = &O; }
                }

                if (pBest)                                                                                                              // pass
                {
                    const float D = Distance(pBest->m_Pos, Pos);
                    Aim   = pBest->m_Pos + pBest->m_Vel * 0.5f;
                    Speed = Clamp(D * 1.7f + 3.5f, 6.0f, P.m_KickSpeed);
                }
                else                                                                                                                    // push it ahead
                {
                    Aim   = { Goal.x, Pos.z * 0.5f };
                    Speed = 8.0f;
                }
            }
            Shoot(BallPos, Aim, Speed, Kick, P, Id);
        }

        // The goalkeeper has the ball: a long pass to the team-mate that is the most ahead and free, or a clearance.
        void DecideKickKeeper(const match& Rules, float Dir, vec2 Pos, vec2 BallPos, const std::vector<details::person>& All, kick& Kick, player& P, const identity& Id) noexcept
        {
            using namespace details;
            const person* pBest = nullptr;
            float         BestScore = -1.0e9f;
            for (const person& O : All)
            {
                if (O.m_bReferee || O.m_Team != P.m_Team || O.m_Id == Id.m_Id || O.m_Role != role::FIELD) continue;
                const float D = Distance(O.m_Pos, Pos);
                if (D < 4.0f) continue;
                float Free = 1.0e9f;
                for (const person& Q : All) if (!Q.m_bReferee && Q.m_Team != P.m_Team) Free = std::min(Free, Distance(Q.m_Pos, O.m_Pos));
                const float Score = (O.m_Pos.x - Pos.x) * Dir * 0.5f + Free;
                if (Score > BestScore) { BestScore = Score; pBest = &O; }
            }

            vec2 Aim = { Pos.x + Dir * 9.0f, m_Random.Range(-1.0f, 1.0f) * (Rules.m_HalfWidth - 1.0f) };
            if (pBest) Aim = pBest->m_Pos;
            Shoot(BallPos, Aim, Clamp(Distance(Aim, Pos) * 1.4f + 3.0f, 8.0f, P.m_KickSpeed * 1.1f), Kick, P, Id);
        }

        // Kicks the ball from BallPos toward Aim, with an error that gets smaller with skill.
        void Shoot(vec2 BallPos, vec2 Aim, float Speed, kick& Kick, player& P, const identity& Id) noexcept
        {
            using namespace details;
            if (Kick.m_bValid) return;                                                                                                  // only one kick per frame
            vec2 Direction = Normalized(Aim - BallPos);
            if (Length(Direction) < 0.5f) return;
            Direction = Turned(Direction, m_Random.Range(-0.35f, 0.35f) * (1.0f - Clamp(P.m_Skill, 0.0f, 1.0f)));

            Kick.m_bValid   = true;
            Kick.m_Velocity = Direction * Speed;
            Kick.m_Team     = P.m_Team;
            Kick.m_Id       = Id.m_Id;
            P.m_Cooldown    = 0.45f;
        }
    };
    XSCRIPT_REGISTER_SYSTEM(player_system)
}
