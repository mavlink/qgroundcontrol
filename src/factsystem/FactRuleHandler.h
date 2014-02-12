#ifndef FACTRULEHANDLER_H
#define FACTRULEHANDLER_H

#include <QObject>
#include <QStringList>
#include <QList>

class UASInterface;

class FactRuleHandler : public QObject
{
Q_OBJECT
    
signals:
    void evaluatedRuleData(int uasId, const QStringList& rawIds, QList<float>& values);
    
public:
    FactRuleHandler(QObject* parent = NULL);
    void setup(UASInterface* uas);
    void setRuleFile(const QString& ruleFile) { _ruleFile = ruleFile; }
    void evaluateRules(void);
    
private:
    QString _ruleFile;
};

#endif