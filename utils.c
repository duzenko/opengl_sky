// Math Functions

#include "utils.h"
#include <math.h>

matrix getProjectionMatrix( int w, int h ) {
    float fov_y = 1;
    float tanFov = tanf( fov_y * 0.5f );
    float aspect = (float) w / (float) h;
    float near = 1.0f;
    float far = 1000.0f;

    return ( matrix ) {
        .m = {
       [0] = 1.0f / ( aspect * tanFov ),
       [5] = 1.0f / tanFov,
       [10] = -1.f,
       [11] = -1.0f,
       [14] = -( 2.0f * near )
        }
    };
}

matrix getRotationMatrix( int axis, float angle ) {
    float c = cosf( angle ), s = sinf( angle );

    if ( axis == 0 )
        return ( matrix ) {
        .m = {
            [0] = 1,

            [5] = c,
            [6] = -s,

            [9] = s,
            [10] = c,

            [15] = 1
        }
    };
    //if ( axis == 1 )
    return ( matrix ) {
        .m = {
            [0] = c,
            [1] = 0,
            [2] = s,

            [5] = 1,

            [8] = -s,
            [10] = c,

            [15] = 1
        }
    };
}