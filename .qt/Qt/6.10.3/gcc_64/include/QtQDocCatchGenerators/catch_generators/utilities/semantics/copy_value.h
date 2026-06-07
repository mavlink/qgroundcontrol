// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../../namespaces.h"

#include <type_traits>

namespace QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE {

    /*!
     * Forces \value to be copied in an expression context.
     *
     * This is used in contexts where inferences of a type that
     * requires generality might identify a reference when ownership
     * is required.
     *
     * Note that the compiler might optmize the copy away. This is a
     * non-issue as we are only interested in breaking lifetime
     * dependencies.
     */
    template<typename T>
    std::remove_reference_t<T> copy_value(T value) { return value; }

} // end QDOC_CATCH_GENERATORS_UTILITIES_ABSOLUTE_NAMESPACE
