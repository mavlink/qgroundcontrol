#include "CheetahModel.h"

#include "CheetahGL.h"

CheetahModel::CheetahModel()
 : cheetah_dl(-1)
{

}

void
CheetahModel::init(float red, float green, float blue)
{
	cheetah_dl = generatePixhawkCheetah(red, green, blue);
}

void
CheetahModel::draw(void)
{
	glPushMatrix();
        glScalef(0.5f, 0.5f, -0.5f);
	glCallList(cheetah_dl);
	glPopMatrix();
}
