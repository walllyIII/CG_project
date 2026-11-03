# Wrecklight

A first-person underwater exploration scene for CG 100433, Track 2. During a two-minute dive, the player follows a half-buried anchor chain to a leaning pirate wreck, swims alongside its hull and switches on a dive lamp to inspect a cannon through a breach.

**Graphics question.** How can the wreck remain recognizable through blue-green water at a distance while wet wood, exposed iron, rust and sand remain distinguishable up close?

## The dive

| View | Player experience | Visual checkpoint |
| --- | --- | --- |
| Discovery | Start above the anchor and look past the chain toward the broken mast. | The mast and tilted hull form a readable silhouette. |
| Approach | Follow the chain across pale sand to the ship. | Moving caustics cross the sunlit seabed and upper hull. Sand beneath the hull remains in shadow. |
| Inspection | Stop outside the breach, switch on the lamp and look around the cannon. | Bare iron shows a tight highlight, damp planks a broader sheen and rust a rough finish. |

## Rendering plan

Build on the course OpenGL and GLSL camera, mesh-loading and shader framework. Extend the camera with vertical swimming and a spotlight toggle. Save the three viewpoints above for repeatable comparisons.

- **Underwater attenuation.** For distance `d` and RGB extinction coefficients `k`, compute `T = exp(-k * d)` componentwise. Blend the lit surface with the water color using `T`. A larger red coefficient makes warm details fade first.
- **Dynamic caustics.** Project two scrolling texture layers along the sunlight direction. Use surface orientation, depth and the directional shadow map to limit the light pattern to illuminated surfaces. This approximates refracted sunlight using real-time texture projection.
- **Materials and dive lamp.** Give wood, iron, rust and sand distinct base colors, roughness, metalness and normal detail. Attach a spotlight to the camera so the player can inspect surface responses under the same light.

## Scope and delivery

Build one wreck, a small seabed and one exterior route. Prioritize the side facing the diver and the interior visible through the breach. Drifting particles are optional once the core effects pass review. Fish schools, animated seaweed and a traversable cabin are outside the four-week scope.

Deliver a runnable scene, an approximately two-minute video, fixed-view effect comparisons, performance measurements and a report crediting models and textures. The target is **1080p at an average of at least 30 FPS on the presentation computer**.

## Team and project status

徐川 · 张开翔 · 韩赟晨 · 黄宇镐 · 李奕坤

Individual responsibilities await team confirmation. All five members participate in integration reviews and write the project report.

The repository is at the planning stage. Begin with the graybox discovery view. See the [four-week execution plan](DEVELOPMENT_PLAN.md) for milestones and acceptance checks.
