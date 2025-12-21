#include "factgroupwizardfactory.h"
#include "factgroupwizardpage.h"
#include "../qgcconstants.h"

#include <coreplugin/basefilewizard.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/filewizardpage.h>

#include <QCoreApplication>

namespace QGC::Internal {

FactGroupWizardFactory::FactGroupWizardFactory()
{
    setId(Constants::WIZARD_FACTGROUP_ID);
    setDisplayName(tr("QGC FactGroup"));
    setDescription(tr("Creates a new QGroundControl FactGroup with header, source, and JSON metadata files."));
    setCategory(Constants::WIZARD_CATEGORY);
    setDisplayCategory(tr(Constants::WIZARD_CATEGORY_DISPLAY));
    setIcon(QIcon()); // TODO: Add QGC icon
    setRequiredFeatures({});
}

Core::BaseFileWizard *FactGroupWizardFactory::create(
    QWidget *parent,
    const Core::WizardDialogParameters &parameters) const
{
    auto wizard = new Core::BaseFileWizard(this, parameters.extraValues(), parent);
    wizard->setWindowTitle(tr("New QGC FactGroup"));

    // Page 1: FactGroup configuration
    auto configPage = new FactGroupWizardPage(wizard);
    wizard->addPage(configPage);

    // Page 2: File location (standard Qt Creator page)
    auto filePage = new Utils::FileWizardPage;
    filePage->setTitle(tr("Location"));
    filePage->setPath(parameters.defaultPath());
    wizard->addPage(filePage);

    // Connect to update file names when domain changes
    QObject::connect(configPage, &FactGroupWizardPage::domainChanged,
                     filePage, [filePage](const QString &domain) {
        filePage->setFileName(QStringLiteral("Vehicle%1FactGroup").arg(domain));
    });

    return wizard;
}

Core::GeneratedFiles FactGroupWizardFactory::generateFiles(
    const QWizard *wizard,
    QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    Core::GeneratedFiles files;

    // Get configuration from wizard pages
    const auto *configPage = wizard->page(0);
    const auto *filePage = qobject_cast<const Utils::FileWizardPage *>(wizard->page(1));

    if (!configPage || !filePage)
        return files;

    const QString domain = configPage->property("domain").toString();
    const QString className = QStringLiteral("Vehicle%1FactGroup").arg(domain);
    const QString basePath = filePage->path();
    const QString headerFile = basePath + "/" + className + ".h";
    const QString sourceFile = basePath + "/" + className + ".cc";
    const QString jsonFile = basePath + "/" + domain + "Fact.json";

    // Get facts from wizard
    const QStringList facts = configPage->property("facts").toStringList();

    // Generate header content
    QString headerContent = generateHeader(className, domain, facts);
    Core::GeneratedFile header(headerFile);
    header.setContents(headerContent);
    header.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    files.append(header);

    // Generate source content
    QString sourceContent = generateSource(className, domain, facts);
    Core::GeneratedFile source(sourceFile);
    source.setContents(sourceContent);
    files.append(source);

    // Generate JSON metadata
    QString jsonContent = generateJson(domain, facts);
    Core::GeneratedFile json(jsonFile);
    json.setContents(jsonContent);
    files.append(json);

    return files;
}

namespace {

QString generateHeader(const QString &className, const QString &domain, const QStringList &facts)
{
    QString content;
    QTextStream out(&content);

    out << R"(/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactGroup.h"

class )" << className << R"( : public FactGroup
{
    Q_OBJECT
)";

    // Q_PROPERTY declarations
    for (const QString &fact : facts) {
        const QString name = fact.section(':', 0, 0);
        out << "    Q_PROPERTY(Fact *" << name << " READ " << name << " CONSTANT)\n";
    }

    out << R"(
public:
    explicit )" << className << R"((QObject *parent = nullptr);

)";

    // Accessor methods
    for (const QString &fact : facts) {
        const QString name = fact.section(':', 0, 0);
        out << "    Fact *" << name << "() { return &_" << name << "Fact; }\n";
    }

    out << R"(
    // Overrides from FactGroup
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) final;

private:
)";

    // Member variables
    for (const QString &fact : facts) {
        const QString name = fact.section(':', 0, 0);
        const QString type = fact.section(':', 1, 1);
        QString metaType = "FactMetaData::valueTypeDouble";
        if (type == "uint8") metaType = "FactMetaData::valueTypeUint8";
        else if (type == "uint16") metaType = "FactMetaData::valueTypeUint16";
        else if (type == "uint32") metaType = "FactMetaData::valueTypeUint32";
        else if (type == "int32") metaType = "FactMetaData::valueTypeInt32";
        else if (type == "float") metaType = "FactMetaData::valueTypeFloat";
        else if (type == "bool") metaType = "FactMetaData::valueTypeBool";

        out << "    Fact _" << name << "Fact = Fact(0, QStringLiteral(\"" << name << "\"), " << metaType << ");\n";
    }

    out << "};\n";

    return content;
}

QString generateSource(const QString &className, const QString &domain, const QStringList &facts)
{
    QString content;
    QTextStream out(&content);

    out << R"(/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include ")" << className << R"(.h"
#include "Vehicle.h"

)" << className << "::" << className << R"((QObject *parent)
    : FactGroup(1000, QStringLiteral(":/json/Vehicle/)" << domain << R"(Fact.json"), parent)
{
)";

    // _addFact calls
    for (const QString &fact : facts) {
        const QString name = fact.section(':', 0, 0);
        out << "    _addFact(&_" << name << "Fact);\n";
    }

    out << "\n";

    // Initialize with qQNaN()
    for (const QString &fact : facts) {
        const QString name = fact.section(':', 0, 0);
        const QString type = fact.section(':', 1, 1);
        if (type == "double" || type == "float") {
            out << "    _" << name << "Fact.setRawValue(qQNaN());\n";
        } else {
            out << "    _" << name << "Fact.setRawValue(0);\n";
        }
    }

    out << R"(}

void )" << className << R"(::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    Q_UNUSED(vehicle);
    Q_UNUSED(message);

    // TODO: Add MAVLink message handlers
    // switch (message.msgid) {
    // case MAVLINK_MSG_ID_XXX:
    //     _handleXxx(message);
    //     break;
    // }
}
)";

    return content;
}

QString generateJson(const QString &domain, const QStringList &facts)
{
    QString content;
    QTextStream out(&content);

    out << "{\n";
    out << "    \"version\":      1,\n";
    out << "    \"fileType\":     \"FactMetaData\",\n";
    out << "    \"QGC.MetaData.Facts\": [\n";

    for (int i = 0; i < facts.size(); ++i) {
        const QString &fact = facts[i];
        const QString name = fact.section(':', 0, 0);
        const QString type = fact.section(':', 1, 1);
        const QString units = fact.section(':', 2, 2);
        const QString shortDesc = name;
        shortDesc[0] = shortDesc[0].toUpper();

        out << "        {\n";
        out << "            \"name\":             \"" << name << "\",\n";
        out << "            \"shortDesc\":        \"" << shortDesc << "\",\n";
        out << "            \"type\":             \"" << (type.isEmpty() ? "double" : type) << "\"";
        if (!units.isEmpty()) {
            out << ",\n            \"units\":            \"" << units << "\"";
        }
        out << "\n        }";
        if (i < facts.size() - 1) out << ",";
        out << "\n";
    }

    out << "    ]\n";
    out << "}\n";

    return content;
}

} // anonymous namespace

} // namespace QGC::Internal
