#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
void drawGraybox();

namespace {
constexpr double rad=3.141592653589793/180;
struct Camera {double x=0,y=3.25,z=12,yaw=0,pitch=-9;};
constexpr Camera views[]={{}, {-1.35,2.45,2.5,12,-9}, {2.15,3.45,-5.65,-22,-20}};
constexpr const char* names[]={"discovery","approach","inspection"};

// Replay 120 seconds of camera travel on a fixed simulation clock. The same
// poses drive the interactive bookmarks and the captured review frames.
Camera routePose(int frame){
    constexpr int legFrames=600;
    int leg=std::min(frame/legFrames,1);
    double t=double(frame-leg*legFrames)/legFrames;
    t=t*t*(3-2*t);
    const Camera &a=views[leg], &b=views[leg+1];
    auto mix=[&](double x,double y){return x+(y-x)*t;};
    return {mix(a.x,b.x),mix(a.y,b.y),mix(a.z,b.z),mix(a.yaw,b.yaw),mix(a.pitch,b.pitch)};
}
struct Viewer {Camera camera;double mx=0,my=0;bool first=true;};
void look(GLFWwindow* w,double x,double y){
    auto &v=*static_cast<Viewer*>(glfwGetWindowUserPointer(w));
    if(!v.first){
        v.camera.yaw+=(x-v.mx)*.11;
        v.camera.pitch=std::clamp(v.camera.pitch-(y-v.my)*.11,-85.,85.);
    }
    v.first=false;v.mx=x;v.my=y;
}
void move(GLFWwindow* w,Camera &c,double dt){
    auto key=[&](int k){return glfwGetKey(w,k)==GLFW_PRESS;};
    if(key(GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(w,true);
    for(int i=0;i<3;++i){
        if(key(GLFW_KEY_1+i) || (i==0 && key(GLFW_KEY_R))){c=views[i];return;}
    }
    double yaw=c.yaw*rad;
    double f=key(GLFW_KEY_W)-key(GLFW_KEY_S), s=key(GLFW_KEY_D)-key(GLFW_KEY_A);
    double speed=3.2*dt/std::max(1.,std::hypot(f,s));
    c.x+=(std::sin(yaw)*f+std::cos(yaw)*s)*speed;
    c.z+=(-std::cos(yaw)*f+std::sin(yaw)*s)*speed;
    c.y=std::clamp(c.y+(key(GLFW_KEY_SPACE)-key(GLFW_KEY_LEFT_CONTROL))*3.2*dt,.4,18.);
}
void render(const Camera &c,int w,int h){
    const GLfloat water[]={.19f,.34f,.37f,1};
    glClearColor(water[0],water[1],water[2],1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();
    double top=.1*std::tan(28*rad),right=top*w/h;
    glFrustum(-right,right,-top,top,.1,110);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    glRotated(-c.pitch,1,0,0);glRotated(c.yaw,0,1,0);glTranslated(-c.x,-c.y,-c.z);
    const GLfloat light[]={-.48f,1,.65f,0},ambient[]={.43f,.45f,.45f,1};
    glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);glLightfv(GL_LIGHT0,GL_POSITION,light);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
    glEnable(GL_FOG);glFogi(GL_FOG_MODE,GL_LINEAR);glFogfv(GL_FOG_COLOR,water);
    glFogf(GL_FOG_START,6);glFogf(GL_FOG_END,54);
    drawGraybox();
}
// PPM keeps the framebuffer capture independent of image-library dependencies.
bool capture(const char* path,int w,int h){
    std::vector<unsigned char> pixels(static_cast<size_t>(w)*h*3);
    glReadBuffer(GL_BACK);glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream file(path,std::ios::binary);
    file<<"P6\n"<<w<<" "<<h<<"\n255\n";
    for(int y=h-1;y>=0;--y) file.write(reinterpret_cast<char*>(pixels.data())+y*w*3,w*3);
    file.close();return file.good() && glGetError()==GL_NO_ERROR;
}
}
int main(int argc,char** argv){
    bool snapshot=argc==3 && std::string(argv[1])=="--capture";
    bool route=argc==3 && std::string(argv[1])=="--route";
    if(route) std::filesystem::create_directories(argv[2]);
    if(argc!=1 && !snapshot && !route){std::cerr<<"Usage: wrecklight [--capture image.ppm | --route output_directory]\n";return 2;}
    glfwSetErrorCallback([](int,const char* msg){std::cerr<<msg<<'\n';});
    if(!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,2);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_VISIBLE,(snapshot || route)?GLFW_FALSE:GLFW_TRUE);
    auto* w=glfwCreateWindow(1280,720,"Wrecklight | Discovery graybox",nullptr,nullptr);
    if(!w){glfwTerminate();return 1;}
    glfwMakeContextCurrent(w);glfwSwapInterval(route?0:1);
    std::cout<<"OpenGL "<<glGetString(GL_VERSION)<<" / "<<glGetString(GL_RENDERER)<<'\n';
    Viewer viewer;glfwSetWindowUserPointer(w,&viewer);
    if(!snapshot && !route){glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED);glfwSetCursorPosCallback(w,look);}
    double last=glfwGetTime();bool ok=true;int frame=0;
    while(!glfwWindowShouldClose(w)){
        glfwPollEvents();double now=glfwGetTime();
        if(route) viewer.camera=routePose(frame);
        else if(!snapshot) move(w,viewer.camera,std::min(now-last,.05));
        last=now;
        int width,height;glfwGetFramebufferSize(w,&width,&height);
        if(width<1 || height<1){glfwWaitEventsTimeout(.05);continue;}
        render(viewer.camera,width,height);
        if(snapshot){ok=capture(argv[2],width,height);break;}
        if(route && frame%600==0){
            int view=frame/600;
            std::string path=(std::filesystem::path(argv[2])/(std::string(names[view])+".ppm")).string();
            ok=capture(path.c_str(),width,height);
            std::cout<<"Route frame "<<frame<<" ("<<frame/10<<" s): "<<names[view]<<std::endl;
            if(!ok || frame==1200) break;
        }
        glfwSwapBuffers(w);
        ++frame;
    }
    glfwDestroyWindow(w);glfwTerminate();return ok?0:1;
}
