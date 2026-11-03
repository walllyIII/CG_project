#include "shader.h"
#ifdef __APPLE__
#include <OpenGL/glext.h>
#else
#include <GL/glext.h>
#endif
#include <iostream>

namespace shader {
namespace {
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
#undef GL_ENTRY

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

bool initialize() {
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
#undef LOAD
    return true;
}

GLuint build(const char* vertex,const char* fragment) {
    GLuint vert = compile(GL_VERTEX_SHADER,vertex);
    GLuint frag = compile(GL_FRAGMENT_SHADER,fragment);
    if (!vert || !frag) {
        if (vert) deleteShader(vert);
        if (frag) deleteShader(frag);
        return 0;
    }
    GLuint program = createProgram();
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
        deleteProgram(program);
        return 0;
    }
    return program;
}
void bind(GLuint program) { useProgram(program); }
void release(GLuint program) { if (program) deleteProgram(program); }
} // namespace shader
