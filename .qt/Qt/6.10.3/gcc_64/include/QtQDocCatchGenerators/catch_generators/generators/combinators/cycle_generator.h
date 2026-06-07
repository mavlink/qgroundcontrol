// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../../namespaces.h"
#include "../../utilities/semantics/generator_handler.h"

#include <catch/catch.hpp>

#include <vector>

namespace QDOC_CATCH_GENERATORS_ROOT_NAMESPACE {
    namespace QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE {

        template<typename T>
        class CycleGenerator : public Catch::Generators::IGenerator<T> {
        public:
            CycleGenerator(Catch::Generators::GeneratorWrapper<T>&& generator)
                : generator{std::move(generator)},
                  cache{},
                  cache_index{0}
            {
                // REMARK: We generally handle extracting the first
                // value by using an handler, to avoid code
                // duplication and the possibility of an error.
                // In this specific case, we turn to a more "manual"
                // approach as it better models the cache-based
                // implementation, removing the need to not increment
                // cache_index the first time that next is called.
                cache.emplace_back(this->generator.get());
            }

            T const& get() const override { return cache[cache_index]; }

            bool next() override {
                if (generator.next()) {
                    cache.emplace_back(generator.get());
                    ++cache_index;
                } else {
                    cache_index = (cache_index + 1) % cache.size();
                }

                return true;
            }

        private:
            Catch::Generators::GeneratorWrapper<T> generator;

            std::vector<T> cache;
            std::size_t cache_index;
        };

    } // end QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE

    /*!
     * Returns a generator that behaves like \a generator until \a
     * generator is exhausted, repeating the same generation that \a
     * generator produced, infinitely, afterwards.
     *
     * This is generally intended to produce infinite generators from
     * finite ones.
     *
     * For example, consider a generator that produces values based on
     * another generator that it owns.
     * If the owning generator needs to produce more values that the
     * owned generator can support, it might fail at some point.
     * By cycling over the owned generator, we can extend the sequence
     * of produced values so that enough are generated, in a controlled
     * way.
     *
     * The type T should generally be copyable for this generator to
     * work.
     */
    template<typename T>
    inline Catch::Generators::GeneratorWrapper<T> cycle(Catch::Generators::GeneratorWrapper<T>&& generator) {
        return Catch::Generators::GeneratorWrapper<T>(std::unique_ptr<Catch::Generators::IGenerator<T>>(new QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::CycleGenerator(std::move(generator))));
    }

} // end QDOC_CATCH_GENERATORS_ROOT_NAMESPACE
