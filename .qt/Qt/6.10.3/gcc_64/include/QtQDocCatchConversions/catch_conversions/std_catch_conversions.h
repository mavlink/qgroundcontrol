// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <ostream>
#include <optional>

template<typename T>
inline std::ostream& operator<<(std::ostream& os, const std::optional<T>& optional) {
    os << "std::optional{\n\t";
    if (optional) os << optional.value();
    else os <<"nullopt";

    return os << "\n};";
}
