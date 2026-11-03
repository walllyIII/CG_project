#include <GLFW/glfw3.h>
#include "water.h"
#include "target.h"
#include <cstdlib>
#include <algorithm>
#include "scene.h"
#include "shader.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
constexpr double rad = 3.141592653589793 / 180.0;
struct Camera {
    double x = 6.6, y = 4.5, z = 8.5;
    double yaw = -32, pitch = -18.5;
};

Target reflectionTarget, refractionTarget;

struct PassTimes { double reflection = 0, refraction = 0, finalScene = 0; };
bool render(const Camera& c, const WaterState& water, int width, int height, int divisor = 1, PassTimes* timing = nullptr) {
    int rw = std::max(1,width/divisor), rh = std::max(1,height/divisor);
    double start = glfwGetTime();
    if (!reflectionTarget.resize(rw,rh) || !refractionTarget.resize(rw,rh)) return false;
    constexpr GLfloat sky[] = {.76f,.81f,.82f,1};
    constexpr GLfloat sideLight[] = {-.62f,.48f,-.64f,0};
    glClearColor(sky[0],sky[1],sky[2],sky[3]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double top = .1*std::tan(28*rad), right = top*width/height;
    glFrustum(-right,right,-top,top,.1,100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotated(-c.pitch,1,0,0);
    glRotated(c.yaw,0,1,0);
    glTranslated(-c.x,-c.y,-c.z);
    scene::bindMaterials();
    reflectionTarget.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();
    glScalef(1,-1,1);
    glLightfv(GL_LIGHT0,GL_POSITION,sideLight);
    const GLdouble above[] = {0,1,0,0};
    glClipPlane(GL_CLIP_PLANE0,above);
    glEnable(GL_CLIP_PLANE0);
    // Courtyard geometry is two-sided; the mirror reverses winding.
    glDisable(GL_CULL_FACE);
    scene::courtyard();
    glDisable(GL_CLIP_PLANE0);
    glPopMatrix();
    glLightfv(GL_LIGHT0,GL_POSITION,sideLight);
    if (timing) { glFinish(); timing->reflection += glfwGetTime()-start; start = glfwGetTime(); }
    refractionTarget.bind();
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // A crest can cover the inside of the pool wall above mean water level.
    // Clipping at zero there removes the near face and exposes distant back faces.
    // Only perimeter and submerged geometry participate in this pass.
    constexpr double crestBound = .052+.031+.019; // Sum of the three wave heights in water.cpp.
    const GLdouble below[] = {0,-1,0,water.amplitude*crestBound};
    glClipPlane(GL_CLIP_PLANE0,below);
    glEnable(GL_CLIP_PLANE0);
    scene::banks();
    scene::submerged();
    glDisable(GL_CLIP_PLANE0);
    if (timing) { glFinish(); timing->refraction += glfwGetTime()-start; start = glfwGetTime(); }
    bindScreen(width,height);
    glClearColor(sky[0],sky[1],sky[2],sky[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene::courtyard();
    scene::submerged();
    drawWater(water,c.x,c.y,c.z,reflectionTarget.color,refractionTarget.color,refractionTarget.depth);
    if (timing) { glFinish(); timing->finalScene += glfwGetTime()-start; }
    return true;
}

void waterKey(GLFWwindow* window, int key, int, int action, int) {
    if (action == GLFW_RELEASE) return;
    auto& water = *static_cast<WaterState*>(glfwGetWindowUserPointer(window));
    switch (key) {
        case GLFW_KEY_1: if (action == GLFW_PRESS) water.reflection = !water.reflection; break;
        case GLFW_KEY_2: if (action == GLFW_PRESS) water.distortion = !water.distortion; break;
        case GLFW_KEY_3: if (action == GLFW_PRESS) water.fresnel = !water.fresnel; break;
        case GLFW_KEY_4: if (action == GLFW_PRESS) water.debugView = (water.debugView+1)%4; break;
        case GLFW_KEY_COMMA: water.absorption = std::max(0.f,water.absorption-.1f); break;
        case GLFW_KEY_PERIOD: water.absorption = std::min(3.f,water.absorption+.1f); break;
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
    Camera camera;
    bool benchmark = argc >= 2 && std::string(argv[1]) == "--benchmark" &&
                     (argc == 2 || (argc == 3 && std::string(argv[2]) == "half"));
    bool snapshot = argc >= 3 && argc <= 7 && std::string(argv[1]) == "--capture";
    if (snapshot && argc >= 4) snapshot = parseNumber(argv[3],2,water.amplitude);
    if (snapshot && argc >= 5) snapshot = parseNumber(argv[4],86400,water.time);
    if (snapshot && argc >= 6) {
        std::string mode = argv[5];
        if (mode == "no-reflection") water.reflection = false;
        else if (mode == "no-distortion") water.distortion = false;
        else if (mode == "fixed-blend") water.fresnel = false;
        else if (mode == "no-absorption") water.absorption = 0;
        else if (mode == "depth") water.debugView = 3;
        else if (mode == "coverage") water.debugView = 4;
        else if (mode != "full") snapshot = false;
    }
    if (snapshot && argc == 7) {
        std::string view = argv[6];
        if (view == "grazing") camera = {2.5,1.25,6,-15,-7};
        else if (view == "top") camera = {0,10,2,0,-77};
        else if (view == "edge") camera = {5.8,1.8,3.6,-45,-32};
        else if (view == "shore") camera = {4.9,1.45,6.8,-22,-22};
        else if (view != "default") snapshot = false;
    }
    if (!benchmark && !snapshot && argc != 1) {
        std::cerr << "Usage: courtyard [--capture output.ppm [amplitude 0..2 [seconds 0..86400 [mode [view]]]]] or --benchmark [half]\n";
        return 1;
    }
    glfwSetErrorCallback([](int, const char* message) { std::cerr << message << '\n'; });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    if (snapshot || benchmark) glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1280,720,"Courtyard Reflections",nullptr,nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    if (!initializeTargets() || !shader::initialize() || !initializeWater() || !scene::initialize()) {
        scene::release();
        releaseWater();
        glfwTerminate();
        return 1;
    }
    glfwSetWindowUserPointer(window,&water);
    glfwSetKeyCallback(window,waterKey);
    glfwSwapInterval(benchmark ? 0 : 1);
    glEnable(GL_DEPTH_TEST);
    double last = glfwGetTime();
    int result = 0;
    if (benchmark) {
        int w,h;
        glfwGetFramebufferSize(window,&w,&h);
        int divisor = argc == 3 ? 2 : 1;
        constexpr int warmup = 60, frames = 300, profileFrames = 60;
        std::cout << glGetString(GL_RENDERER) << " | " << glGetString(GL_VERSION)
                  << " | screen " << w << "x" << h << " | target " << w/divisor << "x" << h/divisor << '\n';
        PassTimes times;
        double start = 0;
        for (int i = -warmup; i < frames+profileFrames; ++i) {
            if (i == 0) { glFinish(); start = glfwGetTime(); }
            water.time = (i+warmup)/60.f;
            if (!render(camera,water,w,h,divisor,i >= frames ? &times : nullptr)) { result = 1; break; }
            // Consume a water-region pixel before swapping the hidden back buffer.
            // This also fences each measured frame and prevents queued/occluded swaps
            // from being mistaken for completed rendering.
            unsigned char probe[4];
            glReadPixels(w/2,h/2,1,1,GL_RGBA,GL_UNSIGNED_BYTE,probe);
            glfwSwapBuffers(window);
            if (i == frames-1) {
                glFinish();
                double seconds = glfwGetTime()-start;
                std::cout << frames << " frames in " << seconds << " s, " << frames/seconds
                          << " FPS, " << 1000*seconds/frames << " ms/frame\n";
            }
        }
        std::cout << "Serialized pass timings (ms): " << times.reflection*1000/profileFrames
                  << ", " << times.refraction*1000/profileFrames << ", " << times.finalScene*1000/profileFrames << '\n';
        if (glGetError() != GL_NO_ERROR) result = 1;
    }
    while (!benchmark && !glfwWindowShouldClose(window)) {
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
        if (!render(camera,water,w,h)) { result = 1; break; }
        if (snapshot) { result = capture(argv[2],w,h) ? 0 : 1; break; }
        glfwSwapBuffers(window);
    }
    releaseWater();
    scene::release();
    refractionTarget.release();
    reflectionTarget.release();
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
