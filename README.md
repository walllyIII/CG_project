# Wrecklight

A first-person night dive for CG 100433, Track 2. The dive starts in blue-black water. Lower the hand torch to find short runs of chain in rippled sand. Lift it again and the seabed drops out of view. As the diver follows the chain, the dark mass beside it resolves into a wreck.

[Watch the current two-minute dive](docs/weathered-wreck/dive.mp4) · [Open the four review frames](docs/black-water/review.png)

| Replay time | What to look for |
| --- | --- |
| 0:00–0:32 | The camera first faces open water, dips to the chain, lifts back into the water and lowers again. The torch remains on throughout. Silt crosses the beam at different apparent speeds and sizes with distance. |
| 0:46–1:09 | Exposed links disappear under ripple crests and reappear farther ahead. A bank of sediment rises over the lower hull. A spar driven into the slope catches a barrel below the gunport, with the caps of several cannonballs showing through the sand. |
| 1:11–1:15 | The torch moves across the three weathered planks and the camera shifts sideways to expose the rib behind the damaged fastening. |
| 1:19–1:38 | The diver inspects the muzzle, rope, crushed locker and berth. |
| 1:43–1:51 | The camera holds still. Watch the soaked strip flex in front of the cannon and its shadow move across the gun. Silt continues drifting sideways. |
| 1:51–2:00 | The torch sweeps away from the muzzle into open water. |

The torch starts on. Move the mouse to sweep it, and press **F** to switch it off. Light comes from the diver's right hand, fades across 5.5 to 11 scene units and casts a moving shadow. Water scattering is visible when the torch points beyond the seabed. The sand ridges and drifts are geometry shared by the visible scene and the torch shadow pass.

Press **7** for the three planks beside the muzzle and **8** for the oblique view. The upper strake splits along its fibers, leaving a narrow tip and a recessed gray-brown end. The middle plank curls away from a nail that remains in the frame. Lateral movement reveals the space between nail, plank and rib. A local sand drift covers part of the lower seam, where sediment and a short cluster of growth collect beneath the rounded lip. Across the wreck, end checks vary between strakes, deck ends and barrel staves have lost material, and washed-out grain breaks up the timber colors. [See the two wood studies and their surroundings](docs/weathered-wreck/review.png).

## Inside the broken gunport

Press **3** for the muzzle. Press **4** to look along the barrel at the frayed breeching rope, its barnacled ring and the overturned shot rack. The loose shot collects on the lower, far side of the deck. Press **5** for the crushed locker. A fallen beam pins the lid over a tin cup and ceramic shards. Sweep the torch over the slanted silver coins caught in the seams. Press **6** to look up at the torn hammock and unlit lantern. The canvas and lantern move slowly around their supports, and the torch shadows follow them.

All four viewpoints stay on the water side of the broken hull. The later part of the replay visits these details before holding at the muzzle. [See the four inspection frames](docs/gun-room/review.png).

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

The replay renders 1,201 frames along the exterior route using a fixed simulation clock of 10 steps per second. It writes three PPM files at simulated times 0, 60 and 103 seconds. The PNGs below were converted from those framebuffers at their original 1280 x 720 resolution. Replay runs as fast as the renderer allows.

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
