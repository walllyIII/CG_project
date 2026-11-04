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
// Coordinates on the three strakes immediately forward of the gun opening.
// A positive depth lifts a fragment out of the hull toward the hand torch.
Point gunPlank(double x,double y,double depth){
    double h=(y-.12)/3.43;
    return {x,y,(2.45-.133*(x-2.))*(.24+.76*h)+.045+depth};
}
void scarFace(Point a,Point b,Point c,Point d){
    face({a,b,c});face({a,c,d});
}
void wornGunport(){
    // Separate strips trace the longitudinal split. The surviving narrow
    // tongue projects beyond the dull, recessed fracture beside it.
    for(int row=5;row<=7;++row){
        double y=.12+3.43*row*.1;
        constexpr int bands=12, spans=13;
        for(int band=0;band<bands;++band){
            auto shape=[&](int line,int segment){
                double v=line/double(bands),u=segment/double(spans);
                double end=1.88;
                if(row==7){
                    end=2.03+.088*std::sin(line*2.37);
                    if(line==8) end=1.17;
                    if(line==7 || line==9) end=1.60;
                    if(line==6) end=2.38;
                }else if(row==6){
                    // The missing wedge intersects the old fastening.
                    end=1.84+.44*std::exp(-std::pow((v-.55)/.17,2));
                }else end=1.89+.035*std::cos(line*.71);
                double x=end+(4.995-end)*u;
                double lift=row==6?.25*std::exp(-2.4*(x-1.84))*(.65+.35*v):0.;
                double roll=row==5?.045*std::sin(pi*v):0.;
                double tip=row==7?.085*std::max(0.,2.-x):0.;
                double edge=(v==0 || v==1)?-.018:0.;
                return gunPlank(x,y+.323*v+.012*std::sin(2.6*x+row),lift+roll+edge+tip);
            };
            for(int j=0;j<spans;++j){
                Point a=shape(band,j),b=shape(band,j+1),c=shape(band+1,j+1),d=shape(band+1,j);
                double tone=.96+.047*std::sin(band*1.7);
                glTexCoord2f(.62f,0);glColor3d(.20*tone,.147*tone,.095*tone);
                scarFace(a,b,c,d);
                Point back{0,0,-.104};
                glColor3f(.165f,.139f,.104f);
                if(j==0) scarFace(a,d,d+back,a+back);
                if(band==0) scarFace(a,b,b+back,a+back);
                if(band==bands-1) scarFace(d,c,c+back,d+back);
            }
        }
    }
    // The last few centimetres of the splinter are burnished along the ridge.
    glTexCoord2f(.39f,0);glColor3f(.29f,.23f,.157f);
    Point tip=gunPlank(1.17,2.739,.073);
    face({tip,gunPlank(1.46,2.756,.061),gunPlank(1.43,2.736,.074)});
    // Flattened hoop nail remains in the rib behind the broken notch.
    glTexCoord2f(.58f,0);glColor3f(.13f,.10f,.066f);
    spar(gunPlank(2.02,1.25,-.25),gunPlank(2.14,3.1,-.25),.14,.105,true,7);
    Point shank=gunPlank(2.11,2.37,-.20);
    glTexCoord2f(.58f,.87f);glColor3f(.19f,.129f,.077f);
    spar(shank,shank+Point{.023,-.014,.25},.019,.025);
    spar(shank+Point{.023,-.014,.24},shank+Point{.026,-.014,.258},.047,.043,true,7);
    // Dark silt remains under the worn lip. A short, uneven fan of sand
    // spills down the outside and merges into the existing seabed bank.
    glTexCoord2f(.89f,0);glColor3f(.079f,.070f,.052f);
    for(int j=0;j<10;++j){
        double x=2.36+.102*j;
        Point a=gunPlank(x,1.845+.013*std::sin(1.8*j),.012);
        Point b=gunPlank(x+.11,1.849+.012*std::sin(1.8*(j+1)),.016);
        face({a,b,b+Point{0,.018,.003},a+Point{0,.009,.003}});
    }
    for(int j=0;j<15;++j){
        double x=2.84+.022*j+.029*std::sin(2.8*j);
        Point foot=gunPlank(x,2.164+.014*std::sin(j*2.3),.03);
        glTexCoord2f(.91f,0);glColor3f(.16f,.17f,.12f);
        spar(foot,foot+Point{.002,-.007,.019},.013+.008*(j%3),.006,false,7);
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
                if(side>0 && i==4 && row>=5 && row<=7) continue;
                double low=row*.10, high=low+.097;
                bool torn=side>0 && i==3 && row>=5;
                double tone=.91+.13*std::sin(i*4.1+row*1.9);
                glTexCoord2f(.62f,0);glColor3d(.224*tone,.147*tone,.087*tone);
                auto board=[&](double u,double h){
                    Point a=at(i,h), b=at(i+1,h);
                    Point p=a+(b-a)*u;
                    p.y+=.023*std::sin(p.x*1.15+row*1.72);
                    p.z+=side*(.04+.031*std::sin(row*2.3)+.027*std::sin(p.x*1.36+row));
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
                    if(row<5 || row>7) section(.91-.045*std::cos(row*3.),1,.97,1);
                }else{
                    // Unequal end checks and eroded corners interrupt straight seams.
                    double chip=.007+.023*(.5+.5*std::sin(i*2.4+row*1.37));
                    section(chip,.998,.002,1.-chip);
                }
            }
            if(i!=3 || side<0){
                glColor3f(.19f,.09f,.038f);
                spar(at(i,.98),at(i+1,.98),.085,.085);
            }

        }
    }
    face({{-6.85,.12,-.36},{-6.85,.12,.36},{-8,4.4,1.5},{-8,4.4,-1.5}});
    wornGunport();
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
    glColor3f(.18f,.129f,.077f);
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
        glTexCoord2f(.27f,.79f);glColor3f(.31f,.40f,.46f);
        face({rim(i,.35,1.33),rim(i+1,.35,1.33),rim(i+1,.21,1.33),rim(i,.21,1.33)});
        glTexCoord2f(.59f,.85f);glColor3f(.09f,.10f,.11f);
        face({rim(i,.21,1.33),rim(i+1,.21,1.33),rim(i+1,.21,.88),rim(i,.21,.88)});
        face({{0,0,.88},rim(i,.21,.88),rim(i+1,.21,.88)});
    }
    glPopMatrix();
    glTexCoord2f(.58f,0);glColor3f(.18f,.085f,.037f);
    for(int j=0;j<20;++j){
        double x=-3.48+.34*j;
        double wear=.055+.087*(1.+std::sin(j*2.73));
        glColor3d(.16+.026*std::sin(j),.108+.015*std::sin(j),.066);
        face({{x,1.74,-1.82},{x,1.74,1.47-wear},{x+.328,1.74,1.47-.28*wear},{x+.328,1.74,-1.82}});
    }
}
void anchor(){
    glPushMatrix();glTranslated(-2.9,-.78,5.6);glRotated(-22,0,1,0);
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
double bed(double x,double z){
    // A long drift piles against the buried lower strakes. Smaller swells
    // and asymmetric ripple crests share one height field with the props.
    double axis=z+9.0-.32*x;
    double bank=1.58*std::exp(-axis*axis/4.6-x*x/58.);
    double crest=std::sin(6.1*z+2.8*x+.90*std::sin(.7*x)+.58*std::sin(.63*z));
    double pocket=.70*std::exp(-std::pow((x-3.01)/1.12,2)-std::pow((z+7.83)/1.34,2));
    return -.13+bank+pocket+.12*std::sin(.33*x+.25*z)+.055*std::cos(.53*x-.39*z)
        +(.028+.012*std::sin(x*.69))*crest+.012*crest*crest;
}
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
void hoops(Point center,double rx,double ry,double tube){
    for(int i=0;i<18;++i){
        auto edge=[&](int j){double t=j*2*pi/18;return center+Point{rx*std::cos(t),ry*std::sin(t),0};};
        spar(edge(i),edge(i+1),tube,tube,false,6);
    }
}
void gunRoom(double time){
    // The roll of the hull makes negative local Z the low side.
    glTexCoord2f(.53f,0);glColor3f(.18f,.089f,.041f);
    timber({2.12,2.66,-1.77},.23,1.86,.25);
    glTexCoord2f(.58f,.83f);glColor3f(.22f,.19f,.14f);
    hoops({2.12,2.52,-1.57},.135,.16,.037);
    glTexCoord2f(.83f,0);glColor3f(.59f,.54f,.40f);
    for(int i=0;i<15;++i){
        double t=i*2.4;
        Point base{2.12+.16*std::cos(t),2.52+.18*std::sin(t),-1.63};
        spar(base,base+Point{.01,.012,.046},.038,.021,false,7);
    }
    // Three twisted hemp strands pass through the ring and slacken onto the deck.
    Point knots[]={{2.18,2.35,-1.72},{2.12,2.55,-1.58},{2.05,2.43,-1.43},
                   {2.04,1.95,-1.19},{1.91,1.80,-.87},{2.18,1.80,-.49},{2.43,1.80,-.61}};
    glTexCoord2f(.81f,0);
    for(int strand=0;strand<3;++strand){
        glColor3d(.33+.047*strand,.21+.034*strand,.106+.021*strand);
        Point prev=knots[0];
        for(int j=0;j<6;++j) for(int k=1;k<=10;++k){
            double t=k/10.,phase=(10*j+k)*.68+strand*2*pi/3;
            Point next=knots[j]+(knots[j+1]-knots[j])*t+Point{.029*std::cos(phase),.014*std::sin(phase),.026*std::sin(phase)};
            spar(prev,next,.023,.022,false,6);prev=next;
        }
        for(int k=0;k<3;++k){
            double t=strand*.65+k*.41;
            spar(prev,prev+Point{.13+.07*std::cos(t),.025*std::sin(4*t),.12*std::sin(2*t)},.010,.002,true,5);
        }
    }
    glTexCoord2f(.56f,0);glColor3f(.23f,.11f,.049f);
    glPushMatrix();glTranslated(2.72,1.98,-1.66);glRotated(71,0,0,1);
    for(double x:{-.14,.14}) timber({x,0,0},.065,.26,.64);
    for(double z:{-.25,.25}) timber({0,-.095,z},.33,.07,.075);
    glPopMatrix();
    glTexCoord2f(.52f,.89f);glColor3f(.20f,.21f,.20f);
    for(Point p: {Point{2.15,1.89,-1.59},Point{2.46,1.89,-1.64},Point{2.78,1.89,-1.39},Point{2.99,1.89,-1.64}})
        ellipsoid(p,{.15,.15,.15});
    // A low wedge of sediment overlaps the feet of the carriage.
    glTexCoord2f(.94f,0);glColor3f(.43f,.34f,.20f);
    for(int i=0;i<18;++i){
        auto edge=[](int j){double t=j*pi/9;return Point{1.12+.81*std::cos(t),1.746,.07+.73*std::sin(t)};};
        face({edge(i),edge(i+1),{1.23,1.91,-.22}});
    }
    // A sheared lid exposes only the near corner of the crushed box.
    glPushMatrix();glTranslated(-.59,1.75,-.39);
    glTexCoord2f(.59f,0);glColor3f(.30f,.16f,.07f);
    for(double x:{-.43,.43})timber({x,.21,0},.065,.42,.80);
    timber({0,.20,-.37},.86,.40,.065);timber({0,.12,.37},.86,.24,.06);
    timber({0,.22,0},.82,.05,.72);
    for(int i=0;i<4;++i){
        double length=i<2?.80:(i==2?.49:.28);
        timber({-.32+i*.215,.435,-.40+length*.5},.208,.072,length);
    }
    for(int i=0;i<4;++i){
        double x=.16+i*.041;
        face({{x,.468,-.115},{x+.04,.468,-.114},{x+.025,.463,.046+.035*std::sin(i)}});
    }
    glColor3f(.15f,.074f,.032f);
    glPushMatrix();glTranslated(-.25,.61,-.08);glRotated(-15,0,0,1);glRotated(32,0,1,0);
    timber({0,0,0},2.26,.23,.27);glPopMatrix();
    // Tin cup with an open mouth, a rolled lip and a loop handle.
    glTexCoord2f(.29f,.76f);glColor3f(.67f,.67f,.60f);
    spar({.23,.245,.17},{.23,.48,.17},.064,.087,false,24);
    glPushMatrix();glTranslated(.23,.48,.17);glRotated(90,1,0,0);hoops({0,0,0},.087,.087,.009);glPopMatrix();
    hoops({.326,.384,.17},.042,.059,.010);
    glTexCoord2f(.32f,0);glColor3f(.80f,.81f,.71f);
    for(int i=0;i<6;++i){
        double x=.03+.048*i,z=.28+.037*std::sin(2.8*i);
        face({{x,.253,z},{x+.074,.26,z+.026},{x+.026,.284,z-.052}});
    }
    glPopMatrix();
    glTexCoord2f(.18f,.79f);glColor3f(.85f,.84f,.76f);
    for(int i=0;i<5;++i){
        double x=-.098-.34*(i%2),z=.21+i*.095;
        glPushMatrix();glTranslated(x,1.757,z);glRotated(12+i*9,0,1,0);glRotated(26+i*8,1,0,0);
        spar({0,-.007,0},{0,.006,0},.057,.057,false,24);
        spar({0,.006,0},{0,.010,0},.057,.051,true,24);
        spar({0,.010,0},{0,.012,0},.024,.021,true,8);glPopMatrix();
    }
    // Torn canvas snagged on projecting wood at the left edge of the opening.
    glTexCoord2f(.86f,0);glColor3f(.19f,.17f,.105f);
    // The soaked strip stays caught on a splinter. Its free end crosses
    // the light path just in front of the gun, including in the depth pass.
    auto strip=[&](int j,double side){
        double t=j/15.,wave=std::sin(.96*time-4.6*t);
        return Point{.03+1.03*t+side*.018,2.94-.36*t+.19*t*wave+side*(.078-.053*t),
                     2.15-.41*t+.16*t*std::sin(.77*time-3.*t)+side*.027};
    };
    for(int i=0;i<15;++i){
        double tornEdge=(i==9 || i==12)?.23:1.;
        face({strip(i,-1),strip(i+1,-1),strip(i+1,tornEdge),strip(i,1)});
    }
    for(int i=-2;i<=2;++i){
        Point root=strip(15,i*.42);
        face({root,root+Point{0,.013,0},root+Point{.11+.025*i,.025*std::sin(time+i),-.05}});
    }
    glTexCoord2f(.89f,0);glColor3f(.46f,.48f,.39f);
    for(int i=0;i<32;++i){
        double x=.018+.23*(.5+.5*std::sin(i*2.4)),y=2.81+.21*std::cos(i*3.88);
        Point foot{x,y,2.15+(y-2.8)*.47};
        spar(foot,foot+Point{.009,.012,.043},.025+.008*(i%3),.014,false,8);
    }
    // The line ends stay fixed as the belly of the torn hammock sways.
    glColor3f(.37f,.32f,.21f);
    for(int i=0;i<19;++i){
        auto cloth=[&](int j,double side){
            double t=j/19.,sag=std::sin(pi*t);
            return Point{-2.31+3.16*t,3.78-.67*sag+.115*side*side*sag,
                         -1.24+sag*(.36*side+.064*std::sin(.73*time))};
        };
        if(i==0 || i==18) continue;
        face({cloth(i,-1),cloth(i+1,-1),cloth(i+1,0),cloth(i,0)});
        if(i%7!=3) face({cloth(i,0),cloth(i+1,0),cloth(i+1,1),cloth(i,1)});
    }
    glColor3f(.18f,.09f,.041f);timber({-.55,3.91,-1.26},3.72,.18,.18);
    for(double x:{-2.10,.65}) spar({x,3.90,-1.24},{x,3.63,-1.24},.021,.021);
    glPushMatrix();glTranslated(-.48,3.81,-1.09);glRotated(4.*std::sin(.87*time),0,0,1);
    glTexCoord2f(.54f,.7f);glColor3f(.33f,.24f,.12f);
    hoops({0,-.095,0},.076,.105,.015);
    spar({0,-.22,0},{0,-.26,0},.074,.13);
    for(int j=0;j<4;++j){double t=j*pi/2;Point foot{.095*std::cos(t),-.51,.095*std::sin(t)};
        spar(foot,foot+Point{0,.26,0},.012,.012);}
    spar({0,-.56,0},{0,-.50,0},.085,.12);
    glTexCoord2f(.73f,0);glColor3f(.10f,.088f,.065f);
    spar({0,-.50,0},{0,-.39,0},.05,.024);
    glPopMatrix();
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
        double tone=.68+.19*std::cos(j*5.2);
        glTexCoord2f(.53f,0);glColor3d(.24*tone,.173*tone,.102*tone);
        auto stave=[&](int end,double y,double r){
            double a=(j+.025+end*.95)*2*pi/14;
            double loss=(y>0 && j%5==1)?(.055+.052*end):0.;
            return Point{r*std::cos(a),y-loss,r*std::sin(a)};
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
    // The long spar arrests a barrel downslope of the breach.
    barrel(.28,-5.40,75,1.02);barrel(-1.0,-6.58,-36,.84);
    glTexCoord2f(.62f,0);glColor3f(.22f,.115f,.046f);
    spar({-2.28,bed(-2.28,-5.65)-.32,-5.65},{1.82,bed(1.82,-4.81)+.74,-4.81},.23,.15);
    // Cargo crates of different sizes, with individual slats and inset seams.
    for(int i=0;i<2;++i){
        const double xs[]={1.25,-1.88},zs[]={-5.86,-6.25};
        double x=xs[i],z=zs[i];
        glPushMatrix();glTranslated(x,bed(x,z)+.31,z);glRotated(22+i*29,0,1,0);glRotated(i*4,0,0,1);
        glTexCoord2f(.59f,0);
        for(int j=0;j<5;++j){
            glColor3d(.195+j*.009,.137+j*.006,.081+j*.006);
            double lostCorner=(j==4?.13:.012)*(1.+.4*std::cos(i+j));
            timber({(j-2)*.22,-lostCorner*.5,0},.214,.75-lostCorner,.79);
        }
        glColor3f(.14f,.073f,.031f);
        for(double end:{-.41,.41}){
            timber({end,0,0},.11,.82,.84);
        }
        glPushMatrix();glRotated(29,0,0,1);timber({0,0,.409},1.10,.095,.08);glPopMatrix();
        glPopMatrix();
    }
    for(int j=0;j<15;++j){
        double x=.32+1.85*std::sin(j*7.31),z=-5.64+.93*std::cos(j*2.32);
        glPushMatrix();glTranslated(x,bed(x,z)+.025,z);glRotated(j*97.,0,1,0);glRotated(4.*std::sin(j),0,0,1);
        glTexCoord2f(.57f,0);glColor3d(.19+.025*(j%4),.092,.044);
        timber({0,0,0},.46+.29*(j%6),.09,.12+.025*(j%5));glPopMatrix();
    }
    glTexCoord2f(.43f,.91f);glColor3f(.21f,.19f,.17f);
    for(int j=0;j<9;++j){
        double x=1.45+.52*std::sin(j*4.4),z=-4.67+.35*std::cos(j*2.5);
        ellipsoid({x,bed(x,z)-.085,z},{.17,.17,.17});
    }
}
void seabedDetails(){
    // Low, asymmetric stones collect in patches beside the swimming corridor.
    for(int group=5;group<9;++group){
        double cx=(group%2?-1.:1.)*(4.+1.4*std::sin(group*3.7));
        double cz=8.-group*2.35;
        for(int j=0;j<9;++j){
            double x=cx+1.8*std::sin(j*3.1+group),z=cz+1.3*std::cos(j*2.+group);
            // Loose stones settle into the drift rather than balancing on its crest.
            if(x>1.6 && x<4.6 && z>-9.3 && z<-6.1) continue;
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
void staticWreck(){
    // More samples near the dive path resolve small sand ridges in silhouette
    // and in the torch shadow map. Outer strips share the same boundary vertices.
    for(double z=-70;z<36;){
        double nextZ=z+(z>=-16 && z<16?.125:2.);
        glTexCoord2f(.93f,0);glColor3f(.42f,.37f,.27f);glBegin(GL_TRIANGLE_STRIP);
        for(double x=-60;x<=60;x+=(x>=-12 && x<12?.125:2.)){
            for(double row:{z,nextZ}){
                Point n=unit({bed(x-.045,row)-bed(x+.045,row),.09,bed(x,row-.045)-bed(x,row+.045)});
                glNormal3d(n.x,n.y,n.z);glVertex3d(x,bed(x,row),row);
            }
        }
        glEnd();z=nextZ;
    }
    glPushMatrix();glTranslated(.6,-.48,-9.6);glRotated(-18,0,1,0);glRotated(-12,1,0,0);
    hull();wreckFittings();cannon();glPopMatrix();
    anchor();seabedDetails();spill();
    glTexCoord2f(.60f,.55f);glColor3f(.32f,.33f,.33f);
    for(int i=-10;i<32;++i){
        double z=3.65-i*.34, x=-2.12+i*.105;
        double y=bed(x,z)-.085+.15*std::cos((i+7)*.59);
        auto link=[&](int j){
            double t=2*pi*j/10, across=.11*std::cos(t), along=.20*std::sin(t);
            return Point{x+across*(i%2?.5:1)-along*.28,
                         y+(i%2?across*.82:0),z+along};
        };
        for(int j=0;j<10;++j) spar(link(j),link(j+1),.032,.032);
    }
}

void drawGraybox(double time){
    static GLuint fixedScene=0;
    if(!fixedScene){
        fixedScene=glGenLists(1);glNewList(fixedScene,GL_COMPILE);staticWreck();glEndList();
    }
    glCallList(fixedScene);
    glPushMatrix();glTranslated(.6,-.48,-9.6);glRotated(-18,0,1,0);glRotated(-12,1,0,0);
    gunRoom(time);glPopMatrix();
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
