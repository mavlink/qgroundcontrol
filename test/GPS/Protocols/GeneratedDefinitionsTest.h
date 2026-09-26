#pragma once

#include "UnitTest.h"

/// Checks the generated SBF blocks against the SBF reference guide and the recorded corpus.
class GeneratedDefinitionsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _sbfCorpusBlocks_data();
    void _sbfCorpusBlocks();
    void _sbfModeBits();
};
