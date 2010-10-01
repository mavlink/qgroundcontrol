typedef struct matrix {
    double index[4][4];
} Matrix;

typedef struct vector {
    double index[4];
} Vector;

void init_matrix(Matrix *m);
void init_vector(Vector *v);
void copy_vector(Vector *v1, Vector *v2);
void copy_matrix(Matrix *m1, Matrix *m2);
void multiply_vector_matrix(Matrix *m, Vector *v);
void multiply_matrix_vector(Matrix *m, Vector *v);
void multiply_matrix(Matrix *m1, Matrix *m2);
void rotate_x(double angle, Matrix *m);
void rotate_y(double angle, Matrix *m);
void rotate_z(double angle, Matrix *m);
