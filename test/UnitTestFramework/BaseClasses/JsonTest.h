#pragma once

#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "UnitTest.h"

/// Test fixture with helper methods for JSON operations.
///
/// Provides utilities for loading JSON from Qt resources, parsing JSON strings,
/// and comparing JSON structures.
///
/// Example:
/// @code
/// class MyJsonTest : public JsonTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testJsonParsing() {
///         QJsonObject obj = loadJsonObjectFromResource(":/test/data.json");
///         QVERIFY(!obj.isEmpty());
///         QCOMPARE(obj["name"].toString(), "test");
///     }
/// };
/// @endcode
class JsonTest : public UnitTest
{
    Q_OBJECT

public:
    explicit JsonTest(QObject* parent = nullptr) : UnitTest(parent)
    {
    }

protected:
    /// Loads JSON from a Qt resource path and returns as QJsonDocument
    /// @param resourcePath Path starting with ":/" (e.g., ":/unittest/test.json")
    /// @return Parsed document, or null document on error
    QJsonDocument loadJsonFromResource(const QString& resourcePath)
    {
        QFile file(resourcePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "JsonTest: Failed to open resource:" << resourcePath;
            return QJsonDocument();
        }

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JsonTest: Parse error in" << resourcePath << ":" << error.errorString();
            return QJsonDocument();
        }
        return doc;
    }

    /// Loads JSON from a resource and returns as QJsonObject
    QJsonObject loadJsonObjectFromResource(const QString& resourcePath)
    {
        QJsonDocument doc = loadJsonFromResource(resourcePath);
        return doc.isObject() ? doc.object() : QJsonObject();
    }

    /// Loads JSON from a resource and returns as QJsonArray
    QJsonArray loadJsonArrayFromResource(const QString& resourcePath)
    {
        QJsonDocument doc = loadJsonFromResource(resourcePath);
        return doc.isArray() ? doc.array() : QJsonArray();
    }

    /// Loads JSON from a file path (not resource)
    QJsonDocument loadJsonFromFile(const QString& filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "JsonTest: Failed to open file:" << filePath;
            return QJsonDocument();
        }

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JsonTest: Parse error in" << filePath << ":" << error.errorString();
            return QJsonDocument();
        }
        return doc;
    }

    /// Parses a JSON string and returns as QJsonDocument
    QJsonDocument parseJson(const QString& jsonString)
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JsonTest: Parse error:" << error.errorString();
            return QJsonDocument();
        }
        return doc;
    }

    /// Parses a JSON string and returns as QJsonObject
    QJsonObject parseJsonObject(const QString& jsonString)
    {
        QJsonDocument doc = parseJson(jsonString);
        return doc.isObject() ? doc.object() : QJsonObject();
    }

    /// Parses a JSON string and returns as QJsonArray
    QJsonArray parseJsonArray(const QString& jsonString)
    {
        QJsonDocument doc = parseJson(jsonString);
        return doc.isArray() ? doc.array() : QJsonArray();
    }

    /// Compares two JSON objects for equality
    /// @return true if objects have same keys and values
    bool jsonObjectsEqual(const QJsonObject& a, const QJsonObject& b)
    {
        return QJsonDocument(a) == QJsonDocument(b);
    }

    /// Compares two JSON arrays for equality
    bool jsonArraysEqual(const QJsonArray& a, const QJsonArray& b)
    {
        return QJsonDocument(a) == QJsonDocument(b);
    }

    /// Gets a nested value from a JSON object using dot notation
    /// @param obj The root object
    /// @param path Dot-separated path (e.g., "foo.bar.baz")
    /// @return The value at the path, or undefined QJsonValue if not found
    QJsonValue getJsonPath(const QJsonObject& obj, const QString& path)
    {
        QStringList parts = path.split('.');
        QJsonValue current = obj;

        for (const QString& part : parts) {
            if (current.isObject()) {
                current = current.toObject().value(part);
            } else if (current.isArray()) {
                bool ok;
                int index = part.toInt(&ok);
                if (ok && index >= 0 && index < current.toArray().count()) {
                    current = current.toArray().at(index);
                } else {
                    return QJsonValue::Undefined;
                }
            } else {
                return QJsonValue::Undefined;
            }
        }
        return current;
    }

    /// Saves a JSON document to a file
    bool saveJsonToFile(const QJsonDocument& doc, const QString& filePath)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "JsonTest: Failed to create file:" << filePath;
            return false;
        }
        return file.write(doc.toJson()) != -1;
    }

    /// Saves a JSON object to a file
    bool saveJsonToFile(const QJsonObject& obj, const QString& filePath)
    {
        return saveJsonToFile(QJsonDocument(obj), filePath);
    }
};
