#pragma once

#include <QtCore/QSortFilterProxyModel>

class OnboardLogSortFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    Q_DISABLE_COPY_MOVE(OnboardLogSortFilterModel)

public:
    explicit OnboardLogSortFilterModel(QObject* parent = nullptr);

    void resort();
    void setAscending(bool ascending);

signals:
    void countChanged();

protected:
    bool lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight) const override;

private:
    bool _ascending = false;
};
