#pragma once
#include <GLFW/glfw3.h>

namespace shader {
bool initialize();
GLuint build(const char* vertex,const char* fragment);
void bind(GLuint program);
void release(GLuint program);
}
