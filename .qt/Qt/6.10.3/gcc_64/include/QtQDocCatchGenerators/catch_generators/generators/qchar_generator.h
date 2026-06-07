// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../namespaces.h"
#include "../utilities/semantics/move_into_vector.h"
#include "combinators/oneof_generator.h"

#include <catch/catch.hpp>

#include <random>

#include <QChar>

namespace QDOC_CATCH_GENERATORS_ROOT_NAMESPACE {
    namespace QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE {

        class QCharGenerator : public Catch::Generators::IGenerator<QChar> {
        public:
            QCharGenerator(
                char16_t lower_bound = std::numeric_limits<char16_t>::min(),
                char16_t upper_bound = std::numeric_limits<char16_t>::max()
            ) : random_engine{std::random_device{}()},
                distribution{static_cast<unsigned int>(lower_bound), static_cast<unsigned int>(upper_bound)}
            {
                assert(lower_bound <= upper_bound);
                static_cast<void>(next());
            }

            QChar const& get() const override { return current_character; }

            bool next() override {
                current_character = QChar(static_cast<char16_t>(distribution(random_engine)));

                return true;
            }

        private:
            QChar current_character;

            std::mt19937 random_engine;
            std::uniform_int_distribution<unsigned int> distribution;
        };

    } // end QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE


    /*!
     * Returns a generator of that generates elements of QChar whose
     * ucs value is in the range [\a lower_bound, \a upper_bound].
     *
     * When \a lower_bound = \a upper_bound, the generator infinitely
     * generates the same character.
     */
    inline Catch::Generators::GeneratorWrapper<QChar> character(char16_t lower_bound = std::numeric_limits<char16_t>::min(), char16_t upper_bound = std::numeric_limits<char16_t>::max()) {
        return Catch::Generators::GeneratorWrapper<QChar>(std::unique_ptr<Catch::Generators::IGenerator<QChar>>(new QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::QCharGenerator(lower_bound, upper_bound)));
    }


    namespace QDOC_CATCH_GENERATORS_QCHAR_ALPHABETS_NAMESPACE {

        namespace QDOC_CATCH_GENERATORS_TRAITS_NAMESPACE {

            enum class Alphabets : std::size_t {digit, ascii_lowercase, ascii_uppercase, ascii_alpha, ascii_alphanumeric, portable_posix_filename};

            template<Alphabets alphabet>
            struct sizeof_alphabet;

            template<Alphabets alphabet>
            inline constexpr std::size_t sizeof_alphabet_v = sizeof_alphabet<alphabet>::value;

            template <> struct sizeof_alphabet<Alphabets::digit> { static constexpr std::size_t value{'9' - '0'}; };
            template <> struct sizeof_alphabet<Alphabets::ascii_lowercase> { static constexpr std::size_t value{'z' - 'a'}; };
            template<> struct sizeof_alphabet<Alphabets::ascii_uppercase> { static constexpr std::size_t value{'Z' - 'A'}; };
            template<> struct sizeof_alphabet<Alphabets::ascii_alpha> { static constexpr std::size_t value{sizeof_alphabet_v<Alphabets::ascii_lowercase> + sizeof_alphabet_v<Alphabets::ascii_uppercase>}; };
            template<> struct sizeof_alphabet<Alphabets::ascii_alphanumeric>{ static constexpr std::size_t value{sizeof_alphabet_v<Alphabets::ascii_alpha> + sizeof_alphabet_v<Alphabets::digit>}; };

        } // end QDOC_CATCH_GENERATORS_TRAITS_NAMESPACE


        inline Catch::Generators::GeneratorWrapper<QChar> digit() {
            return Catch::Generators::GeneratorWrapper<QChar>(std::unique_ptr<Catch::Generators::IGenerator<QChar>>(new QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::QCharGenerator('0', '9')));
        }

        inline Catch::Generators::GeneratorWrapper<QChar> ascii_lowercase() {
            return Catch::Generators::GeneratorWrapper<QChar>(std::unique_ptr<Catch::Generators::IGenerator<QChar>>(new QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::QCharGenerator('a', 'z')));
        }

        inline Catch::Generators::GeneratorWrapper<QChar> ascii_uppercase() {
            return Catch::Generators::GeneratorWrapper<QChar>(std::unique_ptr<Catch::Generators::IGenerator<QChar>>(new QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::QCharGenerator('A', 'Z')));
        }

        inline Catch::Generators::GeneratorWrapper<QChar> ascii_alpha() {
            return uniform_oneof(QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::move_into_vector(ascii_lowercase(), ascii_uppercase()));
        }

        inline Catch::Generators::GeneratorWrapper<QChar> ascii_alphanumeric() {
            return uniformly_valued_oneof(QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::move_into_vector(ascii_alpha(), digit()), std::vector{traits::sizeof_alphabet_v<traits::Alphabets::ascii_alpha> , traits::sizeof_alphabet_v<traits::Alphabets::digit>});
        }

        inline Catch::Generators::GeneratorWrapper<QChar> portable_posix_filename() {
            return uniformly_valued_oneof(QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::move_into_vector(ascii_alphanumeric(), character('.', '.'), character('-', '-'), character('_', '_')),
                                          std::vector{traits::sizeof_alphabet_v<traits::Alphabets::ascii_alphanumeric>, std::size_t{1}, std::size_t{1}, std::size_t{1}});
        }

    } // end QDOC_CATCH_GENERATORS_QCHAR_ALPHABETS_NAMESPACE


} // end QDOC_CATCH_GENERATORS_ROOT_NAMESPACE
