// QGC builds on private Qt Multimedia APIs, which carry no compatibility promise between Qt minor releases: a changed
// contract can still compile and break the video. They were audited against Qt 6.12. When the Qt version in
// .github/build-config.json moves to another minor release, diff each header below against the new Qt, re-check its
// users, and then move this check to that release.
//
// - qhwvideobuffer_p.h, qvideotexturehelper_p.h: the zero-copy GPU paths in src/VideoManager/GStreamer/HwBuffers and
//   Sink/gstqgc/GstQgcFrameMap.cc (QHwVideoBuffer::mapTextures(), the texture pool's slot reuse, plane formats).
// - qdmabuftextureimporter_p.h, qmultimedia_drm_support_p.h: HwBuffers/dmabuf/GstQtDmaBufImport.cc, including its fix
//   for Qt's BGRA fourcc.
// - qvideoframe_p.h, qvideoframeconverter_p.h, qquickvideooutput_p.h, qcameradevice_p.h: tests only.

#include <QtCore/qtversionchecks.h>

static_assert((QT_VERSION >= QT_VERSION_CHECK(6, 12, 0)) && (QT_VERSION < QT_VERSION_CHECK(6, 13, 0)),
              "QGC's use of private Qt Multimedia APIs was audited for Qt 6.12; re-audit it for this Qt version as "
              "test/VideoManager/QtMultimediaPrivateApiCheck.cc describes");
