#pragma once
#include <GLFW/glfw3.h>

namespace scene {
enum Surface { Plain, Stone, Paving, Limewash, Timber, RoofTile, VeinedStone, Coping };
GLuint createMaterialProgram();
}
