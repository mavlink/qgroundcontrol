void* new_smoke(float x, float y, float z, float dx, float dy, float dz,
	int elems, float intensity, unsigned texture);
void delete_smoke(void *smoke);
void draw_smoke(void *smoke);
void update_smoke(void *smoke, float tick);
