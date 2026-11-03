#pragma once

namespace scene {
bool initialize();
void bindMaterials();
void release();
void courtyard();  // Above-water structures and pool perimeter.
void banks();      // Shared perimeter, including the submerged wall.
void submerged();  // Floor, steps and submerged reference stones.
}
