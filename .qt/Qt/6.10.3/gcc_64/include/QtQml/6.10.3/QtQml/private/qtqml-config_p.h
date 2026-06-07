#ifndef QT_FEATURES_Qml_src_qml_qtqml_config_p_h_H
#define QT_FEATURES_Qml_src_qml_qtqml_config_p_h_H

#define QT_FEATURE_qml_jit 1

#define QT_FEATURE_qml_profiler 1

#define QT_FEATURE_qml_preview 1

#define QT_FEATURE_qml_xml_http_request 1

#define QT_FEATURE_qml_locale 1

#define QT_FEATURE_qml_animation 1

#define QT_FEATURE_qml_worker_script 1

#define QT_FEATURE_qml_itemmodel 1

#define QT_FEATURE_qml_xmllistmodel 1

#define QT_FEATURE_qml_type_loader_thread 1

#define QT_FEATURE_qml_python 1


#define QT_QML_JIT_SUPPORTED_IMPL 0
// Unset dummy value
#undef QT_QML_JIT_SUPPORTED_IMPL
// Compute per-arch value and save in extra define
#if QT_CONFIG(qml_jit) && !(defined(Q_OS_MACOS) && defined(Q_PROCESSOR_ARM))
#define QT_QML_JIT_SUPPORTED_IMPL 1
#else
#define QT_QML_JIT_SUPPORTED_IMPL 0
#endif
// Unset original feature value
#undef QT_FEATURE_qml_jit
// Set new value based on previous computation
#if QT_QML_JIT_SUPPORTED_IMPL
#define QT_FEATURE_qml_jit 1
#else
#define QT_FEATURE_qml_jit -1
#endif


#endif // QT_FEATURES_Qml_src_qml_qtqml_config_p_h_H
