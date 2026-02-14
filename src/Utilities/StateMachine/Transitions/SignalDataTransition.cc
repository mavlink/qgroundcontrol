#include "SignalDataTransition.h"

// SignalDataTransition is a template class - most implementation must remain
// in the header for template instantiation. This file provides explicit
// instantiations for common signal argument types to improve compile times.

// Explicit instantiations for common single-argument signals
template class SignalDataTransition<bool>;
template class SignalDataTransition<int>;
template class SignalDataTransition<double>;
template class SignalDataTransition<QString>;
template class SignalDataTransition<QVariant>;

// Explicit instantiations for common two-argument signals
template class SignalDataTransition<bool, bool>;
template class SignalDataTransition<int, int>;
template class SignalDataTransition<QString, QString>;

// Helper function explicit instantiations
// Note: makeSignalDataTransition is also a template and requires the signal
// type to be deduced at call site, so it remains header-only.
