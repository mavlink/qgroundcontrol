// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../../namespaces.h"

#include <catch/catch.hpp>

#include <optional>
#include <cassert>

namespace QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE {

    template<typename T>
    class GeneratorHandler : public Catch::Generators::IGenerator<T> {
    public:

        GeneratorHandler(Catch::Generators::GeneratorWrapper<T>&& generator)
        : generator{std::move(generator)},
          first_call{true}
        {}

        T const& get() const override {
            assert(!first_call);
            return generator.get();
        }

        bool next() override {
            if (first_call) {
                first_call = false;
                return true;
            }

            return generator.next();
        }

    private:
        Catch::Generators::GeneratorWrapper<T> generator;
        bool first_call;
    };


    /*!
     * Returns a generator wrapping \a generator that ensures that
     * changes its semantics so that the first call to get should be
     * preceded by a call to next.
     *
     * Catch generators require that is valid to call get and obtain a
     * valid value on a generator that was just created.
     * That is, generators should be non-empty and their first value
     * should be initialized on construction.
     *
     * Normally, this is not a problem, and the next implementation of
     * the generator can be simply called in the constructor.
     * But when a generator depends on other generators, doing so will
     * generally skip the first value that the generator
     * produces, as the wrapping generator will need to advance the
     * underlying generator, losing the value in the process.
     * This is in particular, a problem, on generators that are finite
     * or infinite and ordered.
     *
     * To solve the issue, the original value can be saved before
     * advancing the generator or some code can be duplicated or
     * abstracted so that what a new element can be generated without
     * advancing the underlying generator.
     *
     * While this is acceptable, it can be error prone on more complex
     * generators, generators that randomly access a collection of
     * generators and so on.
     *
     * To simplify this process, this generator changes the semantics
     * of the wrapped generator such that the first value of the
     * generator is produced after the first call to next and the
     * generator is considered in an invalid state before the first
     * advancement.
     *
     * In this way, by wrapping all generators that a generator
     * depends on, the implementation required for the first value is
     * the same as the one required for all following values, with
     * regards to the sequencing of next and get operations,
     * simplifying the implementation of dependent generators.
     *
     * Do note that, while the generator returned by this function
     * implments the generator interface that Catch2 requires, it
     * cannot be normally used as a generator as it fails to comply
     * with the first value semantics that a generator requires.
     * Indeed, it should only be used as an intermediate wrapper for
     * the implementation of generators that depends on other
     * generators.
     */
    template<typename T>
    inline Catch::Generators::GeneratorWrapper<T> handler(Catch::Generators::GeneratorWrapper<T>&& generator) {
        return Catch::Generators::GeneratorWrapper<T>(std::unique_ptr<Catch::Generators::IGenerator<T>>(new GeneratorHandler(std::move(generator))));
    }

} // end QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE
