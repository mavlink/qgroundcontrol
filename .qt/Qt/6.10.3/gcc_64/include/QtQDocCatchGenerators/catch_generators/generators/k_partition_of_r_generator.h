// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../namespaces.h"

#include <catch/catch.hpp>

#include <random>
#include <numeric>
#include <algorithm>

namespace QDOC_CATCH_GENERATORS_ROOT_NAMESPACE {
    namespace QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE {

        class KPartitionOfRGenerator : public Catch::Generators::IGenerator<std::vector<double>> {
        public:
            KPartitionOfRGenerator(double r, std::size_t k)
                : random_engine{std::random_device{}()},
                  interval_distribution{0.0, r},
                  k{k},
                  r{r},
                  current_partition(k)
            {
                assert(r >= 0.0);
                assert(k >= 1);

                static_cast<void>(next());
            }

            std::vector<double> const& get() const override { return current_partition; }

            bool next() override {
                if (k == 1) current_partition[0] = r;
                else {
                    // REMARK: The following wasn't formally proved
                    // but is based on intuition.
                    // It is probably erroneous but is expected to be
                    // good enough for our case.

                    // REMARK: We aim to provide a non skewed
                    // distribution for the elements of the partition.
                    //
                    // The reasoning for this is to ensure that our
                    // testing surface has a good chance of hitting
                    // many of the available elements between the many
                    // runs.
                    //
                    // To approximate this, a specific algorithm was chosen.
                    // The following code can be intuitively seen as doing the following:
                    //
                    // Consider an interval [0.0, r] on the real line, where r > 0.0.
                    //
                    // k - 1 > 0 elements of the interval are chosen,
                    // partitioning the interval into disjoint
                    // sub-intervals.
                    //
                    // ---------------------------------------------------------------------------------------------------------------------
                    // |     |                   |                                                       |                                 |
                    // 0    k_1                 k_2                                                     k_3                                r
                    // |     |                   |                                                       |                                 |
                    // _______--------------------_______________________________________________________-----------------------------------
                    // k_1 - 0     k_2 - k_1                           k_3 - k_2                                       r - k_3
                    //    p1          p2                                  p3                                            p4
                    //
                    // The length of each sub interval is chosen as one of the elements of the partition.
                    //
                    // Trivially, the sum of the chosen elements is r.
                    //
                    // Furthermore, as long as the distribution used
                    // to choose the elements of the original interval
                    // is uniform, the probability of each partition
                    // being produced should tend to being uniform
                    // itself.
                    std::generate(current_partition.begin(), current_partition.end() - 1, [this](){ return interval_distribution(random_engine); });

                    current_partition.back() = r;

                    std::sort(current_partition.begin(), current_partition.end());
                    std::adjacent_difference(current_partition.begin(), current_partition.end(), current_partition.begin());
                }

                return true;
            }

        private:
            std::mt19937 random_engine;
            std::uniform_real_distribution<double> interval_distribution;

            std::size_t k;
            double r;

            std::vector<double> current_partition;
        };

    } // end QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE

    /*!
     * Returns a generator that generates collections of \a k elements
     * whose sum is \a r.
     *
     * \a r must be a real number greater or euqal to zero and \a k
     * must be a natural number greater than zero.
     *
     * The generated partitions tends to be uniformely distributed
     * over the set of partitions of r.
     */
    inline Catch::Generators::GeneratorWrapper<std::vector<double>> k_partition_of_r(double r, std::size_t k) {
        return Catch::Generators::GeneratorWrapper<std::vector<double>>(std::unique_ptr<Catch::Generators::IGenerator<std::vector<double>>>(new QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::KPartitionOfRGenerator(r, k)));
    }

} // end QDOC_CATCH_GENERATORS_ROOT_NAMESPACE
