#pragma once
#include <GLFW/glfw3.h>

struct Target {
    GLuint framebuffer = 0, color = 0, depth = 0;
    int width = 0, height = 0;
    bool resize(int w, int h);
    void bind() const;
    void release();
};
bool initializeTargets();
void bindScreen(int w, int h);
