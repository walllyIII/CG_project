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

## Run the week-one graybox

The first playable checkpoint uses the previous course project's CMake, GLFW and compatibility OpenGL approach. Install CMake 3.16 or later, a C++17 compiler and an OpenGL driver supporting a 2.1 compatibility context. CMake uses an installed GLFW package when available and otherwise fetches GLFW 3.4. To build offline with an existing GLFW source tree, add `-DFETCHCONTENT_SOURCE_DIR_GLFW=<path>` at configuration time.

On Windows with MinGW, run these commands from the repository root:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build -j
.\build\wrecklight.exe
```

Swim with WASD, look with the mouse, ascend with Space and descend with Left Ctrl. Press R to restore the discovery view and Esc to exit. To reproduce the starting image directly from the framebuffer, run `./build/wrecklight.exe --capture build/discovery.ppm`.

Press **1**, **2** or **3** to return to the discovery, approach or inspection camera. Free swimming resumes from the selected pose. To reproduce all three images from one continuous OpenGL run:

```powershell
./build/wrecklight.exe --route build/route
```

The replay renders 1,201 frames along the exterior route using a fixed simulation clock of 10 steps per second. It writes three PPM files at simulated times 0, 60 and 120 seconds. The PNGs below were converted from those framebuffers at their original 1280 x 720 resolution. Replay runs as fast as the renderer allows.

### Team review

| Fixed view | Frame | Question for reviewers |
| --- | --- | --- |
| [Discovery](docs/discovery.png) | 0 | Before reading the project description, what do you recognize and where would you swim first? |
| [Approach](docs/approach.png) | 600 | Where does the chain lead your eye? Point to the opening you would approach. |
| [Inspection](docs/inspection.png) | 1200 | What object is visible through the breach? Which parts can you distinguish from outside? |

Record each reviewer's first answer before explaining the intended route. The technical playback has been checked on Intel Arc graphics. Teammate feedback is pending.

The graybox uses flat materials and simple distance fog. See the [four-week execution plan](DEVELOPMENT_PLAN.md) for the later rendering milestones.
