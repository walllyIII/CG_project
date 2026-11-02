#pragma once
#include <GLFW/glfw3.h>

struct WaterState {
    float amplitude = 1, heading = 0, time = 0;
    bool paused = false;
};

// Create and release with the rendering context current.
bool initializeWater();
void drawWater(const WaterState& state, float eyeX, float eyeY, float eyeZ, GLuint reflection, GLuint transmission, GLuint depth);
void releaseWater();
