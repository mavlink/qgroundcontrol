#ifndef CHEETAHMODEL_H_
#define CHEETAHMODEL_H_

#if (defined __APPLE__) & (defined __MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

class CheetahModel
{
public:
        CheetahModel();

        void init(float red, float green, float blue);
	void draw(void);

private:
	GLint cheetah_dl;
};

#endif /* CHEETAHMODEL_H_ */
