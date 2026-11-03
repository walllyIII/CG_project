#include "scene.h"
#include <GLFW/glfw3.h>
#include <array>
#include <cmath>

namespace scene {
namespace {
using Color = std::array<float,3>;
struct Point { float x, y, z; };

// Clay values separate silhouettes without depending on Stage B textures.
constexpr Color stone{.64f,.64f,.61f}, coping{.72f,.72f,.68f};
constexpr Color plinth{.54f,.55f,.53f}, plaster{.86f,.85f,.81f};
constexpr Color wood{.35f,.33f,.30f}, tile{.30f,.32f,.32f};
constexpr Color edgeTile{.41f,.42f,.41f}, reveal{.55f,.55f,.52f};

void quad(Point a, Point b, Point c, Point d, Color color, float shade) {
    glColor3f(color[0]*shade,color[1]*shade,color[2]*shade);
    for (Point p : {a,b,c,d}) glVertex3f(p.x,p.y,p.z);
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
} // namespace

void banks() {
    // Continuous backing crosses y=0, including under joints in the coping.
    box(-12,-2,-12,24,2.25f,8,stone);
    box(-12,-2,4,24,2.25f,8,stone);
    box(-12,-2,-4,6,2.25f,8,stone);
    box(6,-2,-4,6,2.25f,8,stone);
    for (float z : {-4.36f,3.92f})
        for (int i = 0; i < 8; ++i)
            box(-6.36f+1.59f*i,.24f,z,1.58f,.18f,.44f,coping);
    for (float x : {-6.36f,5.92f})
        for (int i = 0; i < 5; ++i)
            box(x,.24f,-3.92f+1.568f*i,.44f,.18f,1.556f,coping);
    // A landing spans the waterline between the coping and first submerged tread.
    box(2.f,-1.55f,3.74f,2.24f,1.70f,.26f,plinth);
}

void submerged() {
    box(-6,-2,-4,12,.45f,8,plinth);
    box(-6,-1.55f,-4,3.7f,.89f,8,stone);
    // Five treads descend into the water from the south-east coping.
    for (int i = 0; i < 5; ++i) {
        float top = -.12f-.24f*i;
        box(2.00f,-1.55f,3.26f-.48f*i,2.24f,1.55f+top,.48f,coping);
    }
    box(-4.45f,-.66f,.65f,1.2f,.20f,.67f,plinth);
    box(.75f,-1.55f,-1.40f,.96f,.34f,.65f,stone);
}

void courtyard() {
    banks();
    walls();
    gallery();
    pavilion();
}
} // namespace scene
