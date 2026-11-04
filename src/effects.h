#pragma once
struct Effects {bool water=true, caustics=true, shadows=true, lamp=true;};
bool initEffects();
void bindEffects(const Effects& effects,double time,double x,double y,double z,double yaw,double pitch);
void finishDiveView(bool lamp,double time);
void releaseEffects();
