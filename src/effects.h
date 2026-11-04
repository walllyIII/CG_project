#pragma once
#include <array>
struct Effects {bool water=true, caustics=true, shadows=true, lamp=true;};
struct TorchPose {
    double x=.36,y=-.28,z=-.22,pitch=0;
};
struct BreathBubble {double x=0,y=0,z=0,born=-10,radius=0;};
using BreathCloud=std::array<BreathBubble,3>;
bool initEffects();
void bindEffects(const Effects& effects,double time,double x,double y,double z,double yaw,double pitch,const TorchPose& torchPose);
void finishDiveView(bool lamp,double time,const TorchPose& torchPose,const BreathCloud& breath);
void releaseEffects();
