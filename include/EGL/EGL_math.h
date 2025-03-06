#ifndef EGL_MATH_H
#define EGL_MATH_H


typedef union {
	float array[2];
	struct {
		float x;
		float y;
	};
} Vec2;

typedef union {
	float array[3];
	struct {
		float x;
		float y;
		float z;
	};
} Vec3;

typedef union {
	float array[4];
	struct {
		float x;
		float y;
		float z;
		float w;
	};
} Vec4;


#endif //EGL_MATH_H
