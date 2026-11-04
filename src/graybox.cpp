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
void spar(Point a,Point b,double ra,double rb,bool cap=true,int sides=12){
    Point axis=unit(b-a);
    Point u=unit(cross(axis,std::abs(axis.y)<.9?Point{0,1,0}:Point{1,0,0}));
    Point v=cross(axis,u);
    for(int i=0;i<sides;++i){
        double s=i*2*pi/sides,t=(i+1)*2*pi/sides;
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
            // Overlapping strakes have visible upper edges and a staggered,
            // splintered end on either side of the gun opening.
            for(int row=0;row<10;++row){
                double low=row*.10, high=low+.098;
                bool torn=side>0 && i==3 && row>=5;
                double tone=.91+.13*std::sin(i*4.1+row*1.9);
                glColor3d(.29*tone,.142*tone,.067*tone);
                auto board=[&](double u,double h){
                    Point a=at(i,h), b=at(i+1,h);
                    Point p=a+(b-a)*u;
                    p.z+=side*(.04+.018*std::sin(row*2.3));
                    return p;
                };
                auto section=[&](double lowerStart,double lowerEnd,double upperStart,double upperEnd){
                    Point a=board(lowerStart,low),b=board(lowerEnd,low);
                    Point c=board(upperEnd,high),d=board(upperStart,high);
                    Point inset{0,0,-side*.12};
                    face({a,b,c,d});face({d,c,c+inset,d+inset});
                    face({a,d,d+inset,a+inset});face({b,b+inset,c+inset,c});
                };
                if(torn){
                    double stub=.06+.055*(1+std::sin(row*2.7));
                    section(0,stub+.055,0,stub*.4);
                    section(.91-.045*std::cos(row*3.),1,.97,1);
                }else section(.002,.998,.002,.998);
            }
            if(i!=3 || side<0){
                glColor3f(.19f,.09f,.038f);
                spar(at(i,.98),at(i+1,.98),.085,.085);
            }

        }
    }
    face({{-6.85,.12,-.36},{-6.85,.12,.36},{-8,4.4,1.5},{-8,4.4,-1.5}});
    // Raised stern rail and exposed frames preserve a legible wreck silhouette.
    for(int j=0;j<3;++j){
        double x=-7.8+j*.88, top=4.38-j*.16;
        spar({x,top-.1,1.51+j*.25},{x,top+.52,1.51+j*.25},.075,.065);
    }
    spar({-7.8,4.9,1.51},{-5.95,4.58,2.03},.075,.065);
    for(int j:{1,2,4,5}){
        glColor3f(.17f,.088f,.044f);
        spar({stations[j],1.4,-beam[j]*.5},{stations[j],rim[j]+.12,-beam[j]},.13,.105);
    }
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
    glTexCoord2f(.34f,.95f);glColor3f(.33f,.36f,.37f);
    spar({0,0,-1.22},{0,0,1.18},.40,.29,false,32);
    spar({0,0,-1.66},{0,0,-1.26},.12,.34,true,24);
    spar({0,0,-1.74},{0,0,-1.61},.13,.13,true,24);
    spar({-.57,-.05,-.28},{.57,-.05,-.28},.115,.115,true,24);
    for(double z:{-.95,-.78,.45,.54}){
        double r=z<0?.415:.345;
        spar({0,0,z},{0,0,z+.052},r,r,true,32);
    }
    spar({0,0,-1.27},{0,0,-1.08},.43,.43);
    spar({0,0,-.62},{0,0,-.53},.42,.42);
    spar({0,0,1.07},{0,0,1.33},.35,.35,false,32);
    // Model the annular lip and recessed bore so the opening reads at close range.
    for(int i=0;i<32;++i){
        auto rim=[](int j,double radius,double z){
            return Point{radius*std::cos(j*pi/16),radius*std::sin(j*pi/16),z};
        };
        glColor3f(.34f,.35f,.35f);
        face({rim(i,.35,1.33),rim(i+1,.35,1.33),rim(i+1,.21,1.33),rim(i,.21,1.33)});
        glColor3f(.09f,.10f,.11f);
        face({rim(i,.21,1.33),rim(i+1,.21,1.33),rim(i+1,.21,.88),rim(i,.21,.88)});
        face({{0,0,.88},rim(i,.21,.88),rim(i+1,.21,.88)});
    }
    glPopMatrix();
    glTexCoord2f(.58f,0);glColor3f(.18f,.085f,.037f);
    for(int j=0;j<9;++j){
        double x=-.72+.39*j;
        face({{x,1.74,-1.6},{x,1.74,1.5},{x+.38,1.74,1.5},{x+.38,1.74,-1.6}});
    }
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
// Rectangular timbers keep a closed end grain and cast solid torch shadows.
void timber(Point c,double w,double h,double d){
    Point a{c.x-w/2,c.y-h/2,c.z-d/2}, b{a.x+w,a.y,a.z};
    Point e{0,h,0}, f{0,0,d};
    face({a,b,b+e,a+e});face({a+f,a+e+f,b+e+f,b+f});
    face({a+e,b+e,b+e+f,a+e+f});face({a,a+f,b+f,b});
    face({a,a+e,a+e+f,a+f});face({b,b+f,b+e+f,b+e});
}
void ellipsoid(Point c,Point radii){
    auto vertex=[&](int i,int j){
        double u=i*pi/10,v=-pi/2+j*pi/10;
        return Point{c.x+radii.x*std::cos(u)*std::cos(v),c.y+radii.y*std::sin(v),c.z+radii.z*std::sin(u)*std::cos(v)};
    };
    for(int j=0;j<10;++j) for(int i=0;i<20;++i){
        if(j>0) face({vertex(i,j),vertex(i+1,j),vertex(i+1,j+1)});
        if(j<9) face({vertex(i,j),vertex(i+1,j+1),vertex(i,j+1)});
    }
}
void pirateCrest(){
    // A carved escutcheon bolted beside the breach: crossed bones, a domed
    // cranium, inset dark sockets and individual teeth stand proud of the wood.
    glPushMatrix();glTranslated(-.93,2.8,2.23);glRotated(23,1,0,0);
    glTexCoord2f(.59f,0);glColor3f(.085f,.038f,.019f);
    ellipsoid({0,0,0},{.74,.69,.115});
    glTexCoord2f(.49f,.35f);glColor3f(.41f,.27f,.095f);
    for(int k=0;k<24;++k){
        auto rim=[](int i){return Point{.70*std::cos(i*pi/12),.65*std::sin(i*pi/12),.072};};
        spar(rim(k),rim(k+1),.025,.025);
    }
    glTexCoord2f(.76f,0);glColor3f(.69f,.57f,.37f);
    for(int sign:{-1,1}){
        Point a{-.45,-.30*sign,.13},b{.45,.30*sign,.13};
        spar(a,b,.057,.057);
        for(Point tip:{a,b}){ellipsoid(tip+Point{0,.045,0},{.07,.063,.05});ellipsoid(tip-Point{0,.045,0},{.07,.063,.05});}
    }
    ellipsoid({0,.13,.19},{.30,.285,.14});
    for(int i=-2;i<=2;++i) timber({i*.068,-.13,.245},.053,.099,.083);
    glColor3f(.024f,.018f,.012f);
    for(int sign:{-1,1}) ellipsoid({sign*.126,.13,.312},{.085,.082,.026});
    face({{0,.09,.337},{-.047,-.012,.308},{.047,-.012,.308}});
    glPopMatrix();
}
void wreckFittings(){
    pirateCrest();
    glTexCoord2f(.54f,0);glColor3f(.19f,.088f,.042f);
    // Broken stern gallery with framed window bays and a projecting cornice.
    for(int j=0;j<5;++j) timber({-7.7+j*.52,4.72,1.43},.12,1.26,.20);
    timber({-6.66,4.21,1.43},2.27,.16,.24);
    timber({-6.66,5.31,1.43},2.45,.19,.34);
    for(int k=0;k<3;++k){
        spar({-6.8+k*.28,4.25,1.25},{-4.4+k*.08,7.8,0},.025,.025);
    }
    // Torn pennant folds through depth; the loose edge splits into streamers.
    glTexCoord2f(.85f,0);glColor3f(.16f,.04f,.035f);
    for(int i=0;i<11;++i){
        double x=-4.36+i*.19,y=8.14-i*.055,z=.22*std::sin(i*.8);
        double zn=.22*std::sin((i+1)*.8),drop=.70-.045*i;
        face({{x,y,z},{x+.19,y-.055,zn},{x+.19,y-.055-drop,zn},{x,y-drop,z}});
        if(i>=8) face({{x,y-drop,z},{x+.41,y-drop-.26,zn+.08},{x+.15,y-drop+.02,zn}});
    }
}
void barrel(double x,double z,double yaw,double scale){
    glPushMatrix();glTranslated(x,bed(x,z)+.46*scale,z);glRotated(yaw,0,1,0);
    glRotated(73,1,0,0);glScaled(scale,scale,scale);
    for(int j=0;j<14;++j){
        double tone=.70+.27*std::cos(j*5.2);
        glTexCoord2f(.53f,0);glColor3d(.33*tone,.17*tone,.071*tone);
        auto stave=[&](int end,double y,double r){
            double a=(j+.025+end*.95)*2*pi/14;
            return Point{r*std::cos(a),y,r*std::sin(a)};
        };
        for(int half:{-1,1})face({stave(0,0,.49),stave(1,0,.49),stave(1,half*.59,.39),stave(0,half*.59,.39)});
    }
    spar({0,-.57,0},{0,.57,0},.38,.38,true,14);
    glTexCoord2f(.52f,.87f);glColor3f(.24f,.22f,.17f);
    for(double y:{-.45,-.16,.19,.44}){
        double r=.505-.17*std::abs(y);
        spar({0,y-.033,0},{0,y+.033,0},r,r,false,14);
    }
    glPopMatrix();
}
void spill(){
    barrel(-1.8,-1.1,48,1.18);barrel(-2.8,-2.,-32,.91);barrel(2.3,-4.7,64,1.);
    barrel(1.7,3.8,22,.81);
    // Cargo crates of different sizes, with individual slats and inset seams.
    for(int i=0;i<4;++i){
        const double xs[]={.62,-2.8,2.9,-3.4},zs[]={2.8,-3.5,-3.,-4.8};
        double x=xs[i],z=zs[i];
        glPushMatrix();glTranslated(x,bed(x,z)+.31,z);glRotated(22+i*29,0,1,0);glRotated(i*4,0,0,1);
        glTexCoord2f(.59f,0);
        for(int j=0;j<5;++j){
            glColor3d(.29+j*.012,.137+j*.009,.058+j*.006);
            timber({(j-2)*.22,0,0},.214,.75,.79);
        }
        glColor3f(.14f,.073f,.031f);
        for(double end:{-.41,.41}){
            timber({end,0,0},.11,.82,.84);
        }
        glPushMatrix();glRotated(29,0,0,1);timber({0,0,.409},1.10,.095,.08);glPopMatrix();
        glPopMatrix();
    }
    for(int j=0;j<32;++j){
        double x=3.7*std::sin(j*7.31),z=4.4-10.*(.5+.5*std::cos(j*2.32));
        glPushMatrix();glTranslated(x,bed(x,z)+.025,z);glRotated(j*97.,0,1,0);glRotated(4.*std::sin(j),0,0,1);
        glTexCoord2f(.57f,0);glColor3d(.19+.025*(j%4),.092,.044);
        timber({0,0,0},.46+.29*(j%6),.09,.12+.025*(j%5));glPopMatrix();
    }
    glTexCoord2f(.43f,.91f);glColor3f(.21f,.19f,.17f);
    for(int j=0;j<9;++j){
        double x=1.3+.85*std::sin(j*4.4),z=-3.9+.8*std::cos(j*2.5);
        ellipsoid({x,bed(x,z)+.13,z},{.17,.17,.17});
    }
}
void seabedDetails(){
    // Low, asymmetric stones collect in patches beside the swimming corridor.
    for(int group=0;group<10;++group){
        double cx=(group%2?-1.:1.)*(4.+1.4*std::sin(group*3.7));
        double cz=8.-group*2.35;
        for(int j=0;j<9;++j){
            double x=cx+1.8*std::sin(j*3.1+group),z=cz+1.3*std::cos(j*2.+group);
            double y=bed(x,z)-.06,r=.12+.13*(1.+std::sin(j*4.+group));
            glTexCoord2f(.89f,0);
            glColor3d(.21+j*.008,.235+j*.008,.18+j*.007);
            Point crown{x-.16*r,y+.71*r,z+.22*r};
            for(int k=0;k<7;++k){
                auto edge=[&](int m){double t=m*2*pi/7;return Point{x+1.4*r*std::cos(t),y,z+r*std::sin(t)};};
                face({edge(k+1),edge(k),crown});
            }
        }
        // Short, static seagrass blades give the rocks scale.
        glTexCoord2f(.82f,0);glColor3f(.075f,.16f,.093f);
        for(int j=0;j<12;++j){
            double x=cx+.57*std::sin(j*2.),z=cz+.49*std::cos(j*1.7);
            double y=bed(x,z),h=.28+.2*(1.+std::cos(j));
            face({{x-.055,y-.08,z},{x+.055,y-.08,z},{x+.09,y+h*.6,z+.08}});
            face({{x-.055,y-.08,z},{x+.09,y+h*.6,z+.08},{x+.19,y+h,z+.14}});
        }
    }
}
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
    hull();wreckFittings();cannon();glPopMatrix();
    anchor();seabedDetails();spill();
    // A few exposed chain links lead from the anchor ring toward the gap.
    glTexCoord2f(.60f,.55f);glColor3f(.32f,.33f,.33f);
    for(int i=0;i<32;++i){
        double z=3.65-i*.34, x=-2.12+i*.105;
        double y=bed(x,z)+.055+1.0*std::exp(-.34*i);
        auto link=[&](int j){
            double t=2*pi*j/10, across=.11*std::cos(t), along=.20*std::sin(t);
            return Point{x+across*(i%2?.5:1)-along*.28,
                         y+(i%2?across*.82:0),z+along};
        };
        for(int j=0;j<10;++j) spar(link(j),link(j+1),.032,.032);
    }
}

// First-person equipment uses eye coordinates. Its barrel points away from
// the glove, toward the same side of the frame as the spotlight origin.
void drawHandLight(bool on){
    glPushMatrix();glLoadIdentity();
    glTranslated(.36,-.28,-.46);glRotated(16,0,1,0);glRotated(12,1,0,0);
    glColor3f(.11f,.136f,.145f);
    spar({0,0,.22},{0,0,-.12},.047,.052,true,24);
    for(int i=0;i<7;++i){
        glColor3f(.17f,.19f,.19f);
        spar({0,0,.10-i*.025},{0,0,.091-i*.025},.053,.053,false,24);
    }
    glColor3f(.22f,.245f,.24f);
    spar({0,0,-.10},{0,0,-.20},.051,.077,true,24);
    glColor3f(.14f,.16f,.16f);
    spar({0,0,-.20},{0,0,-.235},.079,.079,true,24);
    glColor3f(on?.64f:.18f,on?.52f:.16f,on?.21f:.12f);
    spar({0,0,-.19},{0,0,-.201},.077,.077,false,24);
    glColor3f(.054f,.073f,.080f);
    ellipsoid({.04,-.045,.11},{.073,.065,.13});
    for(int i=0;i<3;++i) ellipsoid({-.015,-.025,.025+i*.045},{.046,.035,.025});
    glPopMatrix();
}
