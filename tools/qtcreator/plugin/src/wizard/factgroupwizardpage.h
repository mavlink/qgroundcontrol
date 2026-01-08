#pragma once

#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QTableWidget;
class QComboBox;
QT_END_NAMESPACE

namespace QGC::Internal {

class FactGroupWizardPage : public QWizardPage
{
    Q_OBJECT
    Q_PROPERTY(QString domain READ domain WRITE setDomain NOTIFY domainChanged)
    Q_PROPERTY(QStringList facts READ facts)

public:
    explicit FactGroupWizardPage(QWidget *parent = nullptr);

    QString domain() const;
    void setDomain(const QString &domain);
    QStringList facts() const;

    bool isComplete() const override;

signals:
    void domainChanged(const QString &domain);

private slots:
    void addFact();
    void removeFact();
    void onDomainChanged();

private:
    void setupUi();

    QLineEdit *m_domainEdit = nullptr;
    QTableWidget *m_factsTable = nullptr;
};

} // namespace QGC::Internal
