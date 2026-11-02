#include "water.h"
#ifdef __APPLE__
#include <OpenGL/glext.h>
#else
#include <GL/glext.h>
#endif
#include <array>
#include <cmath>
#include <iostream>

namespace {
// OpenGL 2 entry points are loaded after GLFW makes the context current.
#define GL_ENTRY(type, name) type name = nullptr
GL_ENTRY(PFNGLCREATESHADERPROC, createShader);
GL_ENTRY(PFNGLSHADERSOURCEPROC, shaderSource);
GL_ENTRY(PFNGLCOMPILESHADERPROC, compileShader);
GL_ENTRY(PFNGLGETSHADERIVPROC, getShader);
GL_ENTRY(PFNGLGETSHADERINFOLOGPROC, shaderLog);
GL_ENTRY(PFNGLDELETESHADERPROC, deleteShader);
GL_ENTRY(PFNGLCREATEPROGRAMPROC, createProgram);
GL_ENTRY(PFNGLATTACHSHADERPROC, attachShader);
GL_ENTRY(PFNGLLINKPROGRAMPROC, linkProgram);
GL_ENTRY(PFNGLGETPROGRAMIVPROC, getProgram);
GL_ENTRY(PFNGLGETPROGRAMINFOLOGPROC, programLog);
GL_ENTRY(PFNGLUSEPROGRAMPROC, useProgram);
GL_ENTRY(PFNGLDELETEPROGRAMPROC, deleteProgram);
GL_ENTRY(PFNGLGETUNIFORMLOCATIONPROC, uniformLocation);
GL_ENTRY(PFNGLUNIFORM1FPROC, uniform1f);
GL_ENTRY(PFNGLUNIFORM3FPROC, uniform3f);
#undef GL_ENTRY

GLuint program, normalTexture, grid;
GLint timeLoc, ampLoc, headingLoc, eyeLoc;

const char* vertexSource = R"GLSL(
#version 120
uniform float t, amplitude, heading;
varying vec3 world;
varying vec2 slope;
void wave(vec2 dir, float k, float speed, float a) {
    float c = cos(heading), s = sin(heading);
    dir = mat2(c,s,-s,c) * normalize(dir);
    float phase = k * dot(dir, gl_Vertex.xz) - speed*t;
    world.y += amplitude*a*sin(phase);
    slope += amplitude*a*k*cos(phase)*dir;
}
void main() {
    world = gl_Vertex.xyz;
    slope = vec2(0.0);
    wave(vec2(1.0,.3), 2.5, 1.9, .052);
    wave(vec2(-.2,1.0), 3.8, 2.4, .031);
    wave(vec2(.7,-.8), 5.2, 3.1, .019);
    gl_Position = gl_ModelViewProjectionMatrix * vec4(world,1.0);
}
)GLSL";

const char* fragmentSource = R"GLSL(
#version 120
uniform float t, amplitude;
uniform vec3 eye;
uniform sampler2D detail;
varying vec3 world;
varying vec2 slope;
vec2 detailSlope(vec2 uv) {
    vec3 n = texture2D(detail,uv).rgb*2.0-1.0;
    return -n.xy/max(n.z,.01);
}
void main() {
    vec2 fine = detailSlope(world.xz*.63 + vec2(.035,-.017)*t)
              + detailSlope(world.xz*.91 + vec2(-.023,.028)*t)*.55;
    vec2 grad = slope + amplitude*fine;
    vec3 n = normalize(vec3(-grad.x,1.0,-grad.y));
    vec3 h = normalize(normalize(eye-world) + normalize(vec3(-.48,.48,-.73)));
    float glint = pow(max(dot(n,h),0.0),85.0);
    vec3 color = vec3(.12,.58,.65) + vec3(.75,.72,.63)*glint*min(amplitude,1.0)
               + vec3(.32,.40,.37)*dot(n.xz,vec2(.6,.8));
    gl_FragColor = vec4(color,.38);
}
)GLSL";

GLuint compile(GLenum type, const char* source) {
    GLuint shader = createShader(type);
    shaderSource(shader,1,&source,nullptr);
    compileShader(shader);
    GLint ok;
    getShader(shader,GL_COMPILE_STATUS,&ok);
    if (ok) return shader;
    char message[2048]{};
    shaderLog(shader,sizeof(message),nullptr,message);
    std::cerr << message << '\n';
    deleteShader(shader);
    return 0;
}
}

bool initializeWater() {
#define LOAD(name, symbol) name = reinterpret_cast<decltype(name)>(glfwGetProcAddress(symbol)); if (!name) { std::cerr << "Missing " << symbol << '\n'; return false; }
    LOAD(createShader,"glCreateShader")
    LOAD(shaderSource,"glShaderSource")
    LOAD(compileShader,"glCompileShader")
    LOAD(getShader,"glGetShaderiv")
    LOAD(shaderLog,"glGetShaderInfoLog")
    LOAD(deleteShader,"glDeleteShader")
    LOAD(createProgram,"glCreateProgram")
    LOAD(attachShader,"glAttachShader")
    LOAD(linkProgram,"glLinkProgram")
    LOAD(getProgram,"glGetProgramiv")
    LOAD(programLog,"glGetProgramInfoLog")
    LOAD(useProgram,"glUseProgram")
    LOAD(deleteProgram,"glDeleteProgram")
    LOAD(uniformLocation,"glGetUniformLocation")
    LOAD(uniform1f,"glUniform1f")
    LOAD(uniform3f,"glUniform3f")
#undef LOAD
    GLuint vert = compile(GL_VERTEX_SHADER,vertexSource);
    GLuint frag = compile(GL_FRAGMENT_SHADER,fragmentSource);
    if (!vert || !frag) { deleteShader(vert); deleteShader(frag); return false; }
    program = createProgram();
    attachShader(program,vert);
    attachShader(program,frag);
    linkProgram(program);
    deleteShader(vert);
    deleteShader(frag);
    GLint ok;
    getProgram(program,GL_LINK_STATUS,&ok);
    if (!ok) {
        char message[2048]{};
        programLog(program,sizeof(message),nullptr,message);
        std::cerr << message << '\n';
        return false;
    }
    timeLoc = uniformLocation(program,"t");
    ampLoc = uniformLocation(program,"amplitude");
    headingLoc = uniformLocation(program,"heading");
    eyeLoc = uniformLocation(program,"eye");

    // Encode a seamless periodic normal field with its tangent axes along world X/Z.
    constexpr int size = 128;
    std::array<GLubyte,size*size*3> pixels;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            double u = x*6.283185307/size, v = y*6.283185307/size;
            double a = .023*std::cos(3*u+2*v), b = .018*std::cos(u-4*v);
            double nx = -(3*a+b), ny = -(2*a-4*b);
            double len = std::sqrt(1+nx*nx+ny*ny);
            int offset = 3*(y*size+x);
            for (double component : {nx,ny,1.0})
                pixels[offset++] = static_cast<GLubyte>(127.5*(1+component/len)+.5);
        }
    }
    glGenTextures(1,&normalTexture);
    glBindTexture(GL_TEXTURE_2D,normalTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,size,size,0,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    grid = glGenLists(1);
    glNewList(grid,GL_COMPILE);
    for (int z = -32; z < 32; ++z) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int x = -48; x <= 48; ++x) {
            glVertex3f(x/8.f,0,z/8.f);
            glVertex3f(x/8.f,0,(z+1)/8.f);
        }
        glEnd();
    }
    glEndList();
    return glGetError() == GL_NO_ERROR;
}

void drawWater(const WaterState& s, float x, float y, float z) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    useProgram(program);
    uniform1f(timeLoc,s.time);
    uniform1f(ampLoc,s.amplitude);
    uniform1f(headingLoc,s.heading);
    uniform3f(eyeLoc,x,y,z);
    glBindTexture(GL_TEXTURE_2D,normalTexture);
    glCallList(grid);
    useProgram(0);
    glDisable(GL_BLEND);
}

void releaseWater() {
    if (grid) glDeleteLists(grid,1);
    if (normalTexture) glDeleteTextures(1,&normalTexture);
    if (program) deleteProgram(program);
}
