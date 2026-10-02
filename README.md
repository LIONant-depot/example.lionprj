# xresource_pipeline_example_project
An example project for the xresource pipeline

## Soccer: a small game made with the editor

Open the **Soccer** level (`Soccer/Levels/Soccer`) in the Level Editor and press Play: five-a-side, Blue against Red, two goalkeepers and a dark grey referee.
Players pass the ball, shoot, and the team that scores jumps (the referee too). The scoreboard is two bars beside the pitch (Blue and Red) that grow with
every goal. The default camera is too close: orbit out, or `SetCamera -Distance 26 -Pitch -55 -Target 0,0,0` on the pipe (a negative pitch looks from above).

What is in the `Soccer` folder, and why:

| Folder | What it holds |
| --- | --- |
| `Scripts/SoccerGame` | The script module: components (`SoccerMatch`, `SoccerBall`, `SoccerPlayer`, `SoccerReferee`, `SoccerIdentity`, `SoccerShadow`, `SoccerScoreBar`) and one system for each job (players, ball and rules, referee, shadows, scoreboard). The editor compiles it into `Game.dll`; edit a file and press Play, the game rebuilds and reloads. |
| `Prefabs` | `Player Blue`, `Player Red`, `Keeper Blue`, `Keeper Red`, `Referee`, `Ball`, `Shadow`: the pieces the scene is made of. |
| `Materials` | Two physics materials: `Ball Material` (bounces a little) and `Player Material` (slides, so a force moves a player the way it should). |
| `Scenes/Pitch`, `Levels/Soccer` | The pitch, the lines, the goals, the walls, the scoreboard and the teams (folders `Blue Team`, `Red Team`, `Shadows`, `Walls`). |

How it uses the engine:

* **Physics**: the ball is a `PhysicsDynamics` body (0.45 kg) with a sphere collider; the players are dynamic capsules (75 kg, `Constraints/Rotation` locked so they stay
  upright); the floor, the posts and the walls are static colliders. Nothing is moved by hand: a player runs by being pushed (`F = m * a`, at most 9 m/s2 and 700 N,
  never above his top speed) and a kick is one capped force on the ball. The systems that push run in fixed steps of 1/60 s, the same step the physics takes.
* **Connectors**: the Physics system has two connectors, "Before Step" and "After Step", that run once for every fixed step. `Soccer Players` and `Soccer Referee` are connected to "Before Step" (see the System Registry; the data is in `Project.config/SystemOrder.config.txt`), so each physics step is preceded by one step of the people.
* **Game time**: the systems read the time from the game they belong to (`xlioncore::game::From(GameMgr)->m_Time`): the fixed steps that are due (the physics and the people move in them) and the scaled `Dt` that the clock of the match (kickoff, goal pause, celebration) uses. The speed slider next to Play (0.25x to 3x) scales both.
* **Shadows**: flat dark discs under every person and the ball (`SoccerShadow`); the primitive has no transparency yet, so they are the grass colour, darkened.

The module lives in `Descriptors/ScriptModule`; `Project.config/Script.config.txt` lists it. Notes for whoever changes the game are at the top of each `soccer_*.h`.
