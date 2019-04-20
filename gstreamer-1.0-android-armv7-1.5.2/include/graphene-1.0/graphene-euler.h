/* graphene-euler.h: Euler angles
 *
 * Copyright © 2014  Emmanuele Bassi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __GRAPHENE_EULER_H__
#define __GRAPHENE_EULER_H__

#if !defined(GRAPHENE_H_INSIDE) && !defined(GRAPHENE_COMPILATION)
#error "Only graphene.h can be included directly."
#endif

#include "graphene-types.h"
#include "graphene-vec3.h"

GRAPHENE_BEGIN_DECLS

/**
 * graphene_euler_order_t:
 * @GRAPHENE_EULER_ORDER_DEFAULT: Rotate in the default order; the
 *   default order is one of the following enumeration values
 * @GRAPHENE_EULER_ORDER_XYZ: Rotate in the X, Y, and Z order
 * @GRAPHENE_EULER_ORDER_YZX: Rotate in the Y, Z, and X order
 * @GRAPHENE_EULER_ORDER_ZXY: Rotate in the Z, X, and Y order
 * @GRAPHENE_EULER_ORDER_XZY: Rotate in the X, Z, and Y order
 * @GRAPHENE_EULER_ORDER_YXZ: Rotate in the Y, X, and Z order
 * @GRAPHENE_EULER_ORDER_ZYX: Rotate in the Z, Y, and X order
 *
 * Specify the order of the rotations on each axis.
 *
 * The %GRAPHENE_EULER_ORDER_DEFAULT value is special, and is used
 * as an alias for one of the other orders.
 *
 * Since: 1.2
 */
typedef enum {
  GRAPHENE_EULER_ORDER_DEFAULT = -1,
  GRAPHENE_EULER_ORDER_XYZ = 0,
  GRAPHENE_EULER_ORDER_YZX,
  GRAPHENE_EULER_ORDER_ZXY,
  GRAPHENE_EULER_ORDER_XZY,
  GRAPHENE_EULER_ORDER_YXZ,
  GRAPHENE_EULER_ORDER_ZYX
} graphene_euler_order_t;

/**
 * graphene_euler_t:
 *
 * Describe a rotation using Euler angles.
 *
 * The contents of the #graphene_euler_t structure are private
 * and should never be accessed directly.
 *
 * Since: 1.2
 */
struct _graphene_euler_t
{
  /*< private >*/
  GRAPHENE_PRIVATE_FIELD (graphene_vec3_t, angles);
  GRAPHENE_PRIVATE_FIELD (graphene_euler_order_t, order);
};

GRAPHENE_AVAILABLE_IN_1_2
graphene_euler_t *      graphene_euler_alloc                    (void);
GRAPHENE_AVAILABLE_IN_1_2
void                    graphene_euler_free                     (graphene_euler_t            *e);

GRAPHENE_AVAILABLE_IN_1_2
graphene_euler_t *      graphene_euler_init                     (graphene_euler_t            *e,
                                                                 float                        x,
                                                                 float                        y,
                                                                 float                        z);
GRAPHENE_AVAILABLE_IN_1_2
graphene_euler_t *      graphene_euler_init_with_order          (graphene_euler_t            *e,
                                                                 float                        x,
                                                                 float                        y,
                                                                 float                        z,
                                                                 graphene_euler_order_t       order);
GRAPHENE_AVAILABLE_IN_1_2
graphene_euler_t *      graphene_euler_init_from_matrix         (graphene_euler_t            *e,
                                                                 const graphene_matrix_t     *m,
                                                                 graphene_euler_order_t       order);
GRAPHENE_AVAILABLE_IN_1_2
graphene_euler_t *      graphene_euler_init_from_quaternion     (graphene_euler_t            *e,
                                                                 const graphene_quaternion_t *q,
                                                                 graphene_euler_order_t       order);
GRAPHENE_AVAILABLE_IN_1_2
graphene_euler_t *      graphene_euler_init_from_vec3           (graphene_euler_t            *e,
                                                                 const graphene_vec3_t       *v,
                                                                 graphene_euler_order_t       order);
GRAPHENE_AVAILABLE_IN_1_2
graphene_euler_t *      graphene_euler_init_from_euler          (graphene_euler_t            *e,
                                                                 const graphene_euler_t      *src);

GRAPHENE_AVAILABLE_IN_1_2
bool                    graphene_euler_equal                    (const graphene_euler_t      *a,
                                                                 const graphene_euler_t      *b);

GRAPHENE_AVAILABLE_IN_1_2
float                   graphene_euler_get_x                    (const graphene_euler_t      *e);
GRAPHENE_AVAILABLE_IN_1_2
float                   graphene_euler_get_y                    (const graphene_euler_t      *e);
GRAPHENE_AVAILABLE_IN_1_2
float                   graphene_euler_get_z                    (const graphene_euler_t      *e);
GRAPHENE_AVAILABLE_IN_1_2
graphene_euler_order_t  graphene_euler_get_order                (const graphene_euler_t      *e);

GRAPHENE_AVAILABLE_IN_1_2
void                    graphene_euler_to_vec3                  (const graphene_euler_t      *e,
                                                                 graphene_vec3_t             *res);
GRAPHENE_AVAILABLE_IN_1_2
void                    graphene_euler_to_matrix                (const graphene_euler_t      *e,
                                                                 graphene_matrix_t           *res);

GRAPHENE_AVAILABLE_IN_1_2
void                    graphene_euler_reorder                  (const graphene_euler_t      *e,
                                                                 graphene_euler_order_t       order,
                                                                 graphene_euler_t            *res);

GRAPHENE_END_DECLS

#endif /* __GRAPHENE_EULER_H__ */
