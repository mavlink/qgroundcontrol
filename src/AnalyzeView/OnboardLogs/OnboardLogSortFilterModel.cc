#include "OnboardLogSortFilterModel.h"

#include "OnboardLogModel.h"

OnboardLogSortFilterModel::OnboardLogSortFilterModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(false);
    setSortRole(OnboardLogModel::TimeRole);
    sort(0);

    (void) connect(this, &QAbstractItemModel::modelReset, this, &OnboardLogSortFilterModel::countChanged);
    (void) connect(this, &QAbstractItemModel::rowsInserted, this, &OnboardLogSortFilterModel::countChanged);
    (void) connect(this, &QAbstractItemModel::rowsRemoved, this, &OnboardLogSortFilterModel::countChanged);
}

void OnboardLogSortFilterModel::setAscending(bool ascending)
{
    if (_ascending == ascending) {
        return;
    }

    _ascending = ascending;
    resort();
}

void OnboardLogSortFilterModel::resort()
{
    invalidate();
    sort(0);
}

bool OnboardLogSortFilterModel::lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight) const
{
    const QDateTime lhsTime = sourceLeft.data(OnboardLogModel::TimeRole).toDateTime();
    const QDateTime rhsTime = sourceRight.data(OnboardLogModel::TimeRole).toDateTime();
    const bool lhsHasTime = sourceLeft.data(OnboardLogModel::ReceivedRole).toBool() && (lhsTime.toSecsSinceEpoch() > 0);
    const bool rhsHasTime =
        sourceRight.data(OnboardLogModel::ReceivedRole).toBool() && (rhsTime.toSecsSinceEpoch() > 0);
    if (lhsHasTime != rhsHasTime) {
        return lhsHasTime;
    }

    if (lhsHasTime && rhsHasTime && (lhsTime != rhsTime)) {
        return _ascending ? (lhsTime < rhsTime) : (lhsTime > rhsTime);
    }

    const uint lhsId = sourceLeft.data(OnboardLogModel::IdRole).toUInt();
    const uint rhsId = sourceRight.data(OnboardLogModel::IdRole).toUInt();
    return _ascending ? (lhsId < rhsId) : (lhsId > rhsId);
}
