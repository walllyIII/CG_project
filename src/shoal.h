#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace wildlife {
struct Vec {
    double x=0,y=0,z=0;
    Vec operator+(Vec b) const {return {x+b.x,y+b.y,z+b.z};}
    Vec operator-(Vec b) const {return {x-b.x,y-b.y,z-b.z};}
    Vec operator*(double k) const {return {x*k,y*k,z*k};}
    double dot(Vec b) const {return x*b.x+y*b.y+z*b.z;}
    double length() const {return std::sqrt(dot(*this));}
};
struct Fish {
    Vec position,offset,escape;
    double hold=0,heading=0,tail=0;
};
inline std::array<Fish,7> shoal{};
inline Vec cruising(int i,double time){
    double phase=.34*time+.38*i+.48*std::sin((.39+.023*i)*time+i*2.1);
    return {-.48+(1.30+.07*(i%4))*std::sin(phase),
            1.86+.19*std::sin(phase*2+i*2.3),
            -2.65+.72*std::cos(phase)+.20*(i%3)+.12*std::sin(.57*time+i)};
}
// Advance once per frame. Both the shadow pass and the visible pass read this pose.
inline void update(double time,double dt,Vec eye,Vec lamp,Vec beam,bool lit){
    bool reset=dt<=0 || dt>.5;
    for(int i=0;i<int(shoal.size());++i){
        Fish& f=shoal[i];
        Vec home=cruising(i,time);
        if(reset){
            f={};f.position=home;f.tail=time*(6+.17*i)+i;
            Vec tangent=cruising(i,time+.01)-home;
            f.heading=std::atan2(-tangent.z,tangent.x);
            continue;
        }
        Vec away=f.position-eye,ray=f.position-lamp;
        double along=ray.dot(beam);
        Vec lateral=ray-beam*along;
        bool close=away.length()<1.05+.08*(i%4);
        bool illuminated=lit && along>.12 && along<5.8 && lateral.length()<along*(.15+.026*(i%3));
        if(close || illuminated){
            if(f.hold<=0){
                Vec direction=close?away:lateral;
                // Each fish favours one flank, even directly on the beam axis.
                direction=direction+Vec{.20*std::cos(2.4*i),.09+.05*(i%3),.20*std::sin(2.4*i)};
                f.escape=direction*(1.12/std::max(.05,direction.length()));
            }
            f.hold=.78+.13*(i%4);
        }else f.hold=std::max(0.,f.hold-dt);
        Vec target=f.hold>0?f.escape:Vec{};
        f.offset=f.offset+(target-f.offset)*(-std::expm1(-dt/(f.hold>0?.34:2.1)));
        Vec next=home+f.offset,travel=next-f.position;
        double turn=std::remainder(std::atan2(-travel.z,travel.x)-f.heading,6.28318530718);
        if(std::hypot(travel.x,travel.z)>1e-7) f.heading+=std::clamp(turn,-5.*dt,5.*dt);
        f.tail+=dt*(4.5+.21*i+2.1*std::min(2.,travel.length()/dt));
        f.position=next;
    }
}
}
