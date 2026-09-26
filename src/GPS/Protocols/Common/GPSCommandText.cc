#include "GPSCommandText.h"

#include <iomanip>
#include <locale>
#include <sstream>

QByteArray gpsFixedDecimal(double value, int decimals)
{
    std::ostringstream text;
    text.imbue(std::locale::classic());
    text << std::fixed << std::setprecision(decimals) << value;
    return QByteArray::fromStdString(text.str());
}
