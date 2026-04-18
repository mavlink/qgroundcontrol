#include "GstDecoderFallbackController.h"

#include <gst/gstelement.h>
#include <gst/gstpluginfeature.h>

#include "GstDecodingBranch.h"

GstDecoderFallbackController::FallbackResult GstDecoderFallbackController::evaluate(GstMessage* errorMsg,
                                                                                    GstDecodingBranch& branch)
{
    if (!branch.isHwDecoderError(errorMsg))
        return {false, {}};

    const gchar* rawName = gst_plugin_feature_get_name(
        GST_PLUGIN_FEATURE(gst_element_get_factory(GST_ELEMENT(GST_MESSAGE_SRC(errorMsg)))));

    const QString name = rawName ? QString::fromUtf8(rawName) : QStringLiteral("<unknown>");

    markFailed(branch);

    return {true, name};
}

void GstDecoderFallbackController::markFailed(GstDecodingBranch& branch)
{
    branch.setHwDecoderFailed(true);
}
