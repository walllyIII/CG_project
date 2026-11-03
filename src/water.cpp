#include "water.h"
#include "shader.h"
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
GL_ENTRY(PFNGLGETUNIFORMLOCATIONPROC, uniformLocation);
GL_ENTRY(PFNGLUNIFORM1FPROC, uniform1f);
GL_ENTRY(PFNGLUNIFORM3FPROC, uniform3f);
GL_ENTRY(PFNGLACTIVETEXTUREPROC, activeTexture);
GL_ENTRY(PFNGLUNIFORM1IPROC, uniform1i);
#undef GL_ENTRY

GLuint program, normalTexture, grid;
GLint timeLoc, ampLoc, headingLoc, eyeLoc, effectsLoc, debugLoc, absorptionLoc;

const char* vertexSource = R"GLSL(
#version 120
uniform float t, amplitude, heading;
varying vec3 world;
varying vec2 slope;
varying vec4 mirrorClip, sceneClip;
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
    mirrorClip = gl_ModelViewProjectionMatrix * vec4(world.x,-world.y,world.z,1.0);
    sceneClip = gl_ModelViewProjectionMatrix * vec4(world,1.0);
    gl_Position = sceneClip;
}
)GLSL";

const char* fragmentSource = R"GLSL(
#version 120
uniform float t, amplitude;
uniform vec3 eye, effects;
uniform int debugView;
uniform float absorption;
uniform sampler2D detail, reflectionMap, transmissionMap, depthMap;
varying vec3 world;
varying vec2 slope;
varying vec4 mirrorClip, sceneClip;
vec2 detailSlope(vec2 uv) {
    vec3 n = texture2D(detail,uv).rgb*2.0-1.0;
    return -n.xy/max(n.z,.01);
}
float eyeDepth(float d) {
    return gl_ProjectionMatrix[3][2]/(2.0*d-1.0+gl_ProjectionMatrix[2][2]);
}
vec3 eyePoint(vec2 uv,float d) {
    float z = eyeDepth(d);
    return vec3((uv*2.0-1.0)*z/vec2(gl_ProjectionMatrix[0][0],gl_ProjectionMatrix[1][1]),-z);
}
void main() {
    vec2 fine = detailSlope(world.xz*.63 + vec2(.035,-.017)*t)
              + detailSlope(world.xz*.91 + vec2(-.023,.028)*t)*.55;
    vec2 grad = slope + .78*amplitude*fine;
    vec3 n = normalize(vec3(-grad.x,1.0,-grad.y));
    vec3 light = normalize(gl_LightSource[0].position.xyz);
    vec3 halfVector = normalize(normalize(-(gl_ModelViewMatrix*vec4(world,1)).xyz) + light);
    float glint = pow(max(dot(normalize(gl_NormalMatrix*n),halfVector),0.0),48.0);
    vec2 screenUV = sceneClip.xy/sceneClip.w*.5+.5;
    vec2 offset = clamp((gl_ModelViewMatrix*vec4(n-vec3(0,1,0),0)).xy*.035,
                        vec2(-.018),vec2(.018));
    vec2 sampleUV = screenUV + effects.y*offset;
    float floorDepth = texture2D(depthMap,sampleUV).r;
    float baseDepth = texture2D(depthMap,screenUV).r;
    // Reject empty texels, viewport crossings, foreground hits and depth discontinuities.
    if (any(lessThan(sampleUV,vec2(0))) || any(greaterThan(sampleUV,vec2(1))) ||
        floorDepth >= 1.0 || floorDepth <= gl_FragCoord.z ||
        abs(eyeDepth(floorDepth)-eyeDepth(baseDepth)) > .65) {
        sampleUV = screenUV;
        floorDepth = baseDepth;
    }
    vec3 floorColor = texture2D(transmissionMap,sampleUV).rgb;
    // In a closed pool every visible water pixel must hit underwater geometry.
    // Check both the original ray and the sample retained after fallback.
    if (debugView == 4) {
        bool missing = baseDepth >= 1.0 || floorDepth >= 1.0;
        gl_FragColor = missing ? vec4(1,0,1,1) : vec4(0,1,0,1);
        return;
    }
    vec3 floorPoint = eyePoint(sampleUV,floorDepth);
    vec3 surfacePoint = (gl_ModelViewMatrix*vec4(world,1)).xyz;
    float path = length(floorPoint-surfacePoint);
    vec3 attenuation = exp(-vec3(.64,.32,.48)*path*absorption);
    vec3 transmitted = mix(vec3(.105,.247,.202),floorColor,attenuation);
    vec2 mirrorUV = mirrorClip.xy/mirrorClip.w*.5+.5+offset*.55;
    vec3 reflected = texture2D(reflectionMap,mirrorUV).rgb;
    float cosine = clamp(dot(normalize(eye-world),n),0.0,1.0);
    float weight = .035+.965*pow(1.0-cosine,5.0);
    weight = mix(.32,weight,effects.z)*effects.x;
    if (debugView == 1) { gl_FragColor = vec4(reflected,1); return; }
    if (debugView == 2) { gl_FragColor = vec4(floorColor,1); return; }
    if (debugView == 3) { gl_FragColor = vec4(vec3(clamp(path/5.0,0.0,1.0)),1); return; }
    gl_FragColor = vec4(mix(transmitted,reflected,weight)+vec3(.085,.080,.069)*glint,1);

}
)GLSL";

}

bool initializeWater() {
#define LOAD(name, symbol) name = reinterpret_cast<decltype(name)>(glfwGetProcAddress(symbol)); if (!name) { std::cerr << "Missing " << symbol << '\n'; return false; }
    LOAD(uniformLocation,"glGetUniformLocation")
    LOAD(uniform1f,"glUniform1f")
    LOAD(uniform3f,"glUniform3f")
    LOAD(activeTexture,"glActiveTexture")
    LOAD(uniform1i,"glUniform1i")
#undef LOAD
    program = shader::build(vertexSource,fragmentSource);
    if (!program) return false;
    shader::bind(program);
    uniform1i(uniformLocation(program,"reflectionMap"),1);
    uniform1i(uniformLocation(program,"transmissionMap"),2);
    uniform1i(uniformLocation(program,"depthMap"),3);
    shader::bind(0);
    timeLoc = uniformLocation(program,"t");
    ampLoc = uniformLocation(program,"amplitude");
    headingLoc = uniformLocation(program,"heading");
    eyeLoc = uniformLocation(program,"eye");

    effectsLoc = uniformLocation(program,"effects");
    debugLoc = uniformLocation(program,"debugView");
    absorptionLoc = uniformLocation(program,"absorption");

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

void drawWater(const WaterState& s, float x, float y, float z, GLuint reflection, GLuint transmission, GLuint depth) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    shader::bind(program);
    uniform1f(timeLoc,s.time);
    uniform1f(ampLoc,s.amplitude);
    uniform1f(headingLoc,s.heading);
    uniform3f(effectsLoc,s.reflection,s.distortion,s.fresnel);
    uniform1i(debugLoc,s.debugView);
    uniform1f(absorptionLoc,s.absorption);
    uniform3f(eyeLoc,x,y,z);
    glBindTexture(GL_TEXTURE_2D,normalTexture);
    activeTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,reflection);
    activeTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,transmission);
    activeTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D,depth);
    activeTexture(GL_TEXTURE0);
    glCallList(grid);
    shader::bind(0);
    glDisable(GL_BLEND);
}

void releaseWater() {
    if (grid) glDeleteLists(grid,1);
    if (normalTexture) glDeleteTextures(1,&normalTexture);
    if (program) shader::release(program);
}
