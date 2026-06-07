// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../../namespaces.h"

#include <functional>
#include <optional>
#include <ostream>
#include <unordered_map>

namespace QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE {

    template<typename T>
    using Histogram = std::unordered_map<T, std::size_t>;

    template<typename InputIt, typename GroupBy>
    auto make_histogram(InputIt begin, InputIt end, GroupBy&& group_by) {
        Histogram<std::invoke_result_t<GroupBy, decltype(*begin)>> histogram{};

        while (begin != end) {
            auto key{std::invoke(std::forward<GroupBy>(group_by), *begin)};

            histogram.try_emplace(key, 0);
            histogram[key] += 1;
            ++begin;
        }

        return histogram;
    }

    template<typename T>
    struct DistributionError {
        T value;
        double probability;
        double expected_probability;
    };

    template<typename T>
    inline std::ostream& operator<<(std::ostream& os, const DistributionError<T>& error) {
        return os << "DistributionError{" <<
            "The value { " << error.value <<
            " } appear with a probability of { " << error.probability <<
            " } while a probability of { " << error.expected_probability << " } was expected." <<
            "}";
    }

    // REMARK: The following should really return an Either of unit/error
    // but std::variant in C++ is both extremely unusable and comes with a
    // strong overhead unless certain conditions are met.
    // For this reason, we keep to the less intutitive optional error.

    /*!
    * Returns true when the given \a sequence approximately respects a
    * given distribution.
    *
    * The \a sequence respects a given distribution when the count of
    * each collection of values is a percentage of the total values that
    * is near the percentage probability described by distribution.
    *
    * The values in \a sequence are collected according to \a group_by.
    * \a group_by, given an element of \a sequence, should return a value
    * of some type that represent the category of the inspected value.
    * Values that have the same category share their count.
    *
    * The distribution that should be respected is given by \a
    * probability_of. \a probability_of is a function that takes a
    * category that was produced from a call to \a group_by and returns
    * the expect probability, in percentage, of apperance for that
    * category.
    *
    * The given probability is then compared to the one found by counting
    * the element of \a sequence under \a group_by, to ensure that it
    * matches.
    *
    * The margin of error for the comparison is given, in percentage
    * points, by \a margin.
    * The approximation uses an absolute comparison and scales the
    * margin inversely based on the size of \a sequence, to account for the
    * precision of the data set itself.
    *
    * When the distribution is not respected, a DistributionError is
    * returned enclosed in an optional value.
    * The error allows reports which the first category for which the
    * comparison failed, along with its expected probability and the one
    * that was actually inferred from \a sequence.
    */
    template<typename T, typename GroupBy, typename ProbabilityOf>
    std::optional<DistributionError<T>> respects_distribution(std::vector<T>&& sequence, GroupBy&& group_by, ProbabilityOf&& probability_of, double margin = 33) {
        std::size_t data_point_amount{sequence.size()};

        // REMARK: We scale the margin based on the data set to allow for
        // an easier change in downstream tests.
        // The precision required for the approximation will vary
        // depending on how many values we generate.
        // The amount of values we generate depends on how much time we
        // want the tests to take.
        // This amount may change in the future. For example, as code is
        // added and tests are added, we might need some expensive
        // computations here and there.
        // Sometimes, this will increase the test suite runtime without an
        // obvious way of improving the performance of the underlying code
        // to reduce it.
        // In those cases, the total run time can be decreased by running
        // less generations for battle-tested tests.
        // If some code has not been changed for a long time, it will have
        // had thousands of generations by that point, giving us a good
        // degree of certainty of it not being bugged (for whatever bugs
        // the tests account for).
        // Then, running a certain amount of generation is not required
        // anymore such that some of them can be optimized out.
        // For tests like the one using this function, where our ability
        // to test is always dependent on the amount of generations,
        // changing the generated amount will mean that we will need to
        // change our conditions too, potentially changing the meaning of
        // the test.
        // To take this into account, we perform a scaling on the
        // condition itself, so that if the amount of data points that are
        // generated changes, we do not generally have to change anything
        // in the condition.
        //
        // For this case, we scale logarithmically_10 for the simple
        // reason that we tend to generate values in power of tens,
        // starting with the 100 values default that Quickcheck used.
        //
        // The default value for the margin on which the scaling is based,
        // was chosen heuristically.
        // As we expect generation under 10^3 to be generally meaningless
        // for this kind of testing, the value was chosen so that it would
        // start to normalize around that amount.
        // Deviation of about 5-10% were identified trough various
        // generations for an amount of data points near 1000, while a
        // deviation of about 1-3% was identified with about 10000 values.
        // With the chosen default value, the scaling approaches those
        // percentage points with some margin of error.
        //
        // We expect up to a 10%, or a bit more, deviation to be suitable
        // for our purposes, as it would still allow for a varied
        // distribution in downstream consumers.
        double scaled_margin{margin * (1.0/std::log10(data_point_amount))};

        auto histogram{make_histogram(sequence.begin(), sequence.end(), std::forward<GroupBy>(group_by))};

        for (auto& bin : histogram) {
            auto [key, count] = bin;

            double actual_percentage{percent_of(static_cast<double>(count), static_cast<double>(data_point_amount))};
            double expected_percentage{std::invoke(std::forward<ProbabilityOf>(probability_of), key)};

            if (!(actual_percentage == Approx(expected_percentage).margin(scaled_margin)))
                return std::make_optional(DistributionError<T>{key, actual_percentage, expected_percentage});
        }

        return std::nullopt;
    }

} // end QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE
