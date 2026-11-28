#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>

#include "FirmwareUpgradeController.h"
#include "UnitTest.h"

class FirmwareUpgradeControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _manifestCompleteErrorClearsDownloadingState();
    void _manifestCompleteValidJsonPopulatesManifestInfo();
    UT_PARAMETERIZED_TEST(_manifestReadIsDroppedOnceUnwanted);
    void _manifestReadAfterBoardFoundBuildsFirmwareNames();
    void _manifestIsDownloadedWhenAskedFor();
    void _manifestIsDownloadedOnce();
    void _manifestIsDownloadedAgainAfterFailure();
    void _manifestEntriesAreThoseOfTheWholeManifest();
    void _manifestThatCannotBeReadKeepsTheList_data();
    void _manifestThatCannotBeReadKeepsTheList();
    void _manifestSplitterEntries_data();
    void _manifestSplitterEntries();
    void _manifestSplitterRefusesHugeEntry();
    void _manifestSplitterRefusesEveryPrefix();
    void _px4ReleasesCompleteParsesStableAndBeta();
    void _px4ReleasesCompleteBadJsonKeepsPreviousVersions();
    void _px4ReleasesCompleteNonArrayJsonKeepsPreviousVersions();
    void _px4ReleasesCompleteOnlyStableKeepsBetaEmpty();
    void _binImageSizeMustBeMultipleOf4();
    void _manifestFriendlyNameKeepsBrandName();

private:
    using FirmwareList = QList<FirmwareUpgradeController::ManifestFirmwareInfo_t>;

    /// The firmware list of a manifest parsed whole, as QGC read it before it split the manifest
    static FirmwareList _parsedWhole(const QByteArray& manifest);

    /// Why two firmware lists differ, or an empty string if they are the same
    static QString _difference(const FirmwareList& actual, const FirmwareList& expected);

    /// What ManifestSplitter gives for a manifest
    struct Split
    {
        QList<QByteArray> objects;
        bool ok = false;
        QString error;
    };

    /// Splits @a manifest in chunks of @a chunkSize bytes, or in two at @a cut if it is not 0
    static Split _splitManifest(const QByteArray& manifest, qsizetype chunkSize, qsizetype cut = 0);
};
