import QtQuick
import QtQuick.VectorImage

// Vector-rendered SVG that stays sharp at any size or DPR — use for logos,
// resizable diagrams, and other untinted SVG content. For tinted icons
// use QGCColoredImage; VectorImage has no native tint API.
VectorImage {
    preferredRendererType: VectorImage.CurveRenderer
    fillMode:              VectorImage.PreserveAspectFit
}
