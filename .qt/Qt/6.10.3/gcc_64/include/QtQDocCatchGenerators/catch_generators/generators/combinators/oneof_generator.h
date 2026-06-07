// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../../namespaces.h"
#include "../../utilities/statistics/percentages.h"
#include "../../utilities/semantics/generator_handler.h"

#include <catch/catch.hpp>

#include <vector>
#include <random>
#include <algorithm>
#include <numeric>

namespace QDOC_CATCH_GENERATORS_ROOT_NAMESPACE {
    namespace QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE {

        template<typename T>
        class OneOfGenerator : public Catch::Generators::IGenerator<T> {
        public:
            OneOfGenerator(
                std::vector<Catch::Generators::GeneratorWrapper<T>>&& generators,
                const std::vector<double>& weights
            ) : generators{std::move(generators)},
                random_engine{std::random_device{}()},
                choice_distribution{weights.cbegin(), weights.cend()}
            {
                assert(weights.size() == this->generators.size());
                assert(std::reduce(weights.cbegin(), weights.cend()) == Approx(100.0));

                std::transform(
                    this->generators.begin(), this->generators.end(), this->generators.begin(),
                    [](auto& generator){ return QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::handler(std::move(generator)); }
                );

                static_cast<void>(next());
            }

            T const& get() const override { return current_value; }

            bool next() override {
                std::size_t generator_index{choice_distribution(random_engine)};

                if (!generators[generator_index].next()) return false;
                current_value = generators[generator_index].get();

                return true;
            }

        private:
            std::vector<Catch::Generators::GeneratorWrapper<T>> generators;

            std::mt19937 random_engine;
            std::discrete_distribution<std::size_t> choice_distribution;

            T current_value;
        };

    } // end QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE

    /*!
     * Returns a generator whose set of elements is the union of the
     * set of elements of the generators in \a generators.
     *
     * Each time the generator produces a value, a generator from \a
     * generators is randomly chosen to produce the value.
     *
     * The distribution for the choice is given by \a weights.
     * The \e {ith} element in \a weights represent the percentage
     * probability of the \e {ith} element of \a generators to be
     * chosen.
     *
     * It follows that the size of \a weights must be the same as the
     * size of \a generators.
     *
     * Furthermore, the sum of elements in \a weights should be a
     * hundred.
     *
     * The generator produces values until a generator that is chosen
     * to produce a value is unable to do so.
     * The first such generator to do so will stop the generation
     * independently of the availability of the other generators.
     *
     * Similarly, values will be produced as long as the chosen
     * generator can produce a value, independently of the other
     * generators being exhausted already.
     */
    template<typename T>
    inline Catch::Generators::GeneratorWrapper<T> oneof(
        std::vector<Catch::Generators::GeneratorWrapper<T>>&& generators,
        const std::vector<double>& weights
    ) {
        return Catch::Generators::GeneratorWrapper<T>(std::unique_ptr<Catch::Generators::IGenerator<T>>(new QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::OneOfGenerator(std::move(generators), weights)));
    }


    /*!
     * Returns a generator whose set of elements is the union of the
     * set of elements of the generators in \a generators and in which
     * the distribution of the generated elements is uniform over \a
     * generators.
     *
     * Each time the generator produces a value, a generator from \a
     * generators is randomly chosen to produce the value.
     *
     * Each generator from \a generators has the same chance of being
     * chosen.
     *
     * Do note that the distribution over the set of values is not
     * necessarily uniform.
     *
     * The generator produces values until a generator that is chosen
     * to produce a value is unable to do so.
     * The first such generator to do so will stop the generation
     * independently of the availability of the other generators.
     *
     * Similarly, values will be produced as long as the chosen
     * generator can produce a value, independently of the other
     * generators being exhausted already.
     */
    template<typename T>
    inline Catch::Generators::GeneratorWrapper<T> uniform_oneof(
        std::vector<Catch::Generators::GeneratorWrapper<T>>&& generators
    ) {
        std::vector<double> weights(
            generators.size(),
            QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::uniform_probability(generators.size())
        );
        return oneof(std::move(generators), std::move(weights));
    }

    /*!
     * Returns a generator whose set of elements is the union of the
     * set of elements of the generators in \a generators and in which
     * the distribution of the generated elements is uniform over the
     * elements of \a generators.
     *
     * The generators in \a generator should have a uniform
     * distribution and be finite.
     * If the set of elements that the generators in \a generator is
     * not disjoint, the distribution will be skewed towards repeated
     * elements.
     *
     * Each time the generator produces a value, a generator from \a
     * generators is randomly chosen to produce the value.
     *
     * Each generator from \a generators has a probability of being
     * chosen based on the proportion of the cardinality of the subset
     * it produces.
     *
     * The \e {ith} element of \a amounts should contain the
     * cardinality of the set produced by the \e {ith} generator in \a
     * generators.
     *
     * The generator produces values until a generator that is chosen
     * to produce a value is unable to do so.
     * The first such generator to do so will stop the generation
     * independently of the availability of the other generators.
     *
     * Similarly, values will be produced as long as the chosen
     * generator can produce a value, independently of the other
     * generators being exhausted already.
     */
    template<typename T>
    inline Catch::Generators::GeneratorWrapper<T> uniformly_valued_oneof(
        std::vector<Catch::Generators::GeneratorWrapper<T>>&& generators,
        const std::vector<std::size_t>& amounts
    ) {
        std::size_t total_amount{std::accumulate(amounts.cbegin(), amounts.cend(), std::size_t{0})};

        std::vector<double> weights;
        weights.reserve(amounts.size());

        std::transform(
            amounts.cbegin(), amounts.cend(),
            std::back_inserter(weights),
            [total_amount](auto element){ return QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::percent_of(static_cast<double>(element), static_cast<double>(total_amount)); }
        );

        return oneof(std::move(generators), std::move(weights));
    }

} // end QDOC_CATCH_GENERATORS_ROOT_NAMESPACE
