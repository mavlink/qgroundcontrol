#include "factgroupwizardpage.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace QGC::Internal {

FactGroupWizardPage::FactGroupWizardPage(QWidget *parent)
    : QWizardPage(parent)
{
    setTitle(tr("FactGroup Configuration"));
    setSubTitle(tr("Configure the FactGroup name and define its Facts."));
    setupUi();
}

void FactGroupWizardPage::setupUi()
{
    auto *layout = new QVBoxLayout(this);

    // Domain name input
    auto *domainLayout = new QHBoxLayout;
    domainLayout->addWidget(new QLabel(tr("Domain Name:")));
    m_domainEdit = new QLineEdit;
    m_domainEdit->setPlaceholderText(tr("e.g., Wind, Battery, GPS"));
    m_domainEdit->setToolTip(tr("The domain name becomes VehicleXxxFactGroup"));
    domainLayout->addWidget(m_domainEdit);
    layout->addLayout(domainLayout);

    // Preview label
    auto *previewLabel = new QLabel;
    previewLabel->setStyleSheet("color: gray; font-style: italic;");
    layout->addWidget(previewLabel);

    connect(m_domainEdit, &QLineEdit::textChanged, this, [this, previewLabel](const QString &text) {
        if (!text.isEmpty()) {
            previewLabel->setText(tr("Class: Vehicle%1FactGroup").arg(text));
        } else {
            previewLabel->clear();
        }
        onDomainChanged();
    });

    // Facts table
    auto *factsGroup = new QGroupBox(tr("Facts"));
    auto *factsLayout = new QVBoxLayout(factsGroup);

    m_factsTable = new QTableWidget(0, 3);
    m_factsTable->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Units")});
    m_factsTable->horizontalHeader()->setStretchLastSection(true);
    m_factsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_factsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    factsLayout->addWidget(m_factsTable);

    // Add/Remove buttons
    auto *buttonLayout = new QHBoxLayout;
    auto *addButton = new QPushButton(tr("Add Fact"));
    auto *removeButton = new QPushButton(tr("Remove"));
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(removeButton);
    buttonLayout->addStretch();
    factsLayout->addLayout(buttonLayout);

    connect(addButton, &QPushButton::clicked, this, &FactGroupWizardPage::addFact);
    connect(removeButton, &QPushButton::clicked, this, &FactGroupWizardPage::removeFact);

    layout->addWidget(factsGroup);

    // Register fields for wizard
    registerField("domain*", m_domainEdit);
}

QString FactGroupWizardPage::domain() const
{
    return m_domainEdit->text();
}

void FactGroupWizardPage::setDomain(const QString &domain)
{
    m_domainEdit->setText(domain);
}

QStringList FactGroupWizardPage::facts() const
{
    QStringList result;
    for (int row = 0; row < m_factsTable->rowCount(); ++row) {
        QString name = m_factsTable->item(row, 0) ? m_factsTable->item(row, 0)->text() : QString();
        QString type;
        QString units;

        if (auto *typeCombo = qobject_cast<QComboBox *>(m_factsTable->cellWidget(row, 1))) {
            type = typeCombo->currentText();
        }
        if (auto *unitsEdit = qobject_cast<QLineEdit *>(m_factsTable->cellWidget(row, 2))) {
            units = unitsEdit->text();
        }

        if (!name.isEmpty()) {
            result.append(QStringLiteral("%1:%2:%3").arg(name, type, units));
        }
    }
    return result;
}

bool FactGroupWizardPage::isComplete() const
{
    return !m_domainEdit->text().isEmpty() && m_factsTable->rowCount() > 0;
}

void FactGroupWizardPage::addFact()
{
    int row = m_factsTable->rowCount();
    m_factsTable->insertRow(row);

    // Name column
    auto *nameItem = new QTableWidgetItem;
    nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
    m_factsTable->setItem(row, 0, nameItem);

    // Type combo
    auto *typeCombo = new QComboBox;
    typeCombo->addItems({"double", "float", "uint8", "uint16", "uint32", "int8", "int16", "int32", "bool", "string"});
    m_factsTable->setCellWidget(row, 1, typeCombo);

    // Units edit
    auto *unitsEdit = new QLineEdit;
    unitsEdit->setPlaceholderText(tr("e.g., m/s, deg, %"));
    m_factsTable->setCellWidget(row, 2, unitsEdit);

    // Update completion state
    emit completeChanged();
}

void FactGroupWizardPage::removeFact()
{
    int row = m_factsTable->currentRow();
    if (row >= 0) {
        m_factsTable->removeRow(row);
        emit completeChanged();
    }
}

void FactGroupWizardPage::onDomainChanged()
{
    emit domainChanged(m_domainEdit->text());
    emit completeChanged();
}

} // namespace QGC::Internal
