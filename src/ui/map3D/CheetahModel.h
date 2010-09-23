#ifndef CHEETAHMODEL_H_
#define CHEETAHMODEL_H_

#include <GL/gl.h>

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
