#include <GLFW/glfw3.h>
#include <cmath>
#include <initializer_list>

namespace {
constexpr double pi=3.141592653589793;
struct Point { double x,y,z; };
Point operator+(Point a,Point b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Point operator-(Point a,Point b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Point operator*(Point a,double k){return {a.x*k,a.y*k,a.z*k};}
Point cross(Point a,Point b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Point unit(Point a){return a*(1/std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z));}
void face(std::initializer_list<Point> pts){
    const Point* p=pts.begin();
    Point n=unit(cross(p[1]-p[0],p[2]-p[0]));
    glBegin(GL_POLYGON); glNormal3d(n.x,n.y,n.z);
    for(Point v:pts) glVertex3d(v.x,v.y,v.z);
    glEnd();
}
void spar(Point a,Point b,double ra,double rb,bool cap=true){
    Point axis=unit(b-a);
    Point u=unit(cross(axis,std::abs(axis.y)<.9?Point{0,1,0}:Point{1,0,0}));
    Point v=cross(axis,u);
    for(int i=0;i<12;++i){
        double s=i*pi/6,t=(i+1)*pi/6;
        Point e=u*std::cos(s)+v*std::sin(s), f=u*std::cos(t)+v*std::sin(t);
        face({a+e*ra,a+f*ra,b+f*rb,b+e*rb});
        if(cap && rb>0) face({b,b+e*rb,b+f*rb});
    }
}
void hull(){
    glTexCoord2f(.55f,0);glColor3f(.27f,.13f,.061f);
    const double stations[]={-8,-6,-3,0,2,5,8,9};
    const double beam[]={1.5,2.2,2.45,2.5,2.45,2.05,.9,.08};
    const double rim[]={4.4,4.0,3.6,3.55,3.55,3.8,4.25,4.75};
    for(int i=0;i<7;++i){
        for(int side:{-1,1}){
            auto at=[&](int j,double h){
                double keelX=stations[j];
                if(j>=5) keelX-=(stations[j]-5)*.85;
                if(j==0) keelX+=1.15;
                return Point{keelX+(stations[j]-keelX)*h,.12+(rim[j]-.12)*h,side*beam[j]*(.24+.76*h)};
            };
            face({at(i,0),at(i+1,0),at(i+1,.36),at(i,.36)});
            // Leave a full-height gap above the lower strake on the approach side.
            if(side<0 || i!=3) face({at(i,.36),at(i+1,.36),at(i+1,1),at(i,1)});
            if(side>0 && i==3){
                face({at(i,.36),at(i+1,.36),at(i+1,.53),at(i,.44)});
                face({at(i,.44),at(i,.92),at(i,.92)+Point{.43,-.72,-.03}});
            }
        }
    }
    face({{-6.85,.12,-.36},{-6.85,.12,.36},{-8,4.4,1.5},{-8,4.4,-1.5}});
    glColor3f(.23f,.11f,.05f);
    spar({-2.8,1,0},{-4.4,8.25,.25},.32,.20);
    spar({-4.3,7.6,.2},{-5.15,8.75,.22},.14,.018);
    spar({-7.3,6.75,.32},{-.8,5.95,.30},.14,.10);
    spar({-6.9,6.7,.32},{-7.6,4.1,1.25},.045,.045);
    spar({-.85,5.95,.3},{1.2,3.1,-1.6},.05,.05);
}
void cannon(){
    // All dimensions are ship-local. The muzzle ends inside the broken side.
    glPushMatrix();
    glTranslated(1.12,2.53,.12);
    glRotated(-18,0,1,0);
    glTexCoord2f(.60f,0);glColor3f(.20f,.09f,.04f);
    for(double x:{-.31,.31}){
        spar({x,-.38,-.76},{x,-.38,.50},.17,.17);
        for(double z:{-.59,.38}){
            double side=x>0?1:-1;
            spar({x,-.57,z},{x+side*.20,-.57,z},.22,.22);
        }
    }
    glTexCoord2f(.26f,.95f);glColor3f(.33f,.36f,.37f);
    spar({0,0,-1.22},{0,0,1.18},.40,.29,false);
    spar({0,0,-1.27},{0,0,-1.08},.43,.43);
    spar({0,0,-.62},{0,0,-.53},.42,.42);
    spar({0,0,1.07},{0,0,1.33},.35,.35,false);
    // Model the annular lip and recessed bore so the opening reads at close range.
    for(int i=0;i<12;++i){
        auto rim=[](int j,double radius,double z){
            return Point{radius*std::cos(j*pi/6),radius*std::sin(j*pi/6),z};
        };
        glColor3f(.34f,.35f,.35f);
        face({rim(i,.35,1.33),rim(i+1,.35,1.33),rim(i+1,.21,1.33),rim(i,.21,1.33)});
        glColor3f(.09f,.10f,.11f);
        face({rim(i,.21,1.33),rim(i+1,.21,1.33),rim(i+1,.21,.88),rim(i,.21,.88)});
        face({{0,0,.88},rim(i,.21,.88),rim(i+1,.21,.88)});
    }
    glPopMatrix();
    glTexCoord2f(.58f,0);glColor3f(.18f,.085f,.037f);
    face({{-.72,1.74,-1.6},{-.72,1.74,1.5},{2.75,1.74,1.5},{2.75,1.74,-1.6}});
}
void anchor(){
    glPushMatrix();glTranslated(-2.9,.18,5.6);glRotated(-22,0,1,0);
    glRotated(20,1,0,0);
    glTexCoord2f(.56f,.62f);glColor3f(.24f,.26f,.26f);
    spar({0,0,0},{0,.15,-2.05},.14,.11);
    spar({-.65,.12,-1.52},{.65,.12,-1.52},.09,.09);
    for(int s:{-1,1}){
        spar({0,0,0},{s*.68,.04,.15},.15,.13);
        spar({s*.68,.04,.15},{s*1.02,.2,-.33},.13,.08);
        face({{s*.69,.18,-.22},{s*1.12,.16,-.14},{s*1.02,.24,-.76}});
    }
    for(int i=0;i<12;++i){
        auto ring=[](int j){double t=j*pi/6;return Point{.22*std::cos(t),.15,-2.23+.22*std::sin(t)};};
        spar(ring(i),ring(i+1),.045,.045);
    }
    glPopMatrix();
}
double bed(double x,double z){return -.13+.13*std::sin(.32*x+.27*z)+.06*std::cos(.53*x-.39*z);}
}
void drawGraybox(){
    // Low relief on a neutral seabed provides scale without finished materials.
    for(int z=-70;z<36;z+=2) for(int x=-60;x<60;x+=2){
        double shade=.55+.004*std::sin(x*.72+z*.57);
        glTexCoord2f(.93f,0);glColor3d(shade,shade*.79,shade*.49);
        Point a{x*1.,bed(x,z),z*1.}, b{x*1.,bed(x,z+2),z+2.},
              c{x+2.,bed(x+2,z+2),z+2.}, d{x+2.,bed(x+2,z),z*1.};
        face({a,b,c});face({a,c,d});
    }
    glPushMatrix();glTranslated(.6,-.48,-9.6);glRotated(-18,0,1,0);glRotated(-12,1,0,0);
    hull();cannon();glPopMatrix();
    anchor();
    // A few exposed chain links lead from the anchor ring toward the gap.
    glTexCoord2f(.60f,.55f);glColor3f(.32f,.33f,.33f);
    for(int i=0;i<32;++i){
        double z=3.4-i*.34, x=-2.1+i*.105;
        double y=bed(x,z)+.055+1.03*std::exp(-.52*i);
        auto link=[&](int j){
            double t=2*pi*j/10, across=.11*std::cos(t), along=.20*std::sin(t);
            return Point{x+across*(i%2?.5:1)-along*.28,
                         y+(i%2?across*.82:0),z+along};
        };
        for(int j=0;j<10;++j) spar(link(j),link(j+1),.032,.032);
    }
}
