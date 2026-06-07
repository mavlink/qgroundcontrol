// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// TODO: Change the include paths to implicitly consider
// `catch_generators` a root directory and change the CMakeLists.txt
// file to make this possible.

#include "../namespaces.h"
#include "qchar_generator.h"
#include "qstring_generator.h"
#include "../utilities/semantics/move_into_vector.h"
#include "../utilities/semantics/generator_handler.h"

#if defined(Q_OS_WINDOWS)

    #include "combinators/cycle_generator.h"

#endif

#include <catch/catch.hpp>

#include <random>

#include <QChar>
#include <QString>
#include <QStringList>
#include <QRegularExpression>

#if defined(Q_OS_WINDOWS)

    #include <QStorageInfo>

#endif

namespace QDOC_CATCH_GENERATORS_ROOT_NAMESPACE {


    struct PathGeneratorConfiguration {
        double multi_device_path_probability{0.5};
        double absolute_path_probability{0.5};
        double directory_path_probability{0.5};
        double has_trailing_separator_probability{0.5};
        std::size_t minimum_components_amount{1};
        std::size_t maximum_components_amount{10};

        PathGeneratorConfiguration& set_multi_device_path_probability(double amount) {
            multi_device_path_probability = amount;
            return *this;
        }

        PathGeneratorConfiguration& set_absolute_path_probability(double amount) {
            absolute_path_probability = amount;
            return *this;
        }

        PathGeneratorConfiguration& set_directory_path_probability(double amount) {
            directory_path_probability = amount;
            return *this;
        }

        PathGeneratorConfiguration& set_has_trailing_separator_probability(double amount) {
            has_trailing_separator_probability = amount;
            return *this;
        }

        PathGeneratorConfiguration& set_minimum_components_amount(std::size_t amount) {
            minimum_components_amount = amount;
            return *this;
        }

        PathGeneratorConfiguration& set_maximum_components_amount(std::size_t amount) {
            maximum_components_amount = amount;
            return *this;
        }
    };

    /*!
     * \class PathGeneratorConfiguration
     * \brief Defines some parameters to customize the generation of
     * paths by a PathGenerator.
     */

    /*!
     * \variable PathGeneratorConfiguration::multi_device_path_probability
     *
     * Every path produced by a PathGenerator configured with a
     * mutli_device_path_probability of n has a probability of n to be
     * \e {Multi-Device} and a probability of 1.0 - n to not be \a
     * {Multi-Device}.
     *
     * multi_device_path_probability should be a value in the range [0.0,
     * 1.0].
     */

    /*!
     * \variable PathGeneratorConfiguration::absolute_path_probability
     *
     * Every path produced by a PathGenerator configured with an
     * absolute_path_probability of n has a probability of n to be \e
     * {Absolute} and a probability of 1.0 - n to be \e {Relative}.
     *
     * absolute_path_probability should be a value in the range [0.0,
     * 1.0].
     */

    /*!
     * \variable PathGeneratorConfiguration::directory_path_probability
     *
     * Every path produced by a PathGenerator configured with a
     * directory_path_probability of n has a probability of n to be \e
     * {To a Directory} and a probability of 1.0 - n to be \e {To a
     * File}.
     *
     * directory_path_probability should be a value in the range [0.0,
     * 1.0].
     */

    /*!
     * \variable PathGeneratorConfiguration::has_trailing_separator_probability
     *
     * Every path produced by a PathGenerator configured with an
     * has_trailing_separator_probability of n has a probability of n
     * to \e {Have a Trailing Separator} and a probability of 1.0 - n
     * to not \e {Have a Trailing Separator}, when this is applicable.
     *
     * has_trailing_separator_probability should be a value in the
     * range [0.0, 1.0].
     */

    /*!
     * \variable PathGeneratorConfiguration::minimum_components_amount
     *
     * Every path produced by a PathGenerator configured with a
     * minimum_components_amount of n will be the concatenation of at
     * least n non \e {device}, non \e {root}, non \e {separator}
     * components.
     *
     * minimum_components_amount should be greater than zero and less
     * than maximum_components_amount.
     */

    /*!
     * \variable PathGeneratorConfiguration::maximum_components_amount
     *
     * Every path produced by a PathGenerator configured with a
     * maximum_components_amount of n will be the concatenation of at
     * most n non \e {device}, non \e {root}, non \e {separator} components.
     *
     * maximum_components_amount should be greater than or equal to
     * minimum_components_amount.
     */


    namespace QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE {

        class PathGenerator : public Catch::Generators::IGenerator<QString> {
        public:
            PathGenerator(
                Catch::Generators::GeneratorWrapper<QString>&& device_component_generator,
                Catch::Generators::GeneratorWrapper<QString>&& root_component_generator,
                Catch::Generators::GeneratorWrapper<QString>&& directory_component_generator,
                Catch::Generators::GeneratorWrapper<QString>&& filename_component_generator,
                Catch::Generators::GeneratorWrapper<QString>&& separator_component_generator,
                PathGeneratorConfiguration configuration = PathGeneratorConfiguration{}
            ) : device_component_generator{QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::handler(std::move(device_component_generator))},
                root_component_generator{QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::handler(std::move(root_component_generator))},
                directory_component_generator{QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::handler(std::move(directory_component_generator))},
                filename_component_generator{QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::handler(std::move(filename_component_generator))},
                separator_component_generator{QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::handler(std::move(separator_component_generator))},
                random_engine{std::random_device{}()},
                components_amount_distribution{configuration.minimum_components_amount, configuration.maximum_components_amount},
                is_multi_device_distribution{configuration.multi_device_path_probability},
                is_absolute_path_distribution{configuration.absolute_path_probability},
                is_directory_path_distribution{configuration.directory_path_probability},
                has_trailing_separator{configuration.has_trailing_separator_probability},
                current_path{}
            {
                assert(configuration.minimum_components_amount > 0);
                assert(configuration.minimum_components_amount <= configuration.maximum_components_amount);

                if (!next())
                    Catch::throw_exception("Not enough values to initialize the first string");
            }

            QString const& get() const override { return current_path; }

            bool next() override {
                std::size_t components_amount{components_amount_distribution(random_engine)};

                current_path = "";

                // REMARK: As per our specification of a path, we
                // do not count device components, and separators,
                // when considering the amount of components in a
                // path.
                // This is a tradeoff that is not necessarily
                // precise.
                // Counting those kinds of components, on one
                // hand, would allow a device component to stands
                // on its own as a path, for example "C:", which
                // might actually be correct in some path format.
                // On the other hand, counting those kinds of
                // components makes the construction of paths for
                // our model much more complex with regards, for
                // example, to the amount of component.
                //
                // Counting device components, since they can
                // appear both in relative and absolute paths,
                // makes the minimum amount of components
                // different for different kinds of paths.
                //
                // Since absolute paths always require a root
                // component, the minimum amount of components for
                // a multi-device absolute path is 2.
                //
                // But an absolute path that is not multi-device
                // would only require one minimum component.
                //
                // Similarly, problems arise with the existence of
                // Windows' relative multi-device path, which
                // require a leading separator component after a
                // device component.
                //
                // This problem mostly comes from our model
                // simplifying the definition of paths quite a bit
                // into binary-forms.
                // This simplifies the code and its structure,
                // sacrificing some precision.
                // The lost precision is almost none for POSIX
                // based paths, but is graver for DOS paths, since
                // they have a more complex specification.
                //
                // Currently, we expect that the paths that QDoc
                // will encounter will mostly be in POSIX-like
                // forms, even on Windows, and aim to support
                // that, such that the simplification of code is
                // considered a better tradeoff compared to the
                // loss of precision.
                //
                // If this changes, the model should be changed to
                // pursue a Windows-first modeling, moving the
                // categorization of paths from the current binary
                // model to the absolute, drive-relative and
                // relative triptych that Windows uses.
                // This more complex model should be able to
                // completely describe posix paths too, making it
                // a superior choice as long as the complexity is
                // warranted.
                //
                // Do note that the model similarly can become
                // inconsistent when used to generate format of
                // paths such as the one used in some resource
                // systems.
                // Those are considered out-of-scope for our needs
                // and were not taken into account when developing
                // this generator.
                if (is_multi_device_distribution(random_engine)) {
                    if (!device_component_generator.next()) return false;
                    current_path += device_component_generator.get();
                }

                // REMARK: Similarly to not counting other form of
                // components, we do not count root components
                // towards the amounts of components that the path
                // has to simplify the code.
                // To support the "special" root path on, for
                // example, posix systems, we require a more
                // complex branching logic that changes based on
                // the path being absolute or not.
                //
                // We don't expect root to be a particularly
                // useful path for QDoc purposes and expect to not
                // have to consider it for our tests.
                // If consideration for it become required, it is
                // possible to test it directly in the affected
                // systemss as a special case.
                //
                // If most systems are affected by the handling of
                // a root path, then the model should be slightly
                // changed to accommodate its generation.
                if (is_absolute_path_distribution(random_engine)) {
                    if (!root_component_generator.next()) return false;

                    current_path += root_component_generator.get();
                }

                std::size_t prefix_components_amount{std::max(std::size_t{1}, components_amount) - 1};
                while (prefix_components_amount > 0) {
                    if (!directory_component_generator.next()) return false;
                    if (!separator_component_generator.next()) return false;

                    current_path += directory_component_generator.get() + separator_component_generator.get();
                    --prefix_components_amount;
                }

                if (is_directory_path_distribution(random_engine)) {
                    if (!directory_component_generator.next()) return false;
                    current_path += directory_component_generator.get();

                    if (has_trailing_separator(random_engine)) {
                        if (!separator_component_generator.next()) return false;
                        current_path += separator_component_generator.get();
                    }
                } else {
                    if (!filename_component_generator.next()) return false;
                    current_path += filename_component_generator.get();
                }

                return true;
            }

        private:
            Catch::Generators::GeneratorWrapper<QString> device_component_generator;
            Catch::Generators::GeneratorWrapper<QString> root_component_generator;
            Catch::Generators::GeneratorWrapper<QString> directory_component_generator;
            Catch::Generators::GeneratorWrapper<QString> filename_component_generator;
            Catch::Generators::GeneratorWrapper<QString> separator_component_generator;

            std::mt19937 random_engine;
            std::uniform_int_distribution<std::size_t> components_amount_distribution;
            std::bernoulli_distribution is_multi_device_distribution;
            std::bernoulli_distribution is_absolute_path_distribution;
            std::bernoulli_distribution is_directory_path_distribution;
            std::bernoulli_distribution has_trailing_separator;

            QString current_path;
        };

    } // end QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE

/*!
    * Returns a generator that produces QStrings that represent a
    * path in a filesystem.
    *
    * A path is formed by the following components, loosely based
    * on the abstraction that is used by std::filesystem::path:
    *
    * \list
    * \li \b {device}:
    *     Represents the device on the filesystem that
    *     the path should be considered in terms of.
    *     This is an optional components that is sometimes
    *     present on multi-device systems, such as Windows, to
    *     distinguish which device the path refers to.
    *     When present, it always appears before any other
    *     component.
    * \li \b {root}:
    *     A special sequence that marks the path as absolute.
    *     This is an optional component that is present, always,
    *     in absolute paths.
    * \li \b {directory}:
    *     A component that represents a directory on the
    *     filesystem that the path "passes-trough".
    *     Zero or more of this components can be present in the
    *     path.
    *     A path pointing to a directory on the filesystem that
    *     is not \e {root} always ends with a component of this
    *     type.
    * \li \b {filename}:
    *     A component that represents a file on the
    *     filesystem.
    *     When this component is present, it is present only once
    *     and always as the last component of the path.
    *     A path that has such a component is a path that points
    *     to a file on the filesystem.
    *     For some path formats, there is no difference in the
    *     format of a \e {filename} and a \e {directory}.
    * \li \b {separator}:
    *     A component that is interleaved between other types of
    *     components to separate them so that they are
    *     recognizable.
    *     A path that points to a directory on the filesystem may
    *     sometimes have a \e {separator} at the end, after the
    *     ending \e {directory} component.
    * \endlist
    *
    * Each component is representable as a string and a path is a
    * concatenation of the string representation of some
    * components, with the following rules:
    *
    * \list
    * \li There is at most one \e {device} component.
    * \li If a \e {device} component is present it always
    * precedes all other components.
    * \li There is at most one \e {root} component.
    * \li If a \e {root} component is present it:
    *     \list
    *     \li Succeeds the \e {device} component if it is present.
    *     \li Precedes every other components if the \e {device}
    *     component is not present.
    *     \endlist
    * \li There are zero or more \e {directory} component.
    * \li There is at most one \e {filename} component.
    * \li If a \e {filename} component is present it always
    * succeeds all other components.
    * \li Between any two successive \e {directory} components
    * there is a \e {separator} component.
    * \li Between each successive \e {directory} and \e
    * {filename} component there is a \e {separator} component.
    * \li If the last component is a \e {directory} component it
    * can be optionally followed by a \e {separator} component.
    * \li At least one component that is not a \e {device}, a \e
    * {root} or \e {separator} component is present.
    * \endlist
    *
    * For example, if "C:" is a \e {device} component, "\\" is a
    * \e {root} component, "\\" is a \e {separator} component,
    * "directory" is a \e {directory} component and "filename" is
    * a \e {filename} component, the following are all paths:
    *
    * "C:\\directory", "C:\\directory\\directory", "C:filename",
    * "directory\\directory\\", "\\directory\\filename", "filename".
    *
    * While the following aren't:
    *
    * "C:", "C:\\", "directory\\C:", "foo", "C:filename\\",
    * "filename\\directory\\filename", "filename\\filename",
    * "directorydirectory"."
    *
    * The format of different components type can be the same.
    * For example, the \e {root} and \e {separator} component in
    * the above example.
    * For the purpose of generation, we do not care about the
    * format itself and consider a component of a certain type
    * depending only on how it is generated/where it is generated
    * from.
    *
    * For example, if every component is formatted as the string
    * "a", the string "aaa" could be a generated path.
    * By the string alone, it is not possible to simply discern
    * which components form it, but it would be possible to
    * generate it if the first "a" is a \a {device} component,
    * the second "a" is a \e {root} component and the third "a"
    * is a \e {directory} or \e {filename} component.
    *
    * A path, is further said to have some properties, pairs of
    * which are exclusive to each other.
    *
    * A path is said to be:
    *
    * \list
    * \li \b {Multi-Device}:
    *     When it contains a \e {device} component.
    * \li \b {Absolute}:
    *     When it contains a \e {root} component.
    *     If the path is \e {Absolute} it is not \e {Relative}.
    * \li \b {Relative}:
    *     When it does not contain a \e {root} component.
    *     If the path is \e {Relative} it is not \e {Absolute}.
    * \li \b {To a Directory}:
    *     When its last component is a \e {directory} component
    *     or a \e {directory} component followed by a \e
    *     {separator} component.
    *     If the path is \e {To a Directory} it is not \e {To a
    *     File}.
    * \li \b {To a File}:
    *     When its last component is a \e {filename}.
    *     If the path is \e {To a File} it is not \e {To a
    *     Directory}.
    * \endlist
    *
    * All path are \e {Relative/Absolute}, \e {To a
    * Directory/To a File} and \e {Multi-Device} or not.
    *
    * Furthermore, a path that is \e {To a Directory} and whose
    * last component is a \e {separator} component is said to \e
    * {Have a Trailing Separator}.
    */
    inline Catch::Generators::GeneratorWrapper<QString> path(
        Catch::Generators::GeneratorWrapper<QString>&& device_generator,
        Catch::Generators::GeneratorWrapper<QString>&& root_component_generator,
        Catch::Generators::GeneratorWrapper<QString>&& directory_generator,
        Catch::Generators::GeneratorWrapper<QString>&& filename_generator,
        Catch::Generators::GeneratorWrapper<QString>&& separator_generator,
        PathGeneratorConfiguration configuration = PathGeneratorConfiguration{}
    ) {
        return Catch::Generators::GeneratorWrapper<QString>(
            std::unique_ptr<Catch::Generators::IGenerator<QString>>(
                new QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::PathGenerator(std::move(device_generator), std::move(root_component_generator), std::move(directory_generator), std::move(filename_generator), std::move(separator_generator), configuration)
            )
        );
    }

    namespace QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE {

        // REMARK: We need a bounded length for the generation of path
        // components as strings.
        // We trivially do not want components to be the empty string,
        // such that we have a minimum length of 1, but the maximum
        // length is more malleable.
        // We don't want components that are too long to avoid
        // incurring in a big performance overhead, as we may generate
        // many of them.
        // At the same time, we want some freedom in having diffent
        // length components.
        // The value that was chosen is based on the general value for
        // POSIX's NAME_MAX, which seems to tend to be 14 on many systems.
        // We see this value as a small enough but not too much value
        // that further brings with itself a relation to paths,
        // increasing our portability even if it is out of scope, as
        // almost no modern respects NAME_MAX.
        // We don't use POSIX's NAME_MAX directly as it may not be available
        // on all systems.
        inline static constexpr std::size_t minimum_component_length{1};
        inline static constexpr std::size_t maximum_component_length{14};

        /*!
         * Returns a generator that generates strings that are
         * suitable to be used as a root component in POSIX paths.
         *
         * As per
         * \l {https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_02},
         * this is any sequence of slash characters that is not of
         * length 2.
         */
        inline Catch::Generators::GeneratorWrapper<QString> posix_root() {
            return uniformly_valued_oneof(
                QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::move_into_vector(
                    string(character('/', '/'), 1, 1),
                    string(character('/', '/'), 3, maximum_component_length)
                ),
                std::vector{1, maximum_component_length - 3}
            );
        }

        /*!
         * Returns a generator that generates strings that are
         * suitable to be used as directory components in POSIX paths
         * and that use an alphabet that should generally be supported
         * by other systems.
         *
         * Components of this kind use the \l
         * {https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_282}{Portable Filename Character Set}.
         */
        inline Catch::Generators::GeneratorWrapper<QString> portable_posix_directory_name() {
            return string(
                QDOC_CATCH_GENERATORS_QCHAR_ALPHABETS_NAMESPACE::portable_posix_filename(),
                minimum_component_length, maximum_component_length
            );
        }

        /*!
         * Returns a generator that generates strings that are
         * suitable to be used as filenames in POSIX paths and that
         * use an alphabet that should generally be supported by
         * other systems.
         *
         * Filenames of this kind use the \l
         * {https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_282}{Portable Filename Character Set}.
         */
        inline Catch::Generators::GeneratorWrapper<QString> portable_posix_filename() {
            // REMARK: "." and ".." always represent directories so we
            // avoid generating them. Other than this, there is no
            // difference between a file name and a directory name.
            return filter([](auto& filename) { return filename != "." && filename != ".."; },  portable_posix_directory_name());
        }

        /*!
         * Returns a generator that generates strings that can be used
         * as POSIX compliant separators.
         *
         * As per \l
         * {https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_271},
         * a separator is a sequence of one or more slashes.
         */
        inline Catch::Generators::GeneratorWrapper<QString> posix_separator() {
            return string(character('/', '/'), minimum_component_length, maximum_component_length);
        }

        /*!
         * Returns a generator that generates strings that can be
         * suitably used as logical drive names in Windows' paths.
         *
         * As per \l
         * {https://docs.microsoft.com/en-us/dotnet/standard/io/file-path-formats#traditional-dos-paths}
         * and \l
         * {https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getlogicaldrives},
         * they are composed of a single letter.
         * Each generated string always follows the lettet with a
         * colon, as it is specifically intended for path usages,
         * where this is required.
         *
         * We use only uppercase letters for the drives names albeit,
         * depending on case sensitivity, lowercase letter could be
         * used.
         */
        inline Catch::Generators::GeneratorWrapper<QString> windows_logical_drives() {
            // REMARK: If a Windows path is generated on Windows
            // itself, we expect that it may be used to interact with
            // the filesystem, similar to how we expect a POSIX path
            // to be used on Linux.
            // For this reason, we only generate a specific drive, the one
            // that contains the current working directory, so that we
            // know it is an actually available drive and to contain the
            // possible modifications to the filesystem to an easily
            // foundable place.

#if defined(Q_OS_WINDOWS)

            auto root_device{QStorageInfo{QDir()}.rootPath().first(1) + ":"};

            return cycle(Catch::Generators::value(std::move(root_device)));

#else

            return Catch::Generators::map(
                [](QString letter){ return letter + ':';},
                string(QDOC_CATCH_GENERATORS_QCHAR_ALPHABETS_NAMESPACE::ascii_uppercase(), 1, 1)
            );

#endif
        }

        /*!
         * Returns a generator that generate strings that can be used
         * as separators in Windows based paths.
         *
         * As per \l
         * {https://docs.microsoft.com/en-us/dotnet/api/system.io.path.directoryseparatorchar?view=net-6.0}
         * and \l
         * {https://docs.microsoft.com/en-us/dotnet/standard/io/file-path-formats#canonicalize-separators},
         * this is a sequence of one or more backward or forward slashes.
         */
        inline Catch::Generators::GeneratorWrapper<QString> windows_separator() {
            return uniform_oneof(
                QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE::move_into_vector(
                    string(character('\\', '\\'), minimum_component_length, maximum_component_length),
                    string(character('/', '/'), minimum_component_length, maximum_component_length)
                )
            );
        }

    } // end QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE

    /*!
     * Returns a generator that generates strings representing
     * POSIX compatible paths.
     *
     * The generated paths follows the format specified in \l
     * {https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_271}.
     *
     * The optional length-requirements, such as PATH_MAX and
     * NAME_MAX, are relaxed away as they are generally not
     * respected by modern systems.
     *
     * It is possible to set the probability of obtaining a
     * relative or absolute path through \a
     * absolute_path_probability and the one of obtaining a path
     * potentially pointing ot a directory or on a file through \a
     * directory_path_probability.
     */
    inline Catch::Generators::GeneratorWrapper<QString> relaxed_portable_posix_path(double absolute_path_probability = 0.5, double directory_path_probability = 0.5) {
        return path(
            // POSIX path are never multi-device, so that we have
            // provide an empty device component generator and set
            // the probability for Multi-Device paths to zero.
            string(character(), 0, 0),
            QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::posix_root(),
            QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::portable_posix_directory_name(),
            QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::portable_posix_filename(),
            QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::posix_separator(),
            PathGeneratorConfiguration{}
                .set_multi_device_path_probability(0.0)
                .set_absolute_path_probability(absolute_path_probability)
                .set_directory_path_probability(directory_path_probability)
        );
    }

    /*!
     * Returns a generator that produces strings that represents
     * traditional DOS paths as defined in \l
     * {https://docs.microsoft.com/en-us/dotnet/standard/io/file-path-formats#traditional-dos-paths}.
     *
     * The directory and filename components of a path generated
     * in this way are, currently, restricted to use a portable
     * character set as defined by POSIX.
     *
     * Do note that most paths themselves, will not be portable, on
     * the whole, albeit they may be valid paths on other systems, as
     * Windows uses a path system that is generally incompatible with
     * other systems.
     *
     * Some possibly valid special path, such as a "C:" or "\"
     * will never be generated.
     */
    inline Catch::Generators::GeneratorWrapper<QString> traditional_dos_path(
        double absolute_path_probability = 0.5,
        double directory_path_probability = 0.5,
        double multi_device_path_probability = 0.5
    ) {
        return path(
            QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::windows_logical_drives(),
            QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::windows_separator(),
            // REMAKR: Windows treats trailing dots as if they were a
            // component of their own, that is, as the special
            // relative paths.
            // This seems to not be correctly handled by Qt's
            // filesystem methods, resulting in inconsistencies when
            // one such path is encountered.
            // To avoid the issue, considering that an equivalent path
            // can be formed by actually having the dots on their own
            // as a component, we filter out all those paths that have
            // trailing dots but are not only composed of dots.
            Catch::Generators::filter(
                [](auto& path){ return !(path.endsWith(".") && path.contains(QRegularExpression("[^.]"))) ; },
                QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::portable_posix_directory_name()
            ),
            QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::portable_posix_filename(),
            QDOC_CATCH_GENERATORS_PRIVATE_NAMESPACE::windows_separator(),
            PathGeneratorConfiguration{}
                .set_multi_device_path_probability(multi_device_path_probability)
                .set_absolute_path_probability(absolute_path_probability)
                .set_directory_path_probability(directory_path_probability)
        );
    }

    // TODO: Find a good way to test the following functions.
    // native_path can probably be tied to the tests for the
    // OS-specific functions, with TEMPLATE_TEST_CASE.
    // The other ones may follow a similar pattern but require a bit
    // more work so that they tie to a specific case instead of the
    // general one.
    // Nonetheless, this approach is both error prone and difficult to
    // parse, because of the required if preprocessor directives,
    // and should be avoided if possible.

    /*!
     * Returns a generator that generates QStrings that represents
     * paths native to the underlying OS.
     *
     * On Windows, paths that refer to a drive always refer to the
     * root drive.
     *
     * native* functions should always be chosen when using paths for
     * testing interfacing with the filesystem itself.
     *
     * System outside Linux, macOS or Windows are not supported.
     */
    inline Catch::Generators::GeneratorWrapper<QString> native_path(double absolute_path_probability = 0.5, double directory_path_probability = 0.5) {
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)

        return relaxed_portable_posix_path(absolute_path_probability, directory_path_probability);

#elif defined(Q_OS_WINDOWS)

        // REMARK: When generating native paths for testing we
        // generally want to avoid relative paths that are
        // drive-specific, as we want them to be tied to a specific
        // working directory that may not be the current directory on
        // the drive.
        // Hence, we avoid generating paths that may have a drive component.
        // For tests where those kind of paths are interesting, a
        // specific Windows-only test should be made, using
        // traditional_dos_path to generate drive-relative paths only.
        return traditional_dos_path(absolute_path_probability, directory_path_probability, 0.0);

#endif
    }

    /*!
     * Returns a generator that generates QStrings that represents
     * paths native to the underlying OS and that are always \e
     * {Relative}.
     *
     * Avoids generating paths that refer to a directory that is not
     * included in the path itself.
     *
     * System outside Linux, macOS or Windows are not supported.
     */
    inline Catch::Generators::GeneratorWrapper<QString> native_relative_path(double directory_path_probability = 0.5) {
        // REMARK: When testing, we generally use some specific
        // directory as a root for relative paths.
        // We want the generated path to be relative to that
        // directory because we need a clean state for the test to
        // be reliable.
        // When generating paths, it is possible, correctly, to
        // have a path that refers to that directory or some
        // parent of it, removing us from the clean state that we
        // need.
        // To avoid that, we filter out paths that end up referring to a directory that is not under our "root" directory.
        //
        // We can think of each generated component moving us
        // further down or up, in case of "..", a directory
        // hierarchy, or keeping us at the same place in case of
        // ".".
        // Any path that ends up under our original "root"
        // directory will safely keep our clean state for testing.
        //
        // Each "." keeps us at the same level in the hierarchy.
        // Each ".." moves us up one level in the hierarchy.
        // Each component that is not "." or ".." moves us down
        // one level into the hierarchy.
        //
        // Then, to avoid referring to the "root" directory or one
        // of its parents, we need to balance out each "." and
        // ".." with the components that precedes or follow their
        // appearance.
        //
        // Since "." keeps us at the same level, it can appear how
        // many times it wants as long as the path referes to the
        // "root" directory or a directory or file under it and at
        // least one other component referes to a directory or
        // file that is under the "root" directory.
        //
        // Since ".." moves us one level up in the hierarchy, a
        // sequence of n ".." components is safe when at least n +
        // 1 non "." or ".." components appear before it.
        //
        // To avoid the above problem, we filter away paths that
        // do not respect those rules.
        return Catch::Generators::filter(
            [](auto& path){
                QStringList components{path.split(QRegularExpression{R"((\\|\/)+)"}, Qt::SkipEmptyParts)};
                int depth{0};

                for (auto& component : components) {
                    if (component == "..")
                        --depth;
                    else if (component != ".")
                        ++depth;

                    if (depth < 0) return false;
                }

                return (depth > 0);
            },
            native_path(0.0, directory_path_probability)
        );
    }

    /*!
     * Returns a generator that generates QStrings that represents
     * paths native to the underlying OS and that are always \e
     * {Relative} and \e {To a File}.
     *
     * System outside Linux, macOS or Windows are not supported.
     */
    inline Catch::Generators::GeneratorWrapper<QString> native_relative_file_path() {
        return native_relative_path(0.0);
    }

    /*!
     * Returns a generator that generates QStrings that represents
     * paths native to the underlying OS and that are always \e
     * {Relative} and \e {To a Directory}.
     *
     * System outside Linux, macOS or Windows are not supported.
     */
    inline Catch::Generators::GeneratorWrapper<QString> native_relative_directory_path() {
        return native_relative_path(1.0);
    }

} // end QDOC_CATCH_GENERATORS_ROOT_NAMESPACE
