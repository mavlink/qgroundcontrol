#pragma once

#include "PortableTest.h"

/// Tests for the extraction of an archive into a new or empty directory: what Compression::extractArchive() writes,
/// what it skips and refuses, and that it leaves nothing when it fails
class QGCArchiveExtractionTest : public PortableTest
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testExtractFromResource);
    UT_PARAMETERIZED_TEST(_testArchiveOfManyEntries);
    UT_PARAMETERIZED_TEST(_testEncryptedArchive);
    UT_PARAMETERIZED_TEST(_testZipCompressionMethods);
    UT_PARAMETERIZED_TEST(_testZipEntryWithBadCrc);
    void _testExtractArchiveFiltered();
    void _testProgressCallbackExtract();
    UT_PARAMETERIZED_TEST(_testTruncatedArchive);
    UT_PARAMETERIZED_TEST(_testGzipChecksumOfArchive);
    UT_PARAMETERIZED_TEST(_testSparseEntrySkipped);
    void _testConcurrentExtractions();
    void _testEmptyArchive();
    UT_PARAMETERIZED_TEST(_testOutputDirectoryInUse);
    void _testOutputPathIsResolvedByName();
    UT_PARAMETERIZED_TEST(_testFailedExtractionIsUndone);
    UT_PARAMETERIZED_TEST(_testThrowingCallbackUndoesExtraction);
    UT_PARAMETERIZED_TEST(_testEntriesOfTheSameName);
    UT_PARAMETERIZED_TEST(_testExtractionNeedsRoomOnDisk);
    UT_PARAMETERIZED_TEST(_testOutputThatFailsWhileWritten);
    void _testEntryIsWrittenInPlace();
    UT_PARAMETERIZED_TEST(_testCancelledWhereNoDataIsCopied);
    void _testExtractArchiveToSymlinkedOutputPath();
    UT_PARAMETERIZED_TEST(_testExtractRejectsEscapingNames);
    UT_PARAMETERIZED_TEST(_testExtractSkipsNamesThatAreNotPortable);
    UT_PARAMETERIZED_TEST(_testExtractSkipsSpellingsThatClash);
    void _testSpellingsClashInAnotherNormalisation();
    void _testSpellingsGrowWithNewComponentsOnly();
    UT_PARAMETERIZED_TEST(_testExtractTakesBackslashForSeparator);
    void _testExtractCountsSkippedEntries();
    UT_PARAMETERIZED_TEST(_testExtractNameThatIsNotUtf8);
    void _testExtractLeavesOutOverlongPath();
    UT_PARAMETERIZED_TEST(_testExtractLeavesOutLinks);
    void _testExtractDirectoryEntries();
    UT_PARAMETERIZED_TEST(_testEntriesThatClash);
    void _testExtractRestoresModificationTime();
    UT_PARAMETERIZED_TEST(_testExtractLeavesOutSpecialFiles);
    UT_PARAMETERIZED_TEST(_testExtractToUnusualPath);
    UT_PARAMETERIZED_TEST(_testFormatNotInThisBuild);
};
