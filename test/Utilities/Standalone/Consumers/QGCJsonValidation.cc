#include "JsonValidation.h"

int main()
{
    QString error;
    const QList<JsonParsing::KeyValidateInfo> keys{{"value", QJsonValue::Double, true}};
    return JsonParsing::validateKeysStrict({{"value", 1}}, keys, error) ? 0 : 1;
}
