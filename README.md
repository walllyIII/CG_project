# Wrecklight

A first-person night dive for CG 100433, Track 2. Start with a hand torch above a half-buried anchor. Follow the chain past lost cargo, find the skull and crossed bones on the hull, then shine the torch through the breach to inspect the cannon.

[Watch the current dive and gun-room inspection](docs/gun-room/dive.mp4) · [Compare the three views and the torch-off frame](docs/night-dive/review.png)

The torch starts on. Move the mouse to sweep it over the wreck or press **F** to extinguish it. The light originates below and to the right of the diver's eyes. Its strength falls with distance, fades between 8 and 14 scene units, and is blocked by the wreck and cargo. Dim blue-green ambient light preserves a distant silhouette. Drifting silt and water backscatter reveal the beam in front of the diver.

| View | What to look for |
| --- | --- |
| Discovery | The light catches the anchor, a slatted crate, a barrel and loose timber. Explore either side of the chain. |
| Approach | Sweep from the spilled cannonballs up to the raised skull and crossed bones. The damaged stern gallery and torn pennant change the ship's silhouette. |
| Inspection | Turn across the breach. Broken boards throw shadows onto the deck and barrel. Rust patches, wood grooves and sand ripples respond differently as the beam moves. |

## Inside the broken gunport

Press **3** for the muzzle. Press **4** to look along the barrel at the frayed breeching rope, its barnacled ring and the overturned shot rack. The loose shot collects on the lower, far side of the deck. Press **5** for the crushed locker. A fallen beam pins the lid over a tin cup and ceramic shards. Sweep the torch over the slanted silver coins caught in the seams. Press **6** to look up at the torn hammock and unlit lantern. The canvas and lantern move slowly around their supports, and the torch shadows follow them.

All four viewpoints stay on the water side of the broken hull. The final forty seconds of the replay visit the three details and return to the muzzle. [See the four inspection frames](docs/gun-room/review.png).

## Rendering plan

Build on the course OpenGL and GLSL camera, mesh-loading and shader framework. Extend the camera with vertical swimming and a spotlight toggle. Save the three viewpoints above for repeatable comparisons.

- **Underwater attenuation.** For distance `d` and RGB extinction coefficients `k`, compute `T = exp(-k * d)` componentwise. Blend the lit surface with the water color using `T`. A larger red coefficient makes warm details fade first.
- **Dynamic caustics.** Project two scrolling texture layers along the sunlight direction. Use surface orientation, depth and the directional shadow map to limit the light pattern to illuminated surfaces. This approximates refracted sunlight using real-time texture projection.
- **Materials and dive lamp.** Give wood, iron, rust and sand distinct base colors, roughness, metalness and normal detail. Attach a spotlight to the camera so the player can inspect surface responses under the same light.

## Scope and delivery

Build one wreck, a small seabed and one exterior route. Prioritize the side facing the diver and the interior visible through the breach. Fish schools, animated seaweed and a traversable cabin are outside the four-week scope.

Deliver a runnable scene, an approximately two-minute video, fixed-view effect comparisons, performance measurements and a report crediting models and textures. The target is **1080p at an average of at least 30 FPS on the presentation computer**.

## Team and project status

徐川 · 张开翔 · 韩赟晨 · 黄宇镐 · 李奕坤

Individual responsibilities await team confirmation. All five members participate in integration reviews and write the project report.

## Run the underwater scene

The renderer uses the previous course project's CMake, GLFW and compatibility OpenGL approach. Install CMake 3.16 or later, a C++17 compiler and an OpenGL driver supporting a 2.1 compatibility context with framebuffer objects. CMake uses an installed GLFW package when available and otherwise fetches GLFW 3.4. To build offline with an existing GLFW source tree, add `-DFETCHCONTENT_SOURCE_DIR_GLFW=<path>` at configuration time.

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

### Original graybox team review

| Fixed view | Frame | Question for reviewers |
| --- | --- | --- |
| [Discovery](docs/discovery.png) | 0 | Before reading the project description, what do you recognize and where would you swim first? |
| [Approach](docs/approach.png) | 600 | Where does the chain lead your eye? Point to the opening you would approach. |
| [Inspection](docs/inspection.png) | 1200 | What object is visible through the breach? Which parts can you distinguish from outside? |

Record each reviewer's first answer before explaining the intended route. The technical playback has been checked on Intel Arc graphics. Teammate feedback is pending.

The images above are the original week-one layout references. The current renderer adds RGB attenuation, moving projected caustics with sun shadows, and a metallic-roughness material model illuminated by a camera-mounted spotlight.

## Week-two lighting review

Toggle **V** for water attenuation, **C** for caustics, **H** for sun shadows and **F** for the dive lamp. These archived comparisons show the earlier sunlit build. In the current night dive the torch is on from the start. The static hull and sunlight share a 1024-square directional depth map, sampled with a 3 x 3 visibility filter. Two scrolling layers sample a procedurally generated periodic caustic texture. Material response uses a GGX specular term, distinct roughness and metalness values, procedural surface relief and rust patches.

Run `./build/wrecklight.exe --review build/review` to capture the 1280 x 720 PPM comparison files with fixed lighting, cameras and animation times. The [effect comparison sheet](docs/week2/comparisons.png) shows the main toggles.

| Check | Evidence and observation |
| --- | --- |
| Water | The same plank shifts from brown toward the water color in the [near](docs/week2/plank-near.png), [middle](docs/week2/plank-middle.png) and [far](docs/week2/plank-far.png) views. Mean 8-bit RGB in the central 11 x 11 pixels was (63, 60, 49), (59, 73, 65) and (56, 83, 79). |
| Caustics | Compare [4 seconds](docs/week2/mid-lit.png) with [8 seconds](docs/week2/mid-later.png). The sand pattern moves and the hull shadow stays in place. [Disabling shadows](docs/week2/mid-no-shadows.png) lets bright lines appear beneath the ship. |
| Dive lamp | Compare [lamp off](docs/week2/near-dark.png) and [lamp on](docs/week2/near-lamp.png) at the same pose, then inspect the [oblique view](docs/week2/near-oblique.png). Look for changes in the exposed muzzle, rust patches and surrounding wood. |

The captures and full route were rendered on Intel Arc graphics. The week-two captures use the original blockout. The 1080p presentation-machine performance target still requires a timed run. See the [four-week execution plan](DEVELOPMENT_PLAN.md) for the later rendering milestones.

## Week-three scene and first recording

[Watch the two-minute dive](docs/week3/first-dive.mp4) or inspect the [discovery](docs/week3/discovery.png), [approach](docs/week3/approach.png) and [inspection](docs/week3/inspection.png) frames. The original camera bookmarks are used throughout. This archived sunlit recording switches on the lamp at 1:45.

Individual timber strakes, splintered ends and exposed framing now define the approach side. The cannon has a recessed muzzle, reinforcement rings, trunnions and a planked platform. Stone patches and short static seagrass flank the anchor chain. The geometry and procedural materials are authored in the project source.

The first recording uses 2,880 rendered frames encoded at 24 fps. To reproduce it with FFmpeg on PATH, run the following from the repository root in Windows Command Prompt (binary pipe):

```bat
build\wrecklight.exe --film | ffmpeg -f image2pipe -framerate 24 -vcodec ppm -i pipe:0 -c:v libx264 -crf 24 -pix_fmt yuv420p -movflags +faststart first-dive.mp4
```

The recording follows the scripted exterior path. Free-swim collision checks and the timed 1080p presentation-machine run are still pending.
