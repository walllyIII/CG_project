# Four-week development plan

## First session

1. Confirm the course starter project, supported OpenGL/GLSL versions and build environment. Build the starter on the development machine and record the exact commands.
2. Agree on scene scale, world up, sun direction and the ship's tilt. Block out a seabed, leaning hull, broken mast and foreground anchor. Frame the discovery view before producing detailed assets.
3. Review that view together. Can someone unfamiliar with the layout identify the wreck and point toward the side they would approach? Adjust silhouette, spacing and camera angle until the route reads.
4. Add the anchor chain, hull opening and a placeholder cannon. Save camera positions and orientations for discovery, approach and inspection. Confirm individual owners at the team meeting.

## Milestones

| Week | Work and dependencies | Review evidence |
| --- | --- | --- |
| 1 · Route and baseline | Integrate free-look and vertical swimming. Walk the graybox route, check the breach sightline and record a baseline on the agreed hardware. | Playable graybox, three saved-view captures and baseline timing. The start reveals the wreck and the end reveals the cannon. |
| 2 · Rendering | Integrate RGB attenuation, directional shadows with projected caustics, and PBR with the dive lamp. Test them on shared sand and plank geometry. Provide independent attenuation, caustic and lamp switches, plus a shadow switch for profiling. | Paired captures from fixed views. Verify the checks below before replacing the graybox geometry. |
| 3 · Assets and tuning | Replace the route-facing hull and seabed. Detail the chain, breach, muzzle and planks. Tune water density and light range using the saved views. | First complete dive recording. Review distant silhouette, near-surface detail and camera clearance along the route. |
| 4 · Validation and delivery | Test the runnable build on the presentation computer. Fix clipping and light leaking. Capture the final dive, repeat the timing runs and compile the report. | Runnable program with launch instructions, two-minute video, comparison captures, performance log and asset credits. |

## Repeatable acceptance checks

| Check | Procedure | Pass condition |
| --- | --- | --- |
| Water attenuation | Hold light and plank material constant. Capture the same plank at near, middle and far distances. Repeat the far view with attenuation disabled. | Warm details and contrast decrease with distance while the wreck remains identifiable from the start. |
| Caustics and occlusion | Lock the approach camera so sunlit and hull-shadowed sand share the frame. Capture two times with caustics on and a matching frame with them off. | Bright lines move across exposed sand without lighting the seabed shaded by the hull. |
| Lamp and materials | At the breach, take lamp-off and lamp-on captures without moving. With the lamp on, turn slightly while keeping the cannon and adjacent planks visible. | The cannon becomes legible, the exposed metal highlight shifts with view, and rust and wet wood retain distinct finishes. |
| Performance | Replay the same route at 1920 x 1080. Record GPU, driver, build, resolution and VSync setting. Compare shadows and caustics both off, shadows only, and both on. Keep the remaining settings fixed. | For each run record average FPS (frames / elapsed seconds) and maximum frame time in ms. With all core effects on, target at least 30 average FPS. |

## Final review

All five members review the same route each week and contribute to the report. Show the uninterrupted dive first, then return to the saved views for effect toggles. Explain how shadow masking limits the caustic approximation, and report the measured frame costs. For each external asset, record its creator, source, license and where it appears. If time runs short, prioritize the three route checkpoints before optional animation.
