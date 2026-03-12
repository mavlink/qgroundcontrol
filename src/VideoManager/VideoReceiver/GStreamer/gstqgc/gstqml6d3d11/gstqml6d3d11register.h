#pragma once

// Register the GstD3D11Qt6VideoItem QML type early, before the QML engine
// loads. The upstream plugin registers it lazily during GType class_init,
// but QGC's QML components need the type available at engine load time.
void gstQml6D3D11RegisterQmlTypes();
