#include <GLFW/glfw3.h>
#include "water.h"
#include <cstdlib>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
using Color = std::array<float, 3>;
constexpr double rad = 3.141592653589793 / 180.0;
struct Camera {
    double x = 8.2, y = 5.5, z = 10.4;
    double yaw = -34, pitch = -22;
};

void tint(Color c, float light) {
    glColor3f(c[0]*light, c[1]*light, c[2]*light);
}

void box(float x, float y, float z, float w, float h, float d, Color color) {
    const float vertices[][3] = {{x,y,z},{x+w,y,z},{x+w,y+h,z},{x,y+h,z},
        {x,y,z+d},{x+w,y,z+d},{x+w,y+h,z+d},{x,y+h,z+d}};
    constexpr int faces[][4] = {{0,1,2,3},{4,5,6,7},{0,4,7,3},{1,5,6,2},{3,2,6,7},{0,1,5,4}};
    constexpr float lights[] = {.71f,.86f,.66f,.76f,1,.52f};
    glBegin(GL_QUADS);
    for (int f = 0; f < 6; ++f) {
        tint(color, lights[f]);
        for (int i : faces[f]) glVertex3fv(vertices[i]);
    }
    glEnd();
}

void courtyard() {
    const Color paving{.81f,.76f,.63f}, plaster{.93f,.89f,.76f};
    const Color timber{.45f,.21f,.14f}, slate{.21f,.29f,.30f};
    // Four solid banks leave an opening for the submerged floor.
    box(-12,-2,-12,24,2.25f,8,paving);
    box(-12,-2,4,24,2.25f,8,paving);
    box(-12,-2,-4,6,2.25f,8,paving);
    box(6,-2,-4,6,2.25f,8,paving);
    for (int x = -6; x < 6; ++x) {
        const float floorY = -.42f - .55f*((x+6)/4);
        for (int z = -4; z < 4; ++z) {
            const Color tile = ((x+z)%2 == 0) ? Color{.50f,.71f,.64f} : Color{.34f,.57f,.54f};
            box(float(x),-2,float(z),1,2+floorY,1,tile);
        }
    }
    box(-3,-.42f,1,1.2f,.24f,.75f,slate);
    box(3,-1.52f,-1,1.3f,.66f,.8f,slate);
    box(-12,.25f,-10,24,2.6f,.4f,plaster);
    box(-12,2.85f,-10.15f,24,.2f,.7f,slate);
    box(-11,.25f,-10,.4f,2.6f,20,plaster);
    box(-11.15f,2.85f,-10,.7f,.2f,20,slate);
    // Open pavilion with stepped eaves and exposed crossbeams.
    box(-4,.25f,-8,8,.3f,3.2f,paving);
    for (float x : {-3.6f,0.f,3.4f})
        for (float z : {-7.7f,-5.2f})
            box(x,.55f,z,.22f,2.65f,.22f,timber);
    box(-4,3.0f,-5.25f,8,.28f,.32f,timber);
    for (int layer = 0; layer < 5; ++layer) {
        float inset = .24f*layer;
        box(-4.6f+inset,3.28f+.17f*layer,-8.6f+inset,
            9.2f-2*inset,.18f,4.6f-2*inset,slate);
    }
}

void render(const Camera& c, const WaterState& water, int width, int height) {
    glViewport(0,0,width,height);
    glClearColor(.73f,.84f,.86f,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double top = .1*std::tan(28*rad), right = top*width/height;
    glFrustum(-right,right,-top,top,.1,100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotated(-c.pitch,1,0,0);
    glRotated(c.yaw,0,1,0);
    glTranslated(-c.x,-c.y,-c.z);
    courtyard();
    drawWater(water,c.x,c.y,c.z);
}

void waterKey(GLFWwindow* window, int key, int, int action, int) {
    if (action == GLFW_RELEASE) return;
    auto& water = *static_cast<WaterState*>(glfwGetWindowUserPointer(window));
    switch (key) {
        case GLFW_KEY_SPACE: if (action == GLFW_PRESS) water.paused = !water.paused; break;
        case GLFW_KEY_0: water.amplitude = 0; break;
        case GLFW_KEY_MINUS: water.amplitude = std::max(0.f,water.amplitude-.1f); break;
        case GLFW_KEY_EQUAL: water.amplitude = std::min(2.f,water.amplitude+.1f); break;
        case GLFW_KEY_LEFT_BRACKET: water.heading -= 15*rad; break;
        case GLFW_KEY_RIGHT_BRACKET: water.heading += 15*rad; break;
    }
}

bool parseNumber(const char* input, float max, float& out) {
    char* end;
    out = std::strtof(input,&end);
    return end != input && *end == '\0' && std::isfinite(out) && out >= 0 && out <= max;
}

void navigate(GLFWwindow* window, Camera& c, double dt) {
    auto key = [window](int code) { return glfwGetKey(window,code) == GLFW_PRESS; };
    c.yaw += (key(GLFW_KEY_RIGHT)-key(GLFW_KEY_LEFT))*70*dt;
    c.pitch = std::clamp(c.pitch+(key(GLFW_KEY_UP)-key(GLFW_KEY_DOWN))*70*dt,-85.,85.);
    double forward = key(GLFW_KEY_W)-key(GLFW_KEY_S), side = key(GLFW_KEY_D)-key(GLFW_KEY_A);
    double angle = c.yaw*rad, step = 4*dt;
    c.x += step*(forward*std::sin(angle)+side*std::cos(angle));
    c.z += step*(-forward*std::cos(angle)+side*std::sin(angle));
    c.y = std::clamp(c.y+step*(key(GLFW_KEY_E)-key(GLFW_KEY_Q)),.45,12.);
    if (key(GLFW_KEY_R)) c = Camera{};
    if (key(GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window,GLFW_TRUE);
}

bool capture(const char* path, int w, int h) {
    std::vector<unsigned char> pixels(static_cast<size_t>(w)*h*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream file(path,std::ios::binary);
    file << "P6\n" << w << ' ' << h << "\n255\n";
    for (int row = h-1; row >= 0; --row)
        file.write(reinterpret_cast<const char*>(pixels.data()+row*w*3),w*3);
    file.close();
    return file.good() && glGetError() == GL_NO_ERROR;
}
} // namespace

int main(int argc, char** argv) {
    WaterState water;
    bool snapshot = argc >= 3 && argc <= 5 && std::string(argv[1]) == "--capture";
    if (snapshot && argc >= 4) snapshot = parseNumber(argv[3],2,water.amplitude);
    if (snapshot && argc == 5) snapshot = parseNumber(argv[4],86400,water.time);
    if (!snapshot && argc != 1) {
        std::cerr << "Usage: courtyard [--capture output.ppm [amplitude 0..2 [seconds 0..86400]]]\n";
        return 1;
    }
    glfwSetErrorCallback([](int, const char* message) { std::cerr << message << '\n'; });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    if (snapshot) glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1280,720,"Courtyard Reflections",nullptr,nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    if (!initializeWater()) {
        releaseWater();
        glfwTerminate();
        return 1;
    }
    glfwSetWindowUserPointer(window,&water);
    glfwSetKeyCallback(window,waterKey);
    glfwSwapInterval(1);
    glEnable(GL_DEPTH_TEST);
    Camera camera;
    double last = glfwGetTime();
    int result = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double now = glfwGetTime();
        if (!snapshot) {
            navigate(window,camera,std::min(now-last,.1));
            if (!water.paused) water.time += static_cast<float>(now-last);
        }
        last = now;
        int w, h;
        glfwGetFramebufferSize(window,&w,&h);
        if (w <= 0 || h <= 0) { glfwWaitEventsTimeout(.1); continue; }
        render(camera,water,w,h);
        if (snapshot) { result = capture(argv[2],w,h) ? 0 : 1; break; }
        glfwSwapBuffers(window);
    }
    releaseWater();
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
