/*!
 *   @brief Auterion QGCCorePlugin Declaration
 *   @author Gus Grubba <gus@grubba.com>
 */

#pragma once

#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "QGCLoggingCategory.h"

#include <QTranslator>

class AuterionOptions;
class AuterionSettings;

Q_DECLARE_LOGGING_CATEGORY(AuterionLog)

//-----------------------------------------------------------------------------
class AuterionPlugin : public QGCCorePlugin
{
    Q_OBJECT
public:
    AuterionPlugin(QGCApplication* app, QGCToolbox *toolbox);
    ~AuterionPlugin();

    // Overrides from QGCCorePlugin
    QGCOptions*     options                         () final;
    QString         brandImageIndoor                () const final;
    QString         brandImageOutdoor               () const final;

    bool            overrideSettingsGroupVisibility (QString name) final;

    // Overrides from QGCTool
    void            setToolbox                      (QGCToolbox* toolbox);

private:
    AuterionOptions*     _pOptions;
};
