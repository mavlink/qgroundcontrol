// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#ifndef QQMLPROFILERDEFINITIONS_P_H
#define QQMLPROFILERDEFINITIONS_P_H

#include <private/qtqmlglobal_p.h>
#include <private/qv4profiling_p.h>

QT_REQUIRE_CONFIG(qml_debug);

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

struct QQmlProfilerDefinitions {
    enum Message {
        Event,
        RangeStart,
        RangeData,
        RangeLocation,
        RangeEnd,
        Complete, // end of transmission
        PixmapCacheEvent,
        SceneGraphFrame,
        MemoryAllocation,
        DebugMessage,
        Quick3DFrame,

        MaximumMessage
    };

    enum EventType {
        FramePaint,
        Mouse,
        Key,
        AnimationFrame,
        EndTrace,
        StartTrace,

        MaximumEventType
    };

    enum RangeType {
        Painting,
        Compiling,
        Creating,
        Binding,            //running a binding
        HandlingSignal,     //running a signal handler
        Javascript,

        MaximumRangeType
    };

    enum PixmapEventType {
        PixmapSizeKnown,
        PixmapReferenceCountChanged,
        PixmapCacheCountChanged,
        PixmapLoadingStarted,
        PixmapLoadingFinished,
        PixmapLoadingError,

        MaximumPixmapEventType
    };

    enum SceneGraphFrameType {
        SceneGraphRendererFrame,        // Render Thread
        SceneGraphAdaptationLayerFrame, // Render Thread
        SceneGraphContextFrame,         // Render Thread
        SceneGraphRenderLoopFrame,      // Render Thread
        SceneGraphTexturePrepare,       // Render Thread
        SceneGraphTextureDeletion,      // Render Thread
        SceneGraphPolishAndSync,        // GUI Thread
        SceneGraphWindowsRenderShow,    // Unused
        SceneGraphWindowsAnimations,    // GUI Thread
        SceneGraphPolishFrame,          // GUI Thread

        MaximumSceneGraphFrameType,
        NumRenderThreadFrameTypes = SceneGraphPolishAndSync,
        NumGUIThreadFrameTypes = MaximumSceneGraphFrameType - NumRenderThreadFrameTypes
    };

    enum Quick3DFrameType {
        Quick3DRenderFrame,     // Render Thread
        Quick3DSynchronizeFrame,
        Quick3DPrepareFrame,
        Quick3DMeshLoad,
        Quick3DCustomMeshLoad,
        Quick3DTextureLoad,
        Quick3DGenerateShader,
        Quick3DLoadShader,
        Quick3DParticleUpdate,  // GUI Thread
        Quick3DRenderCall,      // Render Thread
        Quick3DRenderPass,      // Render Thread
        Quick3DEventData,       // N/A
        MaximumQuick3DFrameType,
    };

    enum ProfileFeature {
        ProfileJavaScript,
        ProfileMemory,
        ProfilePixmapCache,
        ProfileSceneGraph,
        ProfileAnimations,
        ProfilePainting,
        ProfileCompiling,
        ProfileCreating,
        ProfileBinding,
        ProfileHandlingSignal,
        ProfileInputEvents,
        ProfileDebugMessages,
        ProfileQuick3D,

        MaximumProfileFeature
    };

    enum InputEventType {
        InputKeyPress,
        InputKeyRelease,
        InputKeyUnknown,

        InputMousePress,
        InputMouseRelease,
        InputMouseMove,
        InputMouseDoubleClick,
        InputMouseWheel,
        InputMouseUnknown,

        MaximumInputEventType
    };
};

QT_END_NAMESPACE

#endif
