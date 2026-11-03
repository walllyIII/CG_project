#include "scene.h"
#include "material.h"
#include "shader.h"
#include <GLFW/glfw3.h>
#include <array>
#include <cmath>
#include <utility>

namespace scene {
namespace {
struct Color {
    float r,g,b;
    Surface surface = Plain;
};
struct Point { float x, y, z; };

constexpr Color stone{.49f,.52f,.50f,Stone}, coping{.56f,.61f,.59f,Coping};
constexpr Color plinth{.36f,.40f,.38f,Stone}, plaster{.94f,.92f,.84f,Limewash};
constexpr Color wood{.35f,.21f,.14f,Timber}, tile{.26f,.29f,.30f,RoofTile};
constexpr Color edgeTile{.37f,.40f,.40f,Stone}, reveal{.47f,.50f,.47f,Stone};
constexpr Color paving{.59f,.60f,.55f,Paving}, veins{.43f,.48f,.42f,VeinedStone};
GLuint materialProgram, sceneLists;

void quad(Point a, Point b, Point c, Point d, Color color, float shade) {
    const Point ab{b.x-a.x,b.y-a.y,b.z-a.z}, ad{d.x-a.x,d.y-a.y,d.z-a.z};
    float nx=std::abs(ab.y*ad.z-ab.z*ad.y), ny=std::abs(ab.z*ad.x-ab.x*ad.z);
    float nz=std::abs(ab.x*ad.y-ab.y*ad.x);
    bool horizontal = ny >= nx && ny >= nz;
    float spanU = nx>nz ? std::abs(ab.z)+std::abs(ad.z) : std::abs(ab.x)+std::abs(ad.x);
    float spanV = std::abs(ab.y)+std::abs(ad.y);
    if (horizontal) {
        spanU=std::abs(ab.x)+std::abs(ad.x); spanV=std::abs(ab.z)+std::abs(ad.z);
    }
    glColor3f(color.r*shade,color.g*shade,color.b*shade);
    for (Point p : {a,b,c,d}) {
        float u=horizontal || nx<=nz ? p.x : p.z;
        float v=horizontal ? p.z : p.y;
        if ((color.surface==Timber && spanV>spanU) || (color.surface==RoofTile && horizontal && nx>nz))
            std::swap(u,v);
        glTexCoord3f(u,v,static_cast<float>(color.surface));
        glVertex3f(p.x,p.y,p.z);
    }
}

void box(float x,float y,float z,float w,float h,float d,Color color) {
    const Point a{x,y,z}, b{x+w,y,z}, c{x+w,y+h,z}, e{x,y+h,z};
    const Point f{x,y,z+d}, g{x+w,y,z+d}, i{x+w,y+h,z+d}, j{x,y+h,z+d};
    glBegin(GL_QUADS);
    quad(a,b,c,e,color,.73f); quad(f,g,i,j,color,.87f);
    quad(a,f,j,e,color,.68f); quad(b,g,i,c,color,.79f);
    quad(e,c,i,j,color,1);    quad(a,b,g,f,color,.53f);
    glEnd();
}

// Fold a rectangular roof up to a short ridge. The eave sweeps up at its ends.
void roof(float cx,float cz,float halfX,float halfZ,float y,float rise) {
    constexpr int bays = 12;
    auto ring = [=](int side,float u,float level) -> Point {
        const float rx = halfX*(1-.38f*level), rz = halfZ*(1-level);
        float x = 0, z = 0;
        if (side == 0 || side == 2) {
            x = u*rx; z = side == 0 ? rz : -rz;
        } else {
            x = side == 1 ? rx : -rx; z = u*rz;
        }
        const float corner = std::pow(std::abs(x/rx),4.f) * std::abs(z/halfZ);
        return {cx+x, y+rise*(level*level*.70f+level*.30f+.24f*corner), cz+z};
    };
    glBegin(GL_QUADS);
    for (int side = 0; side < 4; ++side) {
        for (int i = 0; i < bays; ++i) {
            float u = 2.f*i/bays-1, v = 2.f*(i+1)/bays-1;
            Point a = ring(side,u,0), b = ring(side,v,0);
            Point lowA = a, lowB = b;
            lowA.y -= .12f; lowB.y -= .12f;
            quad(lowA,lowB,b,a,edgeTile,.86f);
            for (int j = 1; j <= 5; ++j) {
                Point c = ring(side,v,j/5.f), d = ring(side,u,j/5.f);
                quad(a,b,c,d,tile,.72f+.045f*j+.04f*(side%2));
                a = d; b = c;
            }
        }
    }
    glEnd();
    // Soffit over the occupied part of the roof.
    box(cx-halfX*.90f,y-.14f,cz-halfZ*.86f,1.80f*halfX,.10f,1.72f*halfZ,wood);
    box(cx-halfX*.63f,y+rise-.03f,cz-.07f,1.26f*halfX,.15f,.14f,edgeTile);
}

void column(float x,float z,float floor,float top) {
    box(x-.21f,floor,z-.21f,.42f,.13f,.42f,stone);
    box(x-.16f,floor+.13f,z-.16f,.32f,.16f,.32f,plinth);
    box(x-.11f,floor+.29f,z-.11f,.22f,top-floor-.29f,.22f,wood);
    box(x-.21f,top-.22f,z-.16f,.42f,.22f,.32f,wood);
}

void pavilion() {
    box(-4.65f,.25f,-7.65f,7.2f,.49f,3.65f,plinth);
    box(-4.80f,.74f,-7.78f,7.50f,.12f,3.88f,coping);
    for (float x : {-4.25f,-2.10f,.05f,2.20f}) {
        column(x,-4.39f,.86f,3.02f);
        column(x,-7.24f,.86f,3.02f);
        box(x-.12f,2.92f,-7.52f,.24f,.20f,3.44f,wood);
    }
    for (float z : {-7.24f,-4.39f})
        box(-4.51f,2.91f,z-.13f,6.97f,.27f,.26f,wood);
    // A gap in the front rail keeps the middle bay and its reflection readable.
    for (float x : {-4.25f,.05f}) {
        for (float y : {1.05f,1.48f}) box(x,y,-4.43f,2.15f,.08f,.08f,wood);
        for (int i = 1; i < 5; ++i) box(x+i*.43f,1.12f,-4.42f,.055f,.36f,.06f,wood);
    }
    roof(-1.03f,-5.80f,3.99f,1.98f,3.22f,1.00f);
}

void gallery() {
    // The west walk meets the pavilion via a low landing at its north end.
    box(-8.21f,.25f,-7.65f,3.56f,.42f,2.75f,plinth);
    box(-8.21f,.25f,-4.90f,1.88f,.42f,7.05f,plinth);
    glPushMatrix();
    glTranslatef(-7.27f,0,-2.72f);
    glRotatef(90,0,1,0);
    for (float x : {-4.4f,-2.2f,0.f,2.2f,4.4f})
        for (float z : {-.70f,.70f}) column(x,z,.67f,2.47f);
    for (float z : {-.70f,.70f})
        box(-4.6f,2.37f,z-.10f,9.2f,.18f,.2f,wood);
    roof(0,0,4.92f,1.10f,2.60f,.50f);
    glPopMatrix();
}

// Open window bays are assembled around a void. A second wall behind them
// provides a parallax reference for the patterned mullions.
void windowBay(float x,int pattern) {
    constexpr float z = -8.9f, sill = 1.04f, lintel = 2.62f, width = 2.10f;
    box(x,.25f,z,width,sill-.25f,.32f,plaster);
    box(x,lintel,z,width,.59f,.32f,plaster);
    box(x-.05f,sill-.075f,z-.03f,width+.1f,.075f,.40f,reveal);
    for (float dx : {0.f,width-.08f})
        box(x+dx,sill,z+.09f,.08f,lintel-sill,.13f,reveal);
    float center = x+width*.5f, mid = (sill+lintel)*.5f;
    if (pattern == 1) {
        glPushMatrix();
        glTranslatef(center,mid,z+.17f);
        glRotatef(45,0,0,1);
        for (float pos : {-.54f,.48f}) {
            box(pos,-.54f,-.05f,.06f,1.08f,.10f,wood);
            box(-.54f,pos,-.05f,1.08f,.06f,.10f,wood);
        }
        glPopMatrix();
        for (float dx : {.08f,1.79f})
            box(x+dx,mid-.035f,z+.12f,.23f,.07f,.10f,wood);
    } else {
        for (float dx : {.42f,1.62f})
            box(x+dx,sill,z+.12f,.06f,lintel-sill,.10f,wood);
        for (float y : {sill+.30f,lintel-.36f})
            box(x+.08f,y,z+.12f,width-.16f,.06f,.10f,wood);
    }
}

void walls() {
    constexpr float z = -8.9f;
    box(-11,.25f,z,13.6f,2.96f,.32f,plaster);
    for (int i = 0; i < 3; ++i) {
        float x = 2.6f+i*2.64f;
        windowBay(x,i%2);
        box(x+2.1f,.25f,z,.54f,2.96f,.32f,plaster);
    }
    box(-11,3.21f,z-.12f,21.52f,.17f,.56f,tile);
    box(-11,.25f,z+.32f,.32f,2.5f,14.0f,plaster);
    box(-11.1f,2.75f,z+.30f,.52f,.14f,14.1f,tile);
    box(1.6f,.25f,-11.3f,10.4f,1.45f,.30f,plinth);
    box(1.55f,1.70f,-11.37f,10.5f,.13f,.45f,edgeTile);
}

struct Sprout {
    unsigned int seed;
    float unit() {
        seed=1664525u*seed+1013904223u;
        return float(seed>>8)/16777216.f;
    }
    float signedUnit() { return 2.f*unit()-1.f; }
};

Point blend(Point a, Point b, float t) {
    return {a.x+t*(b.x-a.x),a.y+t*(b.y-a.y),a.z+t*(b.z-a.z)};
}

void branch(Point a,Point b,float foot,float tip,Color color) {
    // Two small horizontal rings build a leaning, tapered stem.
    constexpr int sides=6;
    glBegin(GL_QUADS);
    for (int i=0;i<sides;++i) {
        float angle=i*6.2831853f/sides, next=(i+1)*6.2831853f/sides;
        float x=std::cos(angle), z=std::sin(angle), u=std::cos(next), v=std::sin(next);
        quad({a.x+foot*x,a.y,a.z+foot*z},{a.x+foot*u,a.y,a.z+foot*v},
             {b.x+tip*u,b.y,b.z+tip*v},{b.x+tip*x,b.y,b.z+tip*z},
             color,.82f+.13f*z);
    }
    glEnd();
}

void leaf(Point root,float angle,float length,float width,float rise,Color green) {
    float x=std::cos(angle), z=std::sin(angle);
    Point mid{root.x+.43f*length*x,root.y+rise*.48f+.055f,root.z+.43f*length*z};
    Point tip{root.x+length*x,root.y+rise,root.z+length*z};
    glBegin(GL_TRIANGLES);
    glTexCoord3f(0,0,Plain);
    for (int side : {-1,1}) {
        float light=side<0 ? .85f : 1.f;
        glColor3f(green.r*light,green.g*light,green.b*light);
        glVertex3f(root.x,root.y,root.z);
        glVertex3f(mid.x-side*z*width,mid.y-.022f,mid.z+side*x*width);
        glVertex3f(tip.x,tip.y,tip.z);
    }
    glEnd();
}

void bamboo(float x,float z,int stems,unsigned int seed,float scale) {
    Sprout rng{seed};
    constexpr Color reed{.37f,.43f,.24f}, node{.48f,.50f,.31f};
    for (int i=0;i<stems;++i) {
        Point root{x+.85f*rng.signedUnit(),.25f,z+.33f*rng.signedUnit()};
        float h=scale*(.76f+.36f*rng.unit());
        Point crown{root.x+.82f*rng.signedUnit(),root.y+h,root.z+.50f*rng.signedUnit()};
        branch(root,crown,.035f,.013f,reed);
        for (int j=1;j<=9;++j) {
            float t=j/10.f;
            Point joint=blend(root,crown,t);
            float radius=.040f-.022f*t;
            branch(joint,{joint.x,joint.y+.035f,joint.z},radius,radius,node);
            if (j<4) continue;
            float angle=(i*2.4f+j*2.7f)+.3f*rng.signedUnit();
            float reach=(1.25f-.7f*t)*(.8f+.5f*rng.unit());
            Point twig{joint.x+reach*std::cos(angle),joint.y+.32f+.32f*rng.unit(),
                       joint.z+reach*std::sin(angle)};
            branch(joint,twig,.014f,.004f,reed);
            for (int k=1;k<=7;++k) {
                Point base=blend(joint,twig,k/8.f);
                float tone=rng.unit();
                Color foliage{.18f+.12f*tone,.29f+.12f*tone,.20f+.07f*tone};
                leaf(base,angle+(k%2 ? -.75f:.75f),.29f+.25f*rng.unit(),
                     .043f+.020f*rng.unit(),-.09f-.23f*rng.unit(),foliage);
            }
        }
    }
}

void leaningTree() {
    constexpr Color bark{.29f,.25f,.20f,Timber};
    Point fork{-7.9f,2.05f,2.1f};
    branch({-8.65f,.25f,2.3f},fork,.19f,.105f,bark);
    Sprout rng{713};
    for (int i=0;i<6;++i) {
        float angle=i*2.4f;
        float reach=.75f+.6f*rng.unit();
        Point elbow{fork.x+reach*std::cos(angle),fork.y+.7f+.65f*rng.unit(),fork.z+reach*std::sin(angle)};
        branch(fork,elbow,.09f,.044f,bark);
        for (int j=-1;j<=1;++j) {
            float a=angle+j*.73f;
            Point end{elbow.x+.75f*std::cos(a),elbow.y+.28f+.38f*rng.unit(),elbow.z+.75f*std::sin(a)};
            branch(elbow,end,.04f,.012f,bark);
            for (int k=0;k<6;++k) {
                Point base=blend(elbow,end,(k+1)/7.f);
                float fan=a+(k%2 ? -.9f:.9f);
                Point shoot{base.x+.39f*std::cos(fan),base.y+.18f,base.z+.39f*std::sin(fan)};
                branch(base,shoot,.012f,.004f,bark);
                for (int m=1;m<=5;++m)
                    leaf(blend(base,shoot,m/5.f),fan+.8f*rng.signedUnit(),
                         .25f+.13f*rng.unit(),.065f,.05f*rng.signedUnit(),
                         {.25f,.34f+.09f*rng.unit(),.23f});
            }
        }
    }
}

void rock(float x,float y,float z,float width,float height,float depth,Color color) {
    constexpr std::array<float,7> radii{1,.83f,1.05f,.88f,.96f,.76f,1.02f};
    for (int i=0;i<7;++i) {
        auto corner=[&](int n,float size,float level) -> Point {
            float angle=n*.897598f, radius=radii[n]*size;
            return {x+radius*width*std::cos(angle)+.16f*level*width,y+level*height,
                    z+radius*depth*std::sin(angle)};
        };
        int j=(i+1)%7;
        Point a=corner(i,1,0), b=corner(j,1,0);
        Point c=corner(j,.74f,.70f), d=corner(i,.74f,.70f);
        glBegin(GL_QUADS);
        quad(a,b,c,d,color,.73f+.12f*std::sin(i*.9f));
        quad(d,c,{x+.2f*width,y+height,z},{x+.2f*width,y+height,z},color,.92f);
        glEnd();
    }
}
} // namespace

void drawBanks() {
    // Continuous backing crosses y=0, including under joints in the coping.
    box(-12,-2,-12,24,2.25f,8,paving);
    box(-12,-2,4,24,2.25f,8,paving);
    box(-12,-2,-4,6,2.25f,8,paving);
    box(6,-2,-4,6,2.25f,8,paving);
    for (float z : {-4.36f,3.92f})
        for (int i = 0; i < 8; ++i)
            box(-6.36f+1.59f*i,.24f,z,1.58f,.18f,.44f,coping);
    for (float x : {-6.36f,5.92f})
        for (int i = 0; i < 5; ++i)
            box(x,.24f,-3.92f+1.568f*i,.44f,.18f,1.556f,coping);
    // A landing spans the waterline between the coping and first submerged tread.
    box(2.f,-1.55f,3.74f,2.24f,1.70f,.26f,plinth);
}

void drawSubmerged() {
    box(-6,-2,-4,12,.45f,8,plinth);
    box(-6,-1.55f,-4,3.7f,.89f,8,stone);
    // Five treads descend into the water from the south-east coping.
    for (int i = 0; i < 5; ++i) {
        float top = -.12f-.24f*i;
        box(2.00f,-1.55f,3.26f-.48f*i,2.24f,1.55f+top,.48f,i < 3 ? veins : stone);
    }
    rock(-3.85f,-.66f,1.05f,.80f,.23f,.47f,veins);
    rock(1.22f,-1.55f,-1.05f,.65f,.32f,.43f,stone);
}

void drawArchitecture() {
    walls();
    gallery();
    pavilion();
    bamboo(4.7f,-9.66f,9,19,4.25f);
    bamboo(8.5f,-9.8f,5,23,3.65f);
    bamboo(-5.8f,-7.95f,6,51,3.8f);
    leaningTree();
    rock(-8.2f,.25f,2.55f,.76f,.48f,.42f,plinth);
    rock(-7.5f,.25f,2.38f,.44f,.29f,.32f,stone);
}
// Cache the static geometry once. The same lists feed the three render passes.
bool initialize() {
    materialProgram=createMaterialProgram();
    if (!materialProgram) return false;
    sceneLists=glGenLists(3);
    if (!sceneLists) return false;
    glNewList(sceneLists,GL_COMPILE); drawArchitecture(); glEndList();
    glNewList(sceneLists+1,GL_COMPILE); drawBanks(); glEndList();
    glNewList(sceneLists+2,GL_COMPILE); drawSubmerged(); glEndList();
    return glGetError()==GL_NO_ERROR;
}
void bindMaterials() { shader::bind(materialProgram); }
void banks() { glCallList(sceneLists+1); }
void submerged() { glCallList(sceneLists+2); }
void courtyard() { banks(); glCallList(sceneLists); }
void release() {
    if (sceneLists) glDeleteLists(sceneLists,3);
    if (materialProgram) shader::release(materialProgram);
}
} // namespace scene
