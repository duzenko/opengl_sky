#pragma once

typedef struct { float m[16]; } matrix;

#ifdef __cplusplus
extern "C" {
#endif
	matrix getProjectionMatrix( int w, int h );
	matrix getRotationMatrix( int axis, float angle );
#ifdef __cplusplus
}
#endif
