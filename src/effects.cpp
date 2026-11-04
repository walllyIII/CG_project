#include "effects.h"
#include "current.h"
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

void drawGraybox(double time);
double seabedHeight(double x,double z);
namespace {
GLuint program=0, pattern=0;
Target shadow,torch;
PFNGLACTIVETEXTUREPROC activeTexture;
PFNGLGETUNIFORMLOCATIONPROC location;
PFNGLUNIFORM1IPROC setInt;
PFNGLUNIFORM1FPROC setFloat;
GLint flags[4],clockLocation;
const char* vert=R"(
#version 120
varying vec3 eye,world,n,objectPoint;
varying vec4 lightClip,material;
varying vec4 surface;
void main(){
    vec4 e=gl_ModelViewMatrix*gl_Vertex;
    eye=e.xyz;objectPoint=gl_Vertex.xyz;
    world=(gl_TextureMatrix[0]*e).xyz;
    lightClip=gl_TextureMatrix[1]*vec4(world,1.0);
    n=mat3(gl_TextureMatrix[0])*gl_NormalMatrix*gl_Normal;
    material=gl_Color; surface=gl_MultiTexCoord0;
    gl_Position=gl_ProjectionMatrix*e;
})";
const char* frag=R"(
#version 120
varying vec3 eye,world,n,objectPoint;
varying vec4 lightClip,material;
varying vec4 surface;
uniform sampler2D depthMap,patternMap,torchMap;
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
float hash3(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
float valueNoise(vec3 p){
    vec3 i=floor(p),f=fract(p); f=f*f*(3.-2.*f);
    return mix(mix(mix(hash3(i),hash3(i+vec3(1,0,0)),f.x),
                   mix(hash3(i+vec3(0,1,0)),hash3(i+vec3(1,1,0)),f.x),f.y),
               mix(mix(hash3(i+vec3(0,0,1)),hash3(i+vec3(1,0,1)),f.x),
                   mix(hash3(i+vec3(0,1,1)),hash3(i+vec3(1)),f.x),f.y),f.z);
}
float torchVisibility(vec3 ray){
    float z=-ray.z;
    vec2 uv=.5+.5*ray.xy/(z*.52);
    float depth=14./13.85*(1.-.15/z);
    float total=0.;
    for(int i=-1;i<=1;i++) for(int j=-1;j<=1;j++)
        total+=step(depth-.0013,texture2D(torchMap,uv+vec2(i,j)/768.).r);
    return total/9.;
}
vec3 waterScatter(vec3 endpoint){
    if(!water || !lamp) return vec3(0.);
    float dist=length(endpoint),reach=min(dist,11.);
    vec3 dir=endpoint/max(dist,.001);
    float scatter=0.;
    for(int i=0;i<10;i++){
        float t=(float(i)+.5)*reach/10.;
        vec3 q=dir*t-vec3(.36,-.28,-.22);
        vec3 p=(gl_TextureMatrix[0]*vec4(dir*t,1.)).xyz;
        float density=.91+.30*sin(.83*clockTime-.52*p.x-.19*p.z)
                          +.17*sin(p.y*2.3+.58*p.z-.61*clockTime);
        scatter+=density*smoothstep(.84,.988,-normalize(q).z)*exp(-.38*t)*reach/10.;
    }
    return scatter*vec3(.0023,.0057,.0088);
}
void main(){
    if(surface.x<0.){
        vec3 haze=vec3(.00035,.0018,.0039)+waterScatter(eye);
        gl_FragColor=vec4(pow(haze,vec3(1./2.2)),1.);return;
    }
    float dist=length(eye);
    vec3 V=normalize(mat3(gl_TextureMatrix[0])*(-eye));
    vec3 N=normalize(n);
    if(dot(N,V)<0.) N=-N;
    float rough=surface.x,metal=surface.y;
    vec3 base=material.rgb;
    // World-locked, low-amplitude surface relief. Wood grain and granular sand
    // have different spatial frequencies. A patchy oxide layer covers the iron.
    float pits=valueNoise(world*8.5), fine=valueNoise(world*35.);
    float relief;
    if(surface.z>2.5){
        float film=.7*pits+.3*fine;
        base*=.75+.36*film+.14*surface.w;
        rough=.54-.17*surface.w+.08*pits;
        relief=.0014*(fine+surface.w);
    }else if(surface.z>1.5){
        float strata=valueNoise(world*vec3(6.,19.,7.));
        float pores=smoothstep(.61,.83,.7*pits+.3*fine);
        float vein=1.-smoothstep(.025,.107,abs(strata-.53));
        base*=.46+.58*strata+.26*fine-.41*pores;
        base+=vec3(.041,.031,.020)*vein;
        base=mix(vec3(.185,.16,.112)*(.83+.30*fine),base,smoothstep(.0,.64,surface.w+.13*(pits-.5)));
        rough=.64+.24*pores+.07*(1.-surface.w);
        relief=.013*strata-.014*pores+.002*fine;
    }else if(surface.z>.5){
        float joint=1.-smoothstep(.009,.070+.036*pits,surface.w);
        vec2 seat=(objectPoint.yz-vec2(-.10,-.28))/vec2(.19,.25);
        float contact=(1.-smoothstep(.45,1.45,length(seat)))*smoothstep(.22,.36,abs(objectPoint.x));
        float rust=max(joint,contact)*smoothstep(.25,.74,.65*pits+.35*fine);
        base=mix(base*(.78+.27*pits),vec3(.185,.065,.024)*(.64+.42*fine),rust);
        rough=mix(rough,.91,rust);metal*=1.-rust;
        relief=.0015*fine+.007*rust*pits;
    }else if(metal>.8){
        float oxide=smoothstep(.41,.64,.64*pits+.36*valueNoise(world*2.6));
        base=mix(base,vec3(.22,.071,.028)*(.48+fine),oxide);
        rough=mix(rough,.88,oxide); metal*=1.-oxide;
        relief=.011*oxide*pits+.002*fine;
    }else if(rough<.7 && metal<.1){
        vec3 p=objectPoint;
        float warp=valueNoise(p*vec3(.71,2.4,1.5));
        float grain=valueNoise(vec3(p.x*3.1,p.y*44.+6.4*warp,p.z*7.));
        float pores=smoothstep(.55,.82,grain);
        float checks=smoothstep(.66,.86,valueNoise(vec3(p.x*1.8,p.y*105.+4.*warp,p.z*8.)));
        float bleach=smoothstep(.32,.80,warp)*.57;
        base=mix(base,vec3(dot(base,vec3(.33,.52,.15)))*vec3(.92,.94,.88),bleach);
        base*=.67+.37*grain+.21*pits-.32*pores-.30*checks;
        rough=clamp(rough+.13*pores-.09*warp,.38,.84);
        relief=.003*grain-.0038*checks+.001*fine;
    }else{
        float ripple=sin(world.x*7.+world.z*3.2+2.4*valueNoise(world*.65));
        base*=.65+.33*pits+.22*fine;
        relief=.011*ripple+.008*pits+.0015*fine;
    }
    vec3 dx=dFdx(world),dy=dFdy(world);
    vec3 r1=cross(dy,N),r2=cross(N,dx);
    float det=dot(dx,r1);
    vec3 slope=sign(det)*(dFdx(relief)*r1+dFdy(relief)*r2)/max(abs(det),.0000001);
    N=normalize(N-slope/(1.+2.4*length(slope)));
    float lit=visibility(lightClip.xyz/lightClip.w*.5+.5);
    vec2 uv=(world.xz-world.y*sun.xz/sun.y)*.085;
    uv+=.024*vec2(sin(uv.y*14.+clockTime*.19),cos(uv.x*12.-clockTime*.15));
    float lines=.55*texture2D(patternMap,uv+clockTime*vec2(.011,.006)).r
               +.45*texture2D(patternMap,uv*1.37-clockTime*vec2(.007,.012)).r;
    float caustic=caustics?lines*smoothstep(.05,.72,dot(N,sun))*exp(-.055*max(0.,8.-world.y)):0.;
    vec3 color=.0018*base+lit*vec3(.045,.11,.16)*(.095+.07*caustic)*brdf(N,V,sun,base,rough,metal);
    vec3 waterColor=vec3(.00035,.0018,.0039);
    vec3 ray=eye-vec3(.36,-.28,-.22);
    float range=length(ray);
    float beam=smoothstep(.879,.981,-ray.z/max(.001,range));
    if(lamp && beam>0. && range<14. && ray.z<-.15){
        vec3 L=normalize(mat3(gl_TextureMatrix[0])*(-ray));
        float power=31.*beam*(1.-smoothstep(5.5,11.,range))/(1.+.30*range*range);
        color+=vec3(.90,.97,1.)*power*torchVisibility(ray)*brdf(N,V,L,base,rough,metal);
    }
    if(water) color=mix(waterColor,color,exp(-dist*vec3(.172,.090,.063)));
    color+=waterScatter(eye);
    color=color/(1.+color);
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
    if(!program || !shadow.resize(1024,1024) || !torch.resize(768,768)) return false;
    // Geometry and sunlight are static, so the shadow map only needs one pass.
    shadow.bind();glEnable(GL_DEPTH_TEST);glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);lightMatrix();
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();drawGraybox(0);
    bindScreen(1280,720);makePattern();
    shader::bind(program);
    setInt(location(program,"depthMap"),0);setInt(location(program,"patternMap"),1);
    setInt(location(program,"torchMap"),2);
    const char* keys[]={"water","caustics","shadows","lamp"};
    for(int i=0;i<4;++i) flags[i]=location(program,keys[i]);
    clockLocation=location(program,"clockTime");
    return glGetError()==GL_NO_ERROR;
}
void bindEffects(const Effects& e,double time,double x,double y,double z,double yaw,double pitch){
    if(e.lamp){
        GLint viewport[4]; glGetIntegerv(GL_VIEWPORT,viewport);
        shader::bind(0);torch.bind();glClear(GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();
        glFrustum(-.078,.078,-.078,.078,.15,14.);
        glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
        glTranslated(-.36,.28,.22);glRotated(-pitch,1,0,0);glRotated(yaw,0,1,0);glTranslated(-x,-y,-z);
        glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(2.5f,3.f);
        drawGraybox(time);glDisable(GL_POLYGON_OFFSET_FILL);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();
        bindScreen(viewport[2],viewport[3]);shader::bind(program);
    }
    activeTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,torch.depth);
    activeTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,shadow.depth);
    glMatrixMode(GL_TEXTURE);glLoadIdentity();glTranslated(x,y,z);
    glRotated(-yaw,0,1,0);glRotated(pitch,1,0,0);
    activeTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,pattern);lightMatrix();
    activeTexture(GL_TEXTURE0);glMatrixMode(GL_MODELVIEW);
    setInt(flags[0],e.water);setInt(flags[1],e.caustics);setInt(flags[2],e.shadows);setInt(flags[3],e.lamp);
    setFloat(clockLocation,static_cast<float>(time));
}
void drawHandLight(bool on);
namespace {
struct EyePoint { double x,y,z; };
EyePoint projectWaterPoint(const GLdouble* m,double x,double y,double z){
    return {m[0]*x+m[4]*y+m[8]*z+m[12],m[1]*x+m[5]*y+m[9]*z+m[13],
            m[2]*x+m[6]*y+m[10]*z+m[14]};
}
double beamWeight(EyePoint p){
    double depth=-(p.z+.22);
    if(depth<.18 || depth>10.)return 0.;
    double radius=std::hypot(p.x-.36,p.y+.28)/depth;
    return std::clamp(1.-radius/.49,0.,1.)*(1.-depth/11.);
}
void waterTraces(const GLdouble* m,double time){
    constexpr double tau=6.283185307179586;
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    // A surge lifts a brief, translucent veil off ripple crests at three
    // points along the route. The depth buffer occludes it behind solid objects.
    const double sources[][2]={{-2.14,4.3},{-3.15,-3.25},{.40,-5.7}};
    for(int s=0;s<3;++s)for(int j=0;j<20;++j){
        double age=std::fmod(time+j*.29+s*1.7,7.2);
        if(age>4.1)continue;
        double x=sources[s][0]+.28*std::sin(j*2.4)+age*.26;
        double z=sources[s][1]+.32*std::cos(j*2.7)+.12*std::sin(age+j);
        double y=seabedHeight(x,z)+.055+age*.096;
        EyePoint p=projectWaterPoint(m,x,y,z);
        double glow=beamWeight(p)*std::sin(age/4.1*3.14159)*(.5+.5*currentSurge(x,z,time-age));
        if(glow<.002)continue;
        glBegin(GL_TRIANGLE_FAN);glColor4d(.50,.57,.52,.116*glow);
        glVertex3d(p.x,p.y,p.z);glColor4d(.45,.54,.51,0);
        for(int k=0;k<=12;++k){
            double t=k*tau/12,rad=.108+.102*age;
            glVertex3d(p.x+rad*std::cos(t),p.y+rad*.54*std::sin(t),p.z);
        }
        glEnd();
    }
    // Small intermittent releases of trapped air: dark transparent centers
    // and a short glancing reflection instead of a bright outlined circle.
    for(int j=0;j<8;++j){
        double age=std::fmod(time+8.-j*.185,8.);
        if(age>4.3) continue;
        double x=.66+.10*age+.052*std::sin(age*4.+j);
        double z=-8.12+.07*std::sin(age*2.7+j);
        EyePoint p=projectWaterPoint(m,x,2.13+age*(.53+.024*(j%3)),z);
        double light=beamWeight(p)*std::min(1.,age*7.)*std::min(1.,(4.3-age)*3.);
        if(light<.006)continue;
        double r=(.019+.004*(j%4))*(1.+.08*age);
        glBegin(GL_TRIANGLE_STRIP);
        for(int k=0;k<=24;++k){
            double t=tau*k/24.,glint=std::pow(.5+.5*std::cos(t-.91),10.);
            glColor4d(.73,.88,.95,light*(.08+.88*glint));
            glVertex3d(p.x+r*std::cos(t),p.y+r*.89*std::sin(t),p.z);
            glColor4d(.38,.52,.57,0.);
            glVertex3d(p.x+r*.72*std::cos(t),p.y+r*.65*std::sin(t),p.z);
        }
        glEnd();
    }
    glPopMatrix();
}
}
void finishDiveView(bool on,double time){
    shader::bind(0);
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_POINT_BIT|GL_LIGHTING_BIT);
    if(on){
        // World-space silt drifts through the beam. The scene depth buffer
        // hides particles behind wreckage and the range test bounds visibility.
        GLdouble m[16];glGetDoublev(GL_MODELVIEW_MATRIX,m);
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);
        glEnable(GL_POINT_SMOOTH);
        for(int layer=0;layer<2;++layer){
            glPointSize(layer==0?2.1f:4.6f);glBegin(GL_POINTS);
            unsigned state=317u;
            auto random=[&](){state=1664525u*state+1013904223u;return (state>>8)/16777216.;};
            for(int i=0;i<13000;++i){
                double x=-13.+std::fmod(26.*random()+time*.24,26.);
                double y=.08+9.*random()+.09*std::cos(time*.51+i);
                double z=-14.+31.*random()+.12*std::sin(time*.52+i);
                x+=.26*currentSurge(x,z,time);
                double ex=m[0]*x+m[4]*y+m[8]*z+m[12]-.36;
                double ey=m[1]*x+m[5]*y+m[9]*z+m[13]+.28;
                double ez=-(m[2]*x+m[6]*y+m[10]*z+m[14]+.22);
                if(ez<.18 || ez>9. || (ez<2.)!=(layer==1)) continue;
                double radial=(ex*ex+ey*ey)/(ez*ez);
                if(radial>.24) continue;
                glColor4d(.65,.81,.85,(layer?.86:.62)*(1.-radial/.24)*(1.-ez/9.));
                glVertex3d(x,y,z);
            }
            glEnd();
        }
        waterTraces(m,time);
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);
    }
    glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);glEnable(GL_COLOR_MATERIAL);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    const GLfloat direction[]={-.4f,.7f,-.7f,0};
    const GLfloat fill[]={on?.78f:.12f,on?.86f:.16f,on?.9f:.2f,1};
    glLightfv(GL_LIGHT0,GL_POSITION,direction);glLightfv(GL_LIGHT0,GL_DIFFUSE,fill);
    glPopMatrix();drawHandLight(on);
    glPopAttrib();shader::bind(program);
}
void releaseEffects(){
    shader::bind(0);shader::release(program);shadow.release();torch.release();glDeleteTextures(1,&pattern);
}
