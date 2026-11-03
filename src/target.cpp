#include "target.h"
#ifdef __APPLE__
#include <OpenGL/glext.h>
#else
#include <GL/glext.h>
#endif
#include <iostream>

namespace {
PFNGLGENFRAMEBUFFERSPROC gen;
PFNGLBINDFRAMEBUFFERPROC bindFramebuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC attach;
PFNGLCHECKFRAMEBUFFERSTATUSPROC status;
PFNGLDELETEFRAMEBUFFERSPROC destroy;

void texture(GLuint& id, int w, int h, bool depth) {
    if (!id) glGenTextures(1,&id);
    glBindTexture(GL_TEXTURE_2D,id);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,0,depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8,w,h,0,
                 depth ? GL_DEPTH_COMPONENT : GL_RGBA,depth ? GL_UNSIGNED_INT : GL_UNSIGNED_BYTE,nullptr);
}
}
bool initializeTargets() {
    gen = reinterpret_cast<decltype(gen)>(glfwGetProcAddress("glGenFramebuffers"));
    bindFramebuffer = reinterpret_cast<decltype(bindFramebuffer)>(glfwGetProcAddress("glBindFramebuffer"));
    attach = reinterpret_cast<decltype(attach)>(glfwGetProcAddress("glFramebufferTexture2D"));
    status = reinterpret_cast<decltype(status)>(glfwGetProcAddress("glCheckFramebufferStatus"));
    destroy = reinterpret_cast<decltype(destroy)>(glfwGetProcAddress("glDeleteFramebuffers"));
    if (gen && bindFramebuffer && attach && status && destroy) return true;
    std::cerr << "OpenGL framebuffer objects (ARB_framebuffer_object) required\n";
    return false;
}
bool Target::resize(int w, int h) {
    if (width == w && height == h) return true;
    if (!framebuffer) gen(1,&framebuffer);
    bindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    texture(color,w,h,false);
    texture(depth,w,h,true);
    attach(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
    attach(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,depth,0);
    if (status(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Incomplete render target " << w << "x" << h << '\n';
        return false;
    }
    width = w; height = h;
    return true;
}
void Target::bind() const {
    bindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glViewport(0,0,width,height);
}
void Target::release() {
    if (framebuffer) destroy(1,&framebuffer);
    glDeleteTextures(1,&color);
    glDeleteTextures(1,&depth);
}
void bindScreen(int w, int h) {
    bindFramebuffer(GL_FRAMEBUFFER,0);
    glViewport(0,0,w,h);
}
