#pragma once

#include <coreplugin/basefilewizardfactory.h>

namespace QGC::Internal {

class FactGroupWizardFactory : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    FactGroupWizardFactory();

protected:
    Core::BaseFileWizard *create(QWidget *parent,
                                  const Core::WizardDialogParameters &parameters) const override;
    Core::GeneratedFiles generateFiles(const QWizard *wizard, QString *errorMessage) const override;
};

} // namespace QGC::Internal
