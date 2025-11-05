#ifndef mat_h_INCLUDED
#define mat_h_INCLUDED

void mat_ortho(float left, float right, float bottom, float top, float near_z, float far_z, float dst[16]);

#ifdef CSM_BASE_IMPLEMENTATION

void mat_ortho(float left, float right, float bottom, float top, float near_z, float far_z, float dst[16])
{
	float rl, tb, fn;

	for(u8 i = 0; i < 16; i++) {
		dst[i] = 0.0f;
	}

	rl =  1.0f / (right - left);
	tb =  1.0f / (top   - bottom);
	fn = -1.0f / (far_z - near_z);

	dst[0] = 2.0f * rl;
	dst[5] = 2.0f * tb;
	dst[10] =-fn;
	dst[12] =-(right + left)   * rl;
	dst[13] =-(top   + bottom) * tb;
	dst[14] = near_z * fn;
	dst[15] = 1.0f;
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // mat_h_INCLUDED
