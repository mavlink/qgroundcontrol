#pragma once

/// @file
/// @brief Convenience header that includes all test fixtures
///
/// Include this header to get access to all test fixtures:
/// - Coord namespace: origin(), zurich(), sanFrancisco(), polygon(), waypointPath()
/// - Mission namespace: addWaypoints(), addTakeoff(), addLand(), addRTL()
/// - NetworkReplyFixture, SingleInstanceLockFixture
/// - TempFileFixture, TempJsonFileFixture, TempDirFixture

#include "CoordFixtures.h"
#include "MissionFixtures.h"
#include "RAIIFixtures.h"
#include "LocalHttpTestServer.h"
