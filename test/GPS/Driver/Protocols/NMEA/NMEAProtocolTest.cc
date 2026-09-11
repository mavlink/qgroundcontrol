#include <cstdio>
#include <stdexcept>

#include "NMEAConstellation.h"
#include "NMEAFields.h"
#include "NMEASatelliteEpoch.h"
#include "NMEASentence.h"

#define CHECK(condition)                          \
    do {                                          \
        if (!(condition)) {                       \
            throw std::runtime_error(#condition); \
        }                                         \
    } while (0)

void boundedFields()
{
    CHECK(NMEAFields::number<double>("+47.5").value() == 47.5);
    CHECK(!NMEAFields::number<double>("+-1"));
    CHECK(!NMEAFields::number<double>("12.5garbage"));
    CHECK(!NMEAFields::number<double>("NaN"));
    CHECK(!NMEAFields::number<int>("999999999999999999999"));
    NMEAFields::Cursor fields("1,,3*00");
    int value = 0;
    CHECK(fields.read(value) && value == 1);
    CHECK(!fields.read(value) && fields.valid());
    CHECK(fields.read(value) && value == 3);
}

int main()
{
    try {
        boundedFields();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
    return 0;
}
