#pragma once

// The players: ten of them, five a side, one of them a goalkeeper.
//
// What a field player does, in order:
//   - the one closest to the ball in his team runs at it (he "chases");
//   - the second closest runs ahead along the side, ready for a pass (he "supports");
//   - the others keep their places, leaning toward the ball;
//   - a player that reaches the ball kicks it: a shot when he is close enough to the goal, otherwise a pass to the best placed teammate
//     (ahead of him, with no opponent near him or near the way), otherwise he pushes the ball forward.
// A goalkeeper stays on his goal line following the ball sideways. When the ball comes into his area, not too fast to be caught, and it was not a team-mate that played it, he picks it
// up (the ball goes where his hands go, nobody else can play it), waits a little, and kicks it high toward the middle of the field - when nobody stands in the way close in front of
// him (when somebody does he tries another line, and if none is clear he moves himself, with the ball in his hands, to a place from which the way is clear). While he holds it
// everybody takes position in the middle of the field. A ball that comes too fast he can
// only clear: he kicks it away.
//
// Three moves on top of that (all of them pushes on the bodies within the same force limits as running, jumping and kicking):
//   - a goalkeeper DIVES: a ball flying at his goal that he cannot reach on his feet (too far to the side, or too high) makes him push off with
//     both legs, up and to the side where it is going to cross his line; in the air his hands reach farther and higher - he catches what is slow
//     enough and parries the rest away from the goal;
//   - a field player LOFTS a shot: from farther out, or when an opponent stands in the way, the shot leaves the ground and drops into the goal;
//   - a field player SLIDES: when an opponent has the ball at his feet, a little out of his reach, and he does not come from behind, he slides
//     at it; reaching the ball pokes it away for his team, and either way he needs a moment to get up again.
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

        // The goalkeeper's dive.
        inline constexpr float kDiveMinTime     = 0.12f;    // s: a ball that crosses his line sooner than this is past him already
        inline constexpr float kDiveMaxTime     = 0.9f;     // s: a ball that crosses it later he can walk to
        inline constexpr float kDiveMinBall     = 4.0f;     // m/s toward his goal: a slower ball he plays on his feet
        inline constexpr float kKeeperReach     = 0.6f;     // m to the side he covers standing (arms out)
        inline constexpr float kStandReachY     = 1.6f;     // m: the height his hands reach without jumping
        inline constexpr float kDiveSideMax     = 5.0f;     // m/s sideways his legs can give him
        inline constexpr float kDiveUpMin       = 1.5f;     // m/s up: a low dive still leaves the ground
        inline constexpr float kDiveUpMax       = 3.3f;     // m/s up as his feet leave the ground: about 0.55 m more of flight, on top of kDiveLegs
        inline constexpr float kDiveTakeoff     = 0.2f;     // s his legs push at most
        inline constexpr float kDiveLegs        = 0.45f;    // m: they push until his body is this much over where it stands (the legs are straight)
        inline constexpr float kDiveTime        = 0.9f;     // s the dive lasts, the landing included
        inline constexpr float kDiveCooldown    = 1.5f;     // s on the ground before he may dive again
        inline constexpr float kDiveReach       = 1.3f;     // m: in the air his hands reach this far (on the ground: kCatchRange)
        inline constexpr float kDiveReachY      = 0.9f;     // m above and below his hands
        inline constexpr float kParrySpeed      = 7.0f;     // m/s: a ball too fast to hold is pushed this fast, out toward the side
        inline constexpr float kParryUp         = 2.5f;

        // The slide tackle.
        inline constexpr float kSlideMin        = 0.9f;     // m from the ball: nearer he just kicks it
        inline constexpr float kSlideMax        = 2.4f;     // m: farther he cannot get there sliding
        inline constexpr float kCarrierRange    = 0.9f;     // m: an opponent this near the ball has it at his feet
        inline constexpr float kSlideChance     = 0.05f;    // per fixed step, when he can: he does not always go to ground
        inline constexpr float kSlideTime       = 0.5f;     // s on the ground sliding
        inline constexpr float kSlideRecover    = 0.5f;     // s getting up (he moves slowly)
        inline constexpr float kSlideCooldown   = 2.5f;     // s before he may slide again
        inline constexpr float kSlideReach      = 1.2f;     // m: a sliding leg reaches this far
        inline constexpr float kSlidePoke       = 6.0f;     // m/s the ball leaves a sliding leg

        // Lofted shots.
        inline constexpr float kLoftMinDistance = 4.0f;     // m from the goal: farther out a shot is lofted
        inline constexpr float kLoftMinHeight   = 0.5f;     // m at the goal line: the ball's center, from this ...
        inline constexpr float kLoftUnderBar    = 0.3f;     // ... up to this much under the bar (the ball's radius, 0.21 m, and a margin)
        inline constexpr float kOverTheBar      = 0.35f;    // m: a ball whose center crosses this much over the bar goes over it (bar 0.15 m thick, ball 0.21 m)
        inline constexpr float kMaxLoftUp       = 6.0f;     // m/s up at most

        // A dive: the sideways (z) and upward speeds his legs must give him to meet a ball that crosses his line as C says. False when it is not worth diving: it misses the goal,
        // it comes too soon or too late, or he gets there on his feet and it is not over his head.
        struct dive { float m_Side = 0.0f; float m_Up = 0.0f; };
        inline bool PlanDive(const crossing& C, float KeeperZ, float GoalHalfWidth, float GoalHeight, float WalkSpeed, dive& Out) noexcept
        {
            if (C.m_Time < kDiveMinTime || C.m_Time > kDiveMaxTime) return false;
            if (std::fabs(C.m_Z) > GoalHalfWidth + 0.3f) return false;                                     // wide of the post: let it go
            if (C.m_Y > GoalHeight + kOverTheBar) return false;                                            // over the bar: let it go
            const float Gap   = C.m_Z - KeeperZ;
            const bool  bHigh = C.m_Y > kStandReachY;
            if (std::fabs(Gap) <= WalkSpeed * C.m_Time * 0.5f + kKeeperReach && !bHigh) return false;    // he gets there on his feet
            Out.m_Side = Clamp(Gap / std::max(C.m_Time, 0.15f), -kDiveSideMax, kDiveSideMax);
            Out.m_Up   = bHigh ? Clamp(std::sqrt(2.0f * kGravity * std::max(0.0f, C.m_Y - kStandReachY - kDiveLegs)), kDiveUpMin, kDiveUpMax) : kDiveUpMin;   // (his legs straighten first)
            return true;
        }

        // Can he slide at the ball an opponent (at CarrierPos, moving CarrierVel) has at his feet? He is a little too far to kick it but close enough to slide, and he does not come from
        // behind the carrier (a tackle from behind is a foul).
        inline bool CanSlide(vec2 Pos, vec2 BallPos, vec2 CarrierPos, vec2 CarrierVel) noexcept
        {
            const float D = Distance(Pos, BallPos);
            if (D < kSlideMin || D > kSlideMax) return false;
            if (Distance(CarrierPos, BallPos) > kCarrierRange) return false;
            const vec2 Heading = Normalized(CarrierVel);
            if (Length(Heading) > 0.5f && Dot(Normalized(Pos - CarrierPos), Heading) < -0.5f) return false;
            return true;
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

        random      m_Random;

        // What happened during this fixed step (dives, lofted shots, slide tackles that took the ball): added to the match's counters at its end.
        struct events { int m_Dives = 0; int m_Lofts = 0; int m_Tackles = 0; };
        events      m_Events;

        // What was decided for the ball this frame, applied to it once everybody has been visited: a kick (m_Up: a kick that lifts it), or a goalkeeper holding it (it goes to m_HoldAt).
        struct kick
        {
            bool         m_bValid = false;
            vec2         m_Velocity;
            float        m_Up     = 0.0f;
            team         m_Team   = team::BLUE;
            int          m_Id     = -1;
            bool         m_bHold  = false;
            xmath::fvec3 m_HoldAt = xmath::fvec3::fromZero();
        };

        // The people move with the physics: this system needs a place that runs it once for each fixed step (every run is one fixed delta time long), so it takes ONE step each time it
        // runs and never counts the steps itself. Placed in the Before Step connector of the Physics system the people push on the bodies and the step takes it (how the example is set up).
        using constraints = std::tuple<xlioncore::constraint::fixed_delta_time>;

        void OnUpdate(void) noexcept { FixedStep(); }

        void FixedStep(void) noexcept
        {
            using namespace details;
            const float Dt = kFixedDt;

            //
            // What everybody knows: the rules, the ball, and where every person is.
            //
            match Rules;
            if (!ReadMatch(*this, Rules)) return;
            m_Events = {};

            vec2 BallPos, BallVel;
            float BallY = 0.0f, BallVelY = 0.0f;
            ball  Ball;
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, ball, xlioncore::physics::physics_dynamics>();
                Foreach(Search(Query), [&](const xecs::component::entity&, const xlioncore::transform& T, const ball& B, const xlioncore::physics::physics_dynamics& Body) noexcept
                {
                    BallPos = Flat(T.m_Position); BallVel = Flat(Body.m_LinearVelocity); BallY = T.m_Position.m_Y; BallVelY = Body.m_LinearVelocity.m_Y; Ball = B;
                });
            }

            std::vector<person> All;
            All.reserve(24);
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, player, identity, xlioncore::physics::physics_dynamics>();
                Foreach(Search(Query), [&](const xecs::component::entity&, const xlioncore::transform& T, const player& P, const identity& Id, const xlioncore::physics::physics_dynamics& Body) noexcept
                {
                    All.push_back({ Id.m_Id, P.m_Team, P.m_Role, false, Flat(T.m_Position), Flat(Body.m_LinearVelocity) });
                });
            }
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, referee, identity, xlioncore::physics::physics_dynamics>();
                Foreach(Search(Query), [&](const xecs::component::entity&, const xlioncore::transform& T, const identity& Id, const xlioncore::physics::physics_dynamics& Body) noexcept
                {
                    All.push_back({ Id.m_Id, team::BLUE, role::FIELD, true, Flat(T.m_Position), Flat(Body.m_LinearVelocity) });
                });
            }

            //
            // Everybody decides and moves
            //
            kick Kick;
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<xlioncore::transform, player, identity, xlioncore::physics::physics_dynamics>();
                Foreach(Search(Query), [&](const xecs::component::entity& Entity, xlioncore::transform& T, player& P, const identity& Id, xlioncore::physics::physics_dynamics& Body) noexcept
                {
                    Act(Dt, Rules, BallPos, BallVel, BallY, BallVelY, Ball, All, Kick, Entity, T, P, Id, Body);
                });
            }

            //
            // The kick, if there was one
            //
            if (Kick.m_bValid || Kick.m_bHold)
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<ball, xlioncore::physics::physics_dynamics>();
                Foreach(Search(Query), [&](const xecs::component::entity& Entity, ball& B, xlioncore::physics::physics_dynamics& Body) noexcept
                {
                    B.m_LastTouch  = Kick.m_Team;
                    B.m_LastKicker = Kick.m_Id;
                    if (Kick.m_bHold)                                                                    // in the hands of a goalkeeper: where his hands are, standing still
                    {
                        B.m_bHeld = true;
                        xlioncore::physics::TeleportDynamicBody(getGameMgr(), Entity, Kick.m_HoldAt, xmath::fquat::fromIdentity());
                        return;
                    }
                    B.m_bHeld = false;
                    const xmath::fvec3 F = ForceFor(Body, Kick.m_Velocity.x, Kick.m_Velocity.z, Kick.m_Up);   // the one push that makes the speed asked for
                    AddForce(Body, F.m_X, F.m_Y, F.m_Z, kMaxKickForce);
                });
            }

            //
            // What happened, for the match's counters
            //
            if (m_Events.m_Dives || m_Events.m_Lofts || m_Events.m_Tackles)
            {
                xecs::query::instance Query;
                Query.m_Must.AddFromComponents<match>();
                Foreach(Search(Query), [&](const xecs::component::entity&, match& M) noexcept
                {
                    M.m_Dives   += m_Events.m_Dives;
                    M.m_Lofts   += m_Events.m_Lofts;
                    M.m_Tackles += m_Events.m_Tackles;
                });
            }
        }

        // One player, one frame.
        void Act(float Dt, const match& Rules, vec2 BallPos, vec2 BallVel, float BallY, float BallVelY, const ball& Ball, const std::vector<details::person>& All, kick& Kick
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
                P.m_bHolding = false;
                P.m_HoldTime = 0.0f;
                P.m_Action   = action::NONE;
                P.m_ActionTime = 0.0f;
                P.m_ActionCooldown = 0.0f;
                xlioncore::physics::TeleportDynamicBody(getGameMgr(), Entity, xmath::fvec3(Home.x, T.m_Position.m_Y, Home.z), xmath::fquat::fromIdentity());
                return;
            }
            P.m_Cooldown = std::max(0.0f, P.m_Cooldown - Dt);
            P.m_ActionCooldown = std::max(0.0f, P.m_ActionCooldown - Dt);
            const bool bGrounded = T.m_Position.m_Y < P.m_Home.m_Y + 0.12f && Body.m_LinearVelocity.m_Y < 3.0f;

            // A dive or a slide runs its time out (and ends at once when the play stops).
            if (P.m_Action != action::NONE)
            {
                P.m_ActionTime -= Dt;
                if (P.m_ActionTime <= 0.0f || Rules.m_Phase != phase::PLAYING)
                {
                    P.m_ActionCooldown = P.m_Action == action::DIVE ? kDiveCooldown : kSlideCooldown;
                    P.m_Action         = action::NONE;
                    P.m_ActionTime     = 0.0f;
                }
            }

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

                    if (Ball.m_bHeld)   Target = { Dir * kMiddleAhead, Home.z * 0.7f };                                                              // a goalkeeper has the ball: everybody takes position in the middle of the field
                    else if (bWaiting)  Target = Home;
                    else if (Rank == 0) Target = StandOff(Pos, BallPos + BallVel * 0.25f);                                                            // chase
                    else if (Rank == 1) Target = BallPos + vec2{ Dir * 3.5f, (Pos.z >= BallPos.z ? 1.0f : -1.0f) * 3.0f };            // support: ahead and to the side
                    else                                                                                                               // keep the place, lean to the ball
                    {
                        Target = Home + (BallPos - Home) * 0.3f;
                        if (BallPos.x * Dir > 0.0f) Target.x += Dir * 1.5f;                                                            // the ball is in their half: move up
                    }

                    // A slide tackle: the opponent nearest the ball has it at his feet, a little out of this player's reach (he is the one chasing it)
                    if (P.m_Action == action::NONE && P.m_ActionCooldown <= 0.0f && Rank == 0 && !bWaiting && !Ball.m_bHeld && bGrounded && BallY < kKickHeight)
                    {
                        const person* pCarrier = nullptr;
                        for (const person& O : All)
                            if (!O.m_bReferee && (pCarrier == nullptr || Distance(O.m_Pos, BallPos) < Distance(pCarrier->m_Pos, BallPos))) pCarrier = &O;
                        if (pCarrier && pCarrier->m_Team != P.m_Team && CanSlide(Pos, BallPos, pCarrier->m_Pos, pCarrier->m_Vel) && m_Random.Unit() < kSlideChance)
                        {
                            const vec2 Way = Normalized((BallPos + BallVel * 0.2f) - Pos);
                            P.m_Action     = action::SLIDE;
                            P.m_ActionTime = kSlideTime + kSlideRecover;
                            P.m_ActionX    = Way.x;
                            P.m_ActionZ    = Way.z;
                        }
                    }
                    const bool bSliding = P.m_Action == action::SLIDE && P.m_ActionTime > kSlideRecover;
                    if (bSliding && !Kick.m_bValid && !Kick.m_bHold && !Ball.m_bHeld && BallDist < kSlideReach && BallY < kKickHeight * 0.6f && P.m_Cooldown <= 0.0f)
                    {
                        // the leg reaches the ball: it goes on the way he slides, turned a little toward the goal his team attacks
                        const vec2 Way = Normalized(vec2{ P.m_ActionX, P.m_ActionZ } + vec2{ Dir * 0.6f, 0.0f });
                        Kick.m_bValid   = true;
                        Kick.m_Velocity = Way * kSlidePoke;
                        Kick.m_Up       = 0.0f;
                        Kick.m_Team     = P.m_Team;
                        Kick.m_Id       = Id.m_Id;
                        P.m_Cooldown    = 0.45f;
                        P.m_ActionTime  = std::min(P.m_ActionTime, kSlideRecover);                                                    // the slide is over: he gets up
                        ++m_Events.m_Tackles;
                    }
                    else if (P.m_Action == action::NONE && !bWaiting && !Ball.m_bHeld && BallDist < kKickRange && BallY < kKickHeight && P.m_Cooldown <= 0.0f) DecideKickField(Rules, Dir, Pos, BallPos, All, Kick, P, Id);
                }
                else                                                                                                                   // goalkeeper
                {
                    if (P.m_bHolding)                                                                                                  // he has the ball: he waits, then kicks it high to the middle
                    {
                        Target = Pos;
                        Kick.m_bHold = true;
                        Kick.m_HoldAt = xmath::fvec3(Pos.x + Dir * kHandsAhead, T.m_Position.m_Y + kHandsHeight, Pos.z);
                        Kick.m_Team = P.m_Team;
                        Kick.m_Id   = Id.m_Id;
                        P.m_HoldTime -= Dt;
                        if (P.m_HoldTime <= 0.0f) ThrowToTheMiddle(Rules, Pos, BallPos, All, Kick, P, Id, Target);
                    }
                    else if (P.m_Action == action::DIVE)                                                                                // in the air: his hands reach farther and higher
                    {
                        Target = Pos;
                        const float HandsY   = T.m_Position.m_Y + kHandsHeight;
                        const bool  bPassBack = Ball.m_LastTouch == P.m_Team && Ball.m_LastKicker >= 0;
                        if (!bPassBack && !Ball.m_bHeld && !Kick.m_bValid && !Kick.m_bHold && P.m_Cooldown <= 0.0f && BallDist < kDiveReach && std::fabs(BallY - HandsY) < kDiveReachY)
                        {
                            const float Speed = std::sqrt(Dot(BallVel, BallVel) + BallVelY * BallVelY);
                            Kick.m_Team = P.m_Team;
                            Kick.m_Id   = Id.m_Id;
                            if (Speed < kCatchSpeed)                                                                                    // slow enough: he holds it
                            {
                                P.m_bHolding = true;
                                P.m_HoldTime = kHoldSeconds;
                                Kick.m_bHold = true;
                                Kick.m_HoldAt = xmath::fvec3(Pos.x + Dir * kHandsAhead, HandsY, Pos.z);
                            }
                            else                                                                                                        // he pushes it away: out of the goal, to the side it was going
                            {
                                const float Side = BallPos.z >= 0.0f ? 1.0f : -1.0f;
                                Kick.m_bValid   = true;
                                Kick.m_Velocity = Normalized(vec2{ Dir, Side }) * kParrySpeed;
                                Kick.m_Up       = kParryUp;
                                P.m_Cooldown    = 0.6f;
                            }
                        }
                    }
                    else
                    {
                        // A ball flying at his goal that he cannot reach on his feet: he dives (up, left or right) to meet it where it crosses his line
                        if (P.m_ActionCooldown <= 0.0f && bGrounded && !Ball.m_bHeld && BallVel.x * Dir < -kDiveMinBall && !(Ball.m_LastTouch == P.m_Team && Ball.m_LastKicker >= 0))
                        {
                            dive Plan;
                            if (PlanDive(CrossingAt(Pos.x, BallPos, BallY, BallVel, BallVelY), Pos.z, Rules.m_GoalHalfWidth, Rules.m_GoalHeight, P.m_Speed, Plan))
                            {
                                P.m_Action     = action::DIVE;
                                P.m_ActionTime = kDiveTime;
                                P.m_ActionX    = 0.0f;
                                P.m_ActionZ    = Plan.m_Side;
                                P.m_ActionUp   = Plan.m_Up;
                                ++m_Events.m_Dives;
                            }
                        }

                        const float Reach = Rules.m_GoalHalfWidth - 0.3f;
                        Target = { Home.x, Clamp(BallPos.z * 0.55f, -Reach, Reach) };
                        const bool bComing = BallVel.x * Dir < 0.2f;                                                                   // not moving away from his goal
                        if (Distance(BallPos, Home) < 4.5f && bComing) Target = StandOff(Pos, BallPos);                                               // rush out

                        if (P.m_Action == action::DIVE) Target = Pos;                                                                  // (he just took off)
                        else if (CanCatch(Home, Pos, BallPos, BallVel, BallY, Ball, P))                                               // the right time: pick it up
                        {
                            P.m_bHolding = true;
                            P.m_HoldTime = kHoldSeconds;
                            Kick.m_bHold = true;
                            Kick.m_HoldAt = xmath::fvec3(Pos.x + Dir * kHandsAhead, T.m_Position.m_Y + kHandsHeight, Pos.z);
                            Kick.m_Team = P.m_Team;
                            Kick.m_Id   = Id.m_Id;
                        }
                        else if (BallDist < kKickRange && BallY < kCatchHeight && P.m_Cooldown <= 0.0f) DecideKickKeeper(Rules, Dir, Pos, BallPos, All, Kick, P, Id);   // too fast to catch: clear it
                    }
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
            if (P.m_Action == action::SLIDE)                                                                                           // sliding: straight on, at his top speed; then getting up, slowly
                Desired = P.m_ActionTime > kSlideRecover ? vec2{ P.m_ActionX, P.m_ActionZ } * P.m_Speed : Desired * 0.25f;
            if (Length(Desired) > P.m_Speed) Desired = Normalized(Desired) * P.m_Speed;                                                 // nobody runs faster than his top speed

            if (P.m_Action == action::DIVE)
            {
                // the dive: while his legs are still on the ground (the first kDiveTakeoff seconds, until they are straight) they push him toward the speeds
                // asked for - up (his weight carried as well), to the side, and none along x (he stays on his line); in the air nothing pushes, the physics flies him
                if (T.m_Position.m_Y < P.m_Home.m_Y + kDiveLegs && P.m_ActionTime > kDiveTime - kDiveTakeoff)
                {
                    const float Fx = Body.m_Mass * (0.0f            - Body.m_LinearVelocity.m_X) / kFixedDt;
                    const float Fy = Body.m_Mass * (P.m_ActionUp   - Body.m_LinearVelocity.m_Y) / kFixedDt + Body.m_Mass * kGravity;
                    const float Fz = Body.m_Mass * (P.m_ActionZ    - Body.m_LinearVelocity.m_Z) / kFixedDt;
                    AddForce(Body, Fx, std::max(0.0f, Fy), Fz, kMaxJumpForce);
                }
            }
            else
            {
                vec2 Accel = (Desired - Vel) * (1.0f / kFixedDt);                                                                     // (above it, this brakes him)
                const float MaxAccel = 9.0f;                                                                                           // m/s2
                if (Length(Accel) > MaxAccel) Accel = Normalized(Accel) * MaxAccel;
                const vec2 Push = LimitedPush(Accel * Body.m_Mass, Vel, P.m_Speed);                                                   // (at his top speed: no more push ahead)
                AddForce(Body, Push.x, 0.0f, Push.z, kMaxRunForce);
            }

            // the team that just scored jumps: a push of the legs (about 2 kN, three times his weight) while his feet are on the ground
            if (Rules.m_Phase == phase::GOAL && Rules.m_LastScorer == P.m_Team && bGrounded && std::sin(Rules.m_Timer * 5.0f + static_cast<float>(Id.m_Id)) > 0.0f)
                AddForce(Body, 0.0f, 2200.0f, 0.0f, kMaxJumpForce);

        }

        static constexpr float kKickRange = 0.75f;          // how near the ball must be to be kicked (on the ground: BallDist is the distance in x and z)
        static constexpr float kKickHeight = 0.9f;          // m: how high the ball may be to be kicked with the foot (a ball flying over a player's head is nobody's to kick)

        // The goalkeeper's hands.
        static constexpr float kCatchRange   = 0.85f;       // how near the ball must be to be caught
        static constexpr float kCatchArea    = 5.0f;        // how near his goal (the middle of his goal line) the ball must be: the area he plays in
        static constexpr float kCatchSpeed   = 9.0f;        // m/s: a faster ball he can only push away
        static constexpr float kCatchHeight  = 1.8f;        // m: a ball over his hands is out of reach
        static constexpr float kHoldSeconds  = 1.2f;        // how long he keeps it before he kicks it
        static constexpr float kHandsAhead   = 0.55f;       // m in front of him, at the height below
        static constexpr float kHandsHeight  = 0.15f;       // m over the middle of his body

        // Is it the right time to pick the ball up? It is in his area and within his reach, moving slowly enough to be caught, not high over his head, and not coming from a team-mate (a pass
        // back is not picked up) - and he is not just done kicking.
        bool CanCatch(vec2 Home, vec2 Pos, vec2 BallPos, vec2 BallVel, float BallY, const ball& Ball, const player& P) const noexcept
        {
            using namespace details;
            if (P.m_Cooldown > 0.0f || Ball.m_bHeld) return false;
            if (Ball.m_LastTouch == P.m_Team && Ball.m_LastKicker >= 0) return false;
            if (Distance(BallPos, Home) > kCatchArea || Distance(BallPos, Pos) > kCatchRange) return false;
            return Length(BallVel) < kCatchSpeed && BallY < kCatchHeight;
        }

        // Where the players of both teams stand while the goalkeeper holds the ball: the middle of the field, a little toward the goal they attack.
        static constexpr float kMiddleAhead  = 1.0f;

        // The way out of his hands: nobody may stand close in front of him (the ball leaves low: a person there is a wall; farther on it is over everybody's head).
        static constexpr float kClearDistance = 3.0f;       // m in front of him
        static constexpr float kClearWidth    = 1.0f;       // m to each side of the line
        static constexpr float kMaxExtraWait  = 3.0f;       // s he looks for a clear way (he moves himself to find it), then he kicks anyway
        static constexpr float kSideStep      = 1.5f;       // m he steps aside to find it

        // Can the ball leave the hands of From along the line to Aim: is nobody standing in the way close in front?
        static bool IsClear(vec2 From, vec2 Aim, const std::vector<details::person>& All, int SelfId) noexcept
        {
            using namespace details;
            const vec2 Direction = Normalized(Aim - From);
            for (const person& O : All)
            {
                if (O.m_Id == SelfId) continue;
                const vec2  R     = O.m_Pos - From;
                const float Along = Dot(R, Direction);
                if (Along < 0.0f || Along > kClearDistance) continue;
                if (Distance(R, Direction * Along) < kClearWidth) return false;
            }
            return true;
        }

        // The goalkeeper kicks the ball it holds high toward the middle of the field: the speed that lands it there when it leaves at 45 degrees (D = v * v / g), with the error of his skill.
        // First he checks that nobody is in the way close in front of him. If somebody is, he tries the lines to the left and to the right of the middle; if none is clear he moves himself
        // (Target: the ball goes where his hands go) to the side from which the way is clear, and kicks from there. If the way never opens he kicks it to the middle after kMaxExtraWait.
        void ThrowToTheMiddle(const match& Rules, vec2 Pos, vec2 BallPos, const std::vector<details::person>& All, kick& Kick, player& P, const identity& Id, vec2& Target) noexcept
        {
            using namespace details;

            const float Side = Rules.m_HalfWidth - 1.0f;
            const float Lines[] = { 0.0f, 3.0f, -3.0f, 5.5f, -5.5f };
            vec2 Aim = {};
            bool bFound = false;
            for (const float Z : Lines)
            {
                const vec2 Candidate = { 0.0f, Clamp(Z, -Side, Side) };
                if (!IsClear(BallPos, Candidate, All, Id.m_Id)) continue;
                Aim = Candidate; bFound = true; break;
            }
            if (!bFound && P.m_HoldTime > -kMaxExtraWait)
            {
                // Somebody is in the way on every line: he steps aside, along his line, to where a line is clear (he stays in front of his goal).
                const float Reach = Rules.m_GoalHalfWidth + 0.5f;
                for (const float Step : { kSideStep, -kSideStep })
                {
                    const vec2 Spot = { Pos.x, Clamp(Pos.z + Step, -Reach, Reach) };
                    if (IsClear(BallPos + (Spot - Pos), vec2{}, All, Id.m_Id)) { Target = Spot; return; }
                }

                // No place is clear yet: away from the person that is the most in the way.
                float Nearest = 1.0e9f; float Away = 1.0f;
                const vec2 Direction = Normalized(vec2{} - BallPos);
                for (const person& O : All)
                {
                    if (O.m_Id == Id.m_Id) continue;
                    const vec2  R     = O.m_Pos - BallPos;
                    const float Along = Dot(R, Direction);
                    if (Along < 0.0f || Along > kClearDistance || Along >= Nearest || Distance(R, Direction * Along) >= kClearWidth) continue;
                    Nearest = Along; Away = Pos.z >= O.m_Pos.z ? 1.0f : -1.0f;
                }
                Target = { Pos.x, Clamp(Pos.z + Away * kSideStep, -Reach, Reach) };
                return;
            }

            const float D = Distance(BallPos, Aim);
            if (D < 1.0e-3f) return;
            const float Along  = std::sqrt(D * kGravity * 0.5f);                                // the speed along the ground, and the speed up: the same at 45 degrees
            vec2 Direction = Normalized(Aim - BallPos);
            Direction = Turned(Direction, m_Random.Range(-0.2f, 0.2f) * (1.0f - Clamp(P.m_Skill, 0.0f, 1.0f)));

            Kick.m_bHold    = false;
            Kick.m_bValid   = true;
            Kick.m_Velocity = Direction * Along * 1.5f;
            Kick.m_Up       = Along;
            Kick.m_Team     = P.m_Team;
            Kick.m_Id       = Id.m_Id;
            P.m_bHolding    = false;
            P.m_HoldTime    = 0.0f;
            P.m_Cooldown    = 1.0f;                                                              // (he does not pick up what he just kicked)
        }

        // He has the ball at his feet: shoot, pass, or push it forward.
        void DecideKickField(const match& Rules, float Dir, vec2 Pos, vec2 BallPos, const std::vector<details::person>& All, kick& Kick, player& P, const identity& Id) noexcept
        {
            using namespace details;
            const vec2  Goal     = { Dir * Rules.m_HalfLength, 0.0f };
            const float GoalDist = Distance(Pos, Goal);

            vec2  Aim;
            float Speed;
            float Up = 0.0f;

            if (GoalDist < 6.5f)                                                                                                       // close enough: shoot
            {
                const float Side = m_Random.Range(0.0f, 1.0f) < 0.5f ? -1.0f : 1.0f;                                                   // into a corner, away from the keeper
                Aim   = { Goal.x, Side * (Rules.m_GoalHalfWidth - 0.5f) };
                Speed = P.m_KickSpeed * 1.3f;

                // from farther out, or with an opponent standing in the way, the shot leaves the ground and drops under the bar
                bool bBlocked = false;
                for (const person& Q : All)
                    if (!Q.m_bReferee && Q.m_Team != P.m_Team && DistanceToSegment(Q.m_Pos, Pos, Aim) < 0.8f && Distance(Q.m_Pos, Pos) > 0.5f) bBlocked = true;
                if (GoalDist > kLoftMinDistance || bBlocked)
                    Up = LoftUp(Distance(BallPos, Aim), Speed, m_Random.Range(kLoftMinHeight, std::max(kLoftMinHeight, Rules.m_GoalHeight - kLoftUnderBar)), kMaxLoftUp);
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
            Shoot(BallPos, Aim, Speed, Kick, P, Id, Up);
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

        // Kicks the ball from BallPos toward Aim, with an error that gets smaller with skill (Up: m/s up, a lofted shot).
        void Shoot(vec2 BallPos, vec2 Aim, float Speed, kick& Kick, player& P, const identity& Id, float Up = 0.0f) noexcept
        {
            using namespace details;
            if (Kick.m_bValid) return;                                                                                                  // only one kick per frame
            vec2 Direction = Normalized(Aim - BallPos);
            if (Length(Direction) < 0.5f) return;
            Direction = Turned(Direction, m_Random.Range(-0.35f, 0.35f) * (1.0f - Clamp(P.m_Skill, 0.0f, 1.0f)));

            Kick.m_bValid   = true;
            Kick.m_Velocity = Direction * Speed;
            Kick.m_Up       = Up;
            Kick.m_Team     = P.m_Team;
            Kick.m_Id       = Id.m_Id;
            P.m_Cooldown    = 0.45f;
            if (Up > 0.0f) ++m_Events.m_Lofts;
        }
    };
    XSCRIPT_REGISTER_SYSTEM(player_system)
}
