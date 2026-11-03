#include "material.h"
#include "shader.h"

namespace scene {
GLuint createMaterialProgram() {
    const char* vertex = R"GLSL(
#version 120
varying vec3 local, viewPosition, mapping, base;
void main() {
    local = gl_Vertex.xyz;
    mapping = gl_MultiTexCoord0.xyz;
    base = gl_Color.rgb;
    vec4 eye = gl_ModelViewMatrix * gl_Vertex;
    viewPosition = eye.xyz;
    gl_ClipVertex = eye;
    gl_Position = gl_ProjectionMatrix * eye;
}
)GLSL";
    const char* fragment = R"GLSL(
#version 120
varying vec3 local, viewPosition, mapping, base;
float hash(vec2 v) {
    vec3 p = fract(vec3(v.xyx)*vec3(.1031,.11369,.13787));
    p += dot(p,p.yzx+19.19);
    return fract((p.x+p.y)*p.z);
}
float valueNoise(vec2 uv) {
    vec2 i=floor(uv), f=fract(uv);
    f=f*f*(3.0-2.0*f);
    return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),
               mix(hash(i+vec2(0,1)),hash(i+1.0),f.x),f.y);
}
// Distance to a recurring seam, filtered in screen space for the far roof.
float seam(float phase,float width) {
    float aa = clamp(fwidth(phase),.002,.25);
    return 1.0-smoothstep(width,width+aa,abs(fract(phase+.5)-.5));
}
void main() {
    vec2 uv=mapping.xy;
    int id=int(floor(mapping.z+.5)); // Surface in material.h
    vec3 c=base;
    if (id!=0) {
        float cloud=valueNoise(uv*1.4)+.45*valueNoise(uv*5.5)-.72;
        c *= 1.0+(id==3 ? .049 : .095)*cloud;
        if (id==3) { // limewash and capillary marks at the foot
            float damp=1.0-smoothstep(.27,.73+.23*valueNoise(vec2(uv.x*2.8,0)),local.y);
            c *= 1.0-.21*damp-.028*valueNoise(uv*vec2(9,.7));
        } else if (id==4) { // grain follows the long dimension
            float grain=sin(uv.y*100.0+4.0*valueNoise(uv*vec2(1.3,12)));
            c *= 1.0+.10*grain*(1.0-smoothstep(.01,.06,fwidth(uv.y)))+.09*cloud;
        } else if (id==5) {
            float channel=uv.x*5.7;
            float rib=.5+.5*cos(6.28318*channel);
            float lap=seam(uv.y*4.6+.14*hash(vec2(floor(channel+.5),3)),.045);
            c *= .75+.40*rib-.16*lap+.13*hash(vec2(floor(channel+.5),8));
        } else if (id==6) { // pale mineral ribbons, confined to selected slabs
            float fault=valueNoise(uv*2.7);
            float bed=uv.y*3.4+uv.x*.67+1.05*valueNoise(uv*1.2)+.40*fault;
            float trace=seam(bed,.016+.035*fault);
            float fork=seam(bed+.42*fault,.014)*smoothstep(.35,.68,fault);
            c=mix(c,vec3(.76,.76,.66),(.5+.29*fault)*max(trace,fork));
            c*=1.0-.16*seam(bed+.085,.019);
        } else if (id==2 || id==7) {
            if (id==2) {
                // Offset slab rows; vary lengths and value, leave the dry stone porous.
                float row=floor(uv.y/.59);
                vec2 slab=vec2(uv.x/(1.30+.43*hash(vec2(row,7)))+.5*row,uv.y/.59);
                float joint=max(seam(slab.x,.006),seam(slab.y,.014));
                float bevel=max(seam(slab.x-.010,.003),seam(slab.y-.020,.005));
                c *= .91+.15*hash(floor(slab))-.26*joint+.045*bevel;
            }
            vec2 outside=max(abs(local.xz)-vec2(6,4),0.0);
            float wet=1.0-smoothstep(.18,2.65,length(outside)+.48*cloud);
            float depressions=valueNoise(uv*vec2(.94,2.1))+.24*cloud;
            float sheen=smoothstep(.57,.72,depressions);
            vec3 normal=normalize(cross(dFdx(viewPosition),dFdy(viewPosition)));
            float grazing=1.0-abs(dot(normal,normalize(-viewPosition)));
            c=mix(c,c*vec3(.58,.66,.72),wet*.81);
            c+=wet*sheen*(.045+.20*grazing*grazing)*vec3(.73,.84,.88);
        }
    }
    gl_FragColor=vec4(c,1);
}
)GLSL";
    return shader::build(vertex,fragment);
}
}
