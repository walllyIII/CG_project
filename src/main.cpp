#include <GLFW/glfw3.h>
#include "effects.h"
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
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
struct Camera {double x=0,y=3.25,z=12,yaw=-12,pitch=-19;};
constexpr Camera views[]={{}, {-1.35,2.45,2.5,12,-9}, {2.15,3.45,-5.65,-22,-20}};
constexpr const char* names[]={"discovery","approach","inspection"};

// Replay 120 seconds of camera travel on a fixed simulation clock. The same
// poses drive the interactive bookmarks and the captured review frames.
Camera routePose(double frame){
    constexpr int legFrames=600;
    int leg=std::min(static_cast<int>(frame/legFrames),1);
    double t=double(frame-leg*legFrames)/legFrames;
    t=t*t*(3-2*t);
    const Camera &a=views[leg], &b=views[leg+1];
    auto mix=[&](double x,double y){return x+(y-x)*t;};
    return {mix(a.x,b.x),mix(a.y,b.y),mix(a.z,b.z),mix(a.yaw,b.yaw),mix(a.pitch,b.pitch)};
}
struct Viewer {Camera camera;Effects effects;double mx=0,my=0;bool first=true;};
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
void toggle(GLFWwindow* w,int key,int,int action,int){
    if(action!=GLFW_PRESS) return;
    auto &e=static_cast<Viewer*>(glfwGetWindowUserPointer(w))->effects;
    if(key==GLFW_KEY_F) e.lamp=!e.lamp;
    if(key==GLFW_KEY_C) e.caustics=!e.caustics;
    if(key==GLFW_KEY_V) e.water=!e.water;
    if(key==GLFW_KEY_H) e.shadows=!e.shadows;
}
void render(const Camera &c,int w,int h,const Effects& effects,double time){
    const GLfloat water[]={.033f,.055f,.068f,1};
    glClearColor(water[0],water[1],water[2],1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();
    double top=.1*std::tan(28*rad),right=top*w/h;
    glFrustum(-right,right,-top,top,.1,110);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    glRotated(-c.pitch,1,0,0);glRotated(c.yaw,0,1,0);glTranslated(-c.x,-c.y,-c.z);
    bindEffects(effects,time,c.x,c.y,c.z,c.yaw,c.pitch);
    drawGraybox();
    finishDiveView(effects.lamp,time);
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
    bool film=argc==2 && std::string(argv[1])=="--film";
#ifdef _WIN32
    if(film) _setmode(_fileno(stdout),_O_BINARY);
#endif
    bool snapshot=argc==3 && std::string(argv[1])=="--capture";
    bool route=argc==3 && std::string(argv[1])=="--route";
    bool review=argc==3 && std::string(argv[1])=="--review";
    if(route || review) std::filesystem::create_directories(argv[2]);
    if(argc!=1 && !film && !snapshot && !route && !review){std::cerr<<"Usage: wrecklight [--capture image.ppm | --route output_directory | --review output_directory | --film]\n";return 2;}
    glfwSetErrorCallback([](int,const char* msg){std::cerr<<msg<<'\n';});
    if(!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,2);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_VISIBLE,(snapshot || route || review || film)?GLFW_FALSE:GLFW_TRUE);
    auto* w=glfwCreateWindow(1280,720,"Wrecklight | Wrecklight dive",nullptr,nullptr);
    if(!w){glfwTerminate();return 1;}
    glfwMakeContextCurrent(w);glfwSwapInterval(route || film?0:1);
    std::cerr<<"OpenGL "<<glGetString(GL_VERSION)<<" / "<<glGetString(GL_RENDERER)<<'\n';
    if(!initEffects()){std::cerr<<"Underwater renderer initialization failed\n";glfwTerminate();return 1;}
    Viewer viewer;glfwSetWindowUserPointer(w,&viewer);
    glfwSetKeyCallback(w,toggle);
    if(!film && !snapshot && !route && !review){glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED);glfwSetCursorPosCallback(w,look);}
    double last=glfwGetTime();bool ok=true;int frame=0;
    struct Comparison {const char* name; int view; Effects effects; double time;};
    const Comparison checks[]={
        {"far-water",0,{},4},{"far-clear",0,{false,true,true,true},4},
        {"mid-lit",1,{},4},{"mid-no-caustics",1,{true,false,true,true},4},
        {"mid-later",1,{},8},{"mid-no-shadows",1,{true,true,false,true},4},
        {"near-dark",2,{true,true,true,false},4},{"near-lamp",2,{true,true,true,true},4},
        {"near-oblique",2,{true,true,true,true},4},
        {"plank-near",2,{true,false,true,true},4},
        {"plank-middle",2,{true,false,true,true},4},
        {"plank-far",2,{true,false,true,true},4}
    };
    constexpr int filmFrames=24*120;
    std::vector<char> filmPixels;
    while(!glfwWindowShouldClose(w)){
        glfwPollEvents();double now=glfwGetTime();
        const double tick=film?1200.*frame/(filmFrames-1):frame;
        if(review){
            viewer.camera=views[checks[frame].view];
            viewer.effects=checks[frame].effects;
            std::string shot=checks[frame].name;
            if(shot=="near-oblique") viewer.camera={1.45,3.30,-5.55,-12,-18};
            if(shot.rfind("plank-",0)==0){
                double distance=shot=="plank-near"?3:(shot=="plank-middle"?10:22);
                // Look straight at a fixed point on the starboard plank, across all distances.
                viewer.camera={3.8,2.45,-6.25+distance,0,0};
            }
        }else if(route || film){
            viewer.camera=routePose(tick);viewer.effects.lamp=true;
        }
        else if(!snapshot) move(w,viewer.camera,std::min(now-last,.05));
        last=now;
        int width,height;glfwGetFramebufferSize(w,&width,&height);
        if(width<1 || height<1){glfwWaitEventsTimeout(.05);continue;}
        double time=review?checks[frame].time:((route || film)?tick*.1:(snapshot?0.:now));
        render(viewer.camera,width,height,viewer.effects,time);
        if(review){
            auto path=std::filesystem::path(argv[2])/(std::string(checks[frame].name)+".ppm");
            ok=capture(path.string().c_str(),width,height);
            if(!ok || frame==int(sizeof(checks)/sizeof(checks[0]))-1) break;
        }
        if(snapshot){ok=capture(argv[2],width,height);break;}
        if(film){
            filmPixels.resize(static_cast<size_t>(width)*height*3);
            glPixelStorei(GL_PACK_ALIGNMENT,1);glReadBuffer(GL_BACK);
            glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,filmPixels.data());
            // Concatenated PPMs keep frames self-describing on high-DPI displays.
            std::cout<<"P6\n"<<width<<" "<<height<<"\n255\n";
            for(int y=height-1;y>=0;--y) std::cout.write(filmPixels.data()+y*width*3,width*3);
            ok=std::cout.good() && glGetError()==GL_NO_ERROR;
            if(!ok || frame+1==filmFrames) break;
        }
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
    releaseEffects();glfwDestroyWindow(w);glfwTerminate();return ok?0:1;
}
