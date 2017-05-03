
#ifndef __g_cclosure_user_marshal_MARSHAL_H__
#define __g_cclosure_user_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:POINTER,FLOAT,FLOAT,FLOAT,FLOAT (marshaller.src:1) */
extern void g_cclosure_user_marshal_VOID__POINTER_FLOAT_FLOAT_FLOAT_FLOAT (GClosure     *closure,
                                                                           GValue       *return_value,
                                                                           guint         n_param_values,
                                                                           const GValue *param_values,
                                                                           gpointer      invocation_hint,
                                                                           gpointer      marshal_data);

/* VOID:POINTER,DOUBLE,DOUBLE,DOUBLE,DOUBLE (marshaller.src:2) */
extern void g_cclosure_user_marshal_VOID__POINTER_DOUBLE_DOUBLE_DOUBLE_DOUBLE (GClosure     *closure,
                                                                               GValue       *return_value,
                                                                               guint         n_param_values,
                                                                               const GValue *param_values,
                                                                               gpointer      invocation_hint,
                                                                               gpointer      marshal_data);

/* POINTER:POINTER,FLOAT,FLOAT,FLOAT,FLOAT (marshaller.src:3) */
extern void g_cclosure_user_marshal_POINTER__POINTER_FLOAT_FLOAT_FLOAT_FLOAT (GClosure     *closure,
                                                                              GValue       *return_value,
                                                                              guint         n_param_values,
                                                                              const GValue *param_values,
                                                                              gpointer      invocation_hint,
                                                                              gpointer      marshal_data);

/* POINTER:POINTER,DOUBLE,DOUBLE,DOUBLE,DOUBLE (marshaller.src:4) */
extern void g_cclosure_user_marshal_POINTER__POINTER_DOUBLE_DOUBLE_DOUBLE_DOUBLE (GClosure     *closure,
                                                                                  GValue       *return_value,
                                                                                  guint         n_param_values,
                                                                                  const GValue *param_values,
                                                                                  gpointer      invocation_hint,
                                                                                  gpointer      marshal_data);

G_END_DECLS

#endif /* __g_cclosure_user_marshal_MARSHAL_H__ */

