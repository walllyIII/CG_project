#include "effects.h"
#include "shader.h"
#include "target.h"
#ifdef __APPLE__
#include <OpenGL/glext.h>
#else
#include <GL/glext.h>
#endif
#include <algorithm>
#include <cmath>
#include <vector>

void drawGraybox();
namespace {
GLuint program=0, pattern=0;
Target shadow;
PFNGLACTIVETEXTUREPROC activeTexture;
PFNGLGETUNIFORMLOCATIONPROC location;
PFNGLUNIFORM1IPROC setInt;
PFNGLUNIFORM1FPROC setFloat;
GLint flags[4],clockLocation;
const char* vert=R"(
#version 120
varying vec3 eye,world,n;
varying vec4 lightClip,material;
varying vec2 surface;
void main(){
    vec4 e=gl_ModelViewMatrix*gl_Vertex;
    eye=e.xyz;
    world=(gl_TextureMatrix[0]*e).xyz;
    lightClip=gl_TextureMatrix[1]*vec4(world,1.0);
    n=mat3(gl_TextureMatrix[0])*gl_NormalMatrix*gl_Normal;
    material=gl_Color; surface=gl_MultiTexCoord0.xy;
    gl_Position=gl_ProjectionMatrix*e;
})";
const char* frag=R"(
#version 120
varying vec3 eye,world,n;
varying vec4 lightClip,material;
varying vec2 surface;
uniform sampler2D depthMap,patternMap;
uniform bool water,caustics,shadows,lamp;
uniform float clockTime;
const vec3 sun=vec3(-0.304,0.743,-0.596);
float visibility(vec3 uv){
    if(!shadows || any(lessThan(uv,vec3(0))) || any(greaterThan(uv,vec3(1)))) return 1.0;
    float lit=0.0;
    for(int i=-1;i<=1;i++) for(int j=-1;j<=1;j++)
        lit+=step(uv.z-.0013,texture2D(depthMap,uv.xy+vec2(i,j)/1024.).r);
    return lit/9.;
}
vec3 brdf(vec3 N,vec3 V,vec3 L,vec3 color,float rough,float metal){
    vec3 H=normalize(V+L);
    float nl=max(dot(N,L),0.),nv=max(dot(N,V),.001),nh=max(dot(N,H),0.);
    float a2=pow(rough,4.), den=nh*nh*(a2-1.)+1.;
    float D=a2/(3.14159*den*den);
    float k=(rough+1.)*(rough+1.)/8.;
    float G=nv/(nv*(1.-k)+k)*nl/(nl*(1.-k)+k);
    vec3 F0=mix(vec3(.04),color,metal);
    vec3 F=F0+(1.-F0)*pow(1.-max(dot(V,H),0.),5.);
    return nl*((1.-F)*(1.-metal)*color/3.14159+D*G*F/max(.001,4.*nl*nv));
}
void main(){
    float dist=length(eye);
    vec3 V=normalize(mat3(gl_TextureMatrix[0])*(-eye));
    vec3 N=normalize(n);
    if(dot(N,V)<0.) N=-N;
    float rough=surface.x,metal=surface.y;
    vec3 base=material.rgb;
    // World-locked, low-amplitude surface relief. Wood grain and granular sand
    // have different spatial frequencies. A patchy oxide layer covers the iron.
    float relief;
    if(metal>.8){
        float oxide=smoothstep(.28,.73,sin(world.x*10.+world.z*4.)*sin(world.y*9.+world.z*5.));
        base=mix(base,vec3(.27,.072,.029),oxide);
        rough=mix(rough,.87,oxide); metal*=1.-oxide;
        relief=.05*oxide*sin(world.y*31.);
    }else if(rough<.7){
        float grain=sin(world.y*48.+2.1*sin(world.x*1.7+world.z*.8)+.7*sin(world.x*4.));
        base*=.92+.08*grain; relief=.045*grain;
    }else relief=.05*sin(world.x*19.)*cos(world.z*24.);
    N=normalize(N+relief*(vec3(.4,.3,.8)-N*dot(N,vec3(.4,.3,.8))));
    float lit=visibility(lightClip.xyz/lightClip.w*.5+.5);
    vec2 uv=(world.xz-world.y*sun.xz/sun.y)*.085;
    uv+=.024*vec2(sin(uv.y*14.+clockTime*.19),cos(uv.x*12.-clockTime*.15));
    float lines=.55*texture2D(patternMap,uv+clockTime*vec2(.011,.006)).r
               +.45*texture2D(patternMap,uv*1.37-clockTime*vec2(.007,.012)).r;
    float caustic=caustics?lines*smoothstep(.05,.72,dot(N,sun))*exp(-.055*max(0.,8.-world.y)):0.;
    vec3 color=.22*base+lit*vec3(.70,.94,1.)*(2.+4.*caustic)*brdf(N,V,sun,base,rough,metal);
    // Approximate broad sky and dark seabed reflections on the exposed iron.
    vec3 reflected=reflect(-V,N);
    color+=metal*base*(.045+.24*smoothstep(-.35,.85,reflected.y));
    if(lamp){
        float cone=smoothstep(.82,.965,-eye.z/max(.001,dist));
        color+=vec3(1.,.88,.73)*13.*cone/(1.+.18*dist*dist)*brdf(N,V,V,base,rough,metal);
    }
    vec3 waterColor=vec3(.033,.112,.134);
    if(water) color=mix(waterColor,color,exp(-dist*vec3(.097,.049,.032)));
    gl_FragColor=vec4(pow(clamp(color,0.,1.),vec3(1./2.2)),1.);
})";

void lightMatrix(){
    glLoadIdentity();glOrtho(-27,27,-27,27,1,90);
    glTranslated(0,0,-38);glRotated(48,1,0,0);glRotated(153,0,1,0);
    glTranslated(0,0,6);
}
void makePattern(){
    constexpr int side=256,cells=6;
    std::vector<unsigned char> data(side*side);
    for(int y=0;y<side;++y) for(int x=0;x<side;++x){
        double u=x*double(cells)/side,v=y*double(cells)/side,first=10,second=10;
        int ix=int(u),iy=int(v);
        for(int j=iy-1;j<=iy+1;++j) for(int i=ix-1;i<=ix+1;++i){
            int a=(i+cells)%cells,b=(j+cells)%cells;
            double dx=i+.5+.32*std::sin(a*13.+b*17.+.5)-u;
            double dy=j+.5+.32*std::cos(a*19.+b*7.+1.)-v;
            double d=std::hypot(dx,dy);
            if(d<first){second=first;first=d;}else second=std::min(second,d);
        }
        data[y*side+x]=static_cast<unsigned char>(255*std::exp(-55*(second-first)*(second-first)));
    }
    glGenTextures(1,&pattern);glBindTexture(GL_TEXTURE_2D,pattern);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,side,side,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,data.data());
}
}
bool initEffects(){
    if(!shader::initialize() || !initializeTargets()) return false;
    activeTexture=reinterpret_cast<PFNGLACTIVETEXTUREPROC>(glfwGetProcAddress("glActiveTexture"));
    location=reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(glfwGetProcAddress("glGetUniformLocation"));
    setInt=reinterpret_cast<PFNGLUNIFORM1IPROC>(glfwGetProcAddress("glUniform1i"));
    setFloat=reinterpret_cast<PFNGLUNIFORM1FPROC>(glfwGetProcAddress("glUniform1f"));
    if(!activeTexture || !location || !setInt || !setFloat) return false;
    program=shader::build(vert,frag);
    if(!program || !shadow.resize(1024,1024)) return false;
    // Geometry and sunlight are static, so the shadow map only needs one pass.
    shadow.bind();glEnable(GL_DEPTH_TEST);glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);lightMatrix();
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();drawGraybox();
    bindScreen(1280,720);makePattern();
    shader::bind(program);
    setInt(location(program,"depthMap"),0);setInt(location(program,"patternMap"),1);
    const char* keys[]={"water","caustics","shadows","lamp"};
    for(int i=0;i<4;++i) flags[i]=location(program,keys[i]);
    clockLocation=location(program,"clockTime");
    return glGetError()==GL_NO_ERROR;
}
void bindEffects(const Effects& e,double time,double x,double y,double z,double yaw,double pitch){
    activeTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,shadow.depth);
    glMatrixMode(GL_TEXTURE);glLoadIdentity();glTranslated(x,y,z);
    glRotated(-yaw,0,1,0);glRotated(pitch,1,0,0);
    activeTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,pattern);lightMatrix();
    activeTexture(GL_TEXTURE0);glMatrixMode(GL_MODELVIEW);
    setInt(flags[0],e.water);setInt(flags[1],e.caustics);setInt(flags[2],e.shadows);setInt(flags[3],e.lamp);
    setFloat(clockLocation,static_cast<float>(time));
}
void releaseEffects(){
    shader::bind(0);shader::release(program);shadow.release();glDeleteTextures(1,&pattern);
}
