#include "OnboardLogTransport.h"

#include "QmlObjectListModel.h"

OnboardLogTransport::OnboardLogTransport(QObject* parent)
    : QObject(parent), _logEntriesModel(new QmlObjectListModel(this))
{}

OnboardLogTransport::~OnboardLogTransport() = default;
