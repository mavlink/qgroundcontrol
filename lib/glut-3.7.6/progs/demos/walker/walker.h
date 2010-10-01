#define CYCLE_SIZE 100
#define CYCLE_STEP 1.0/CYCLE_SIZE

extern GLfloat Walk_cycle[2][5][CYCLE_SIZE];

extern int Step;

extern void DrawTheGuy_WC(void);
extern void DrawTheGuy_SC(void);
extern void DrawTheGuy_SL(void);
extern void DrawTheGuy_SL2(void);
extern void StoreTheGuy_SL(void);
extern void StoreTheGuy_SL2(void);
