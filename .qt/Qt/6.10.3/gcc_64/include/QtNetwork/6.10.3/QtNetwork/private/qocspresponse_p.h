// Copyright (C) 2011 Richard J. Moore <rich@kde.org>
// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QOCSPRESPONSE_P_H
#define QOCSPRESPONSE_P_H

#include <private/qtnetworkglobal_p.h>

#include <qsslcertificate.h>
#include <qocspresponse.h>

#include <qshareddata.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QOcspResponsePrivate : public QSharedData
{
public:

    QOcspCertificateStatus certificateStatus = QOcspCertificateStatus::Unknown;
    QOcspRevocationReason revocationReason = QOcspRevocationReason::None;

    QSslCertificate signerCert;
    QSslCertificate subjectCert;
};

inline bool operator==(const QOcspResponsePrivate &lhs, const QOcspResponsePrivate &rhs)
{
    return lhs.certificateStatus == rhs.certificateStatus
            && lhs.revocationReason == rhs.revocationReason
            && lhs.signerCert == rhs.signerCert
            && lhs.subjectCert == rhs.subjectCert;
}

QT_END_NAMESPACE

#endif // QOCSPRESPONSE_P_H
