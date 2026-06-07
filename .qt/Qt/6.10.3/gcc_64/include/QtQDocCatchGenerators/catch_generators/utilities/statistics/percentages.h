// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../../namespaces.h"

#include <cassert>

namespace QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE {

    /*!
     * Returns the percentage of \amount over \a total.
     *
     * \a amount needs to be greater or equal to zero and \a total
     * needs to be greater than zero.
     */
    inline double percent_of(double amount, double total) {
        assert(amount >= 0.0);
        assert(total > 0.0);

        return (amount / total) * 100.0;
    }

    /*!
     * Given the cardinality of a set, returns the percentage
     * probability that applied to every element of the set generates
     * a uniform distribution.
     */
    inline double uniform_probability(std::size_t cardinality) {
        assert(cardinality > 0);

        return (100.0 / static_cast<double>(cardinality));
    }

    /*!
     * Returns a percentage probability that is equal to \a
     * probability.
     *
     * \a probability must be in the range [0.0, 1.0]
     */
    inline double probability_to_percentage(double probability) {
        assert(probability >= 0.0);
        assert(probability <= 1.0);

        return probability * 100.0;
    }

} // end QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE
