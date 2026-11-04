#pragma once
#include <cmath>

// A long surge travels along +X, with a weaker cross-current. Submerged
// materials sample the same phase with different response times.
inline double currentSurge(double x,double z,double time){
    return .68*std::sin(.83*time-.52*x-.19*z)
          +.32*std::sin(.37*time-.21*x+.27*z);
}
