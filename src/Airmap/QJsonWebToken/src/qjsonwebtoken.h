/**
\file
\version 1.0
\date    22/06/2016
\author  JGB
\brief   JWT (JSON Web Token) Implementation in Qt C++
*/

// The MIT License(MIT)
// Copyright(c) <2016> <Juan Gonzalez Burgos>
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef QJSONWEBTOKEN_H
#define QJSONWEBTOKEN_H

#include <QObject>
#include <QMessageAuthenticationCode>
#include <QJsonDocument>
#include <QJsonObject>

/**

\brief   QJsonWebToken : JWT (JSON Web Token) Implementation in Qt C++

## Introduction

This class implements a subset of the [JSON Web Token](https://en.wikipedia.org/wiki/JSON_Web_Token) 
open standard [RFC 7519](https://tools.ietf.org/html/rfc7519).

Currently this implementation only supports the following algorithms:

Alg   | Parameter Value	Algorithm
----- | ------------------------------------
HS256 | HMAC using SHA-256 hash algorithm
HS384 | HMAC using SHA-384 hash algorithm
HS512 | HMAC using SHA-512 hash algorithm

### Include

In order to include this class in your project, in the qt project **.pro** file add the lines:

```
HEADERS  += ./src/qjsonwebtoken.h
SOURCES  += ./src/qjsonwebtoken.cpp
```

### Usage

The repository of this project includes examples that demonstrate the use of this class:

* ./examples/jwtcreator/  : Example that shows how to create a JWT with your custom *payload*.

* ./examples/jwtverifier/ : Example that shows how to validate a JWT with a given *secret*.

*/
class QJsonWebToken
{

public:

	/**

	\brief Constructor.
	\return A new instance of QJsonWebToken.

	Creates a default QJsonWebToken instance with *HS256 algorithm*, empty *payload*
	and empty *secret*.

	*/
    QJsonWebToken();                            // TODO : improve with params

	/**

	\brief Copy Construtor.
	\param other Other QJsonWebToken to copy from.
	\return A new instance of QJsonWebToken with same contents as the *other* instance.
	
	Copies to the new instance the JWT *header*, *payload*, *signature*, *secret* and *algorithm*.

	*/
	QJsonWebToken(const QJsonWebToken &other); 

	/**

	\brief Returns the JWT *header* as a QJsonDocument.
	\return JWT *header* as a QJsonDocument.

	*/
	QJsonDocument getHeaderJDoc();

	/**

	\brief Returns the JWT *header* as a QString.
	\param format Defines the format of the JSON returned.
	\return JWT *header* as a QString.

	Format can be *QJsonDocument::JsonFormat::Indented* or *QJsonDocument::JsonFormat::Compact*

	*/
	QString       getHeaderQStr(QJsonDocument::JsonFormat format = QJsonDocument::JsonFormat::Indented);

	/**

	\brief Sets the JWT *header* from a QJsonDocument.
	\param jdocHeader JWT *header* as a QJsonDocument.
	\return true if the header was set, false if the header was not set.

	This method checks for a valid header format and returns false if the header is invalid.

	*/
	bool          setHeaderJDoc(QJsonDocument jdocHeader);

	/**

	\brief Sets the JWT *header* from a QString.
	\param jdocHeader JWT *header* as a QString.
	\return true if the header was set, false if the header was not set.

	This method checks for a valid header format and returns false if the header is invalid.

	*/
	bool          setHeaderQStr(QString strHeader);

	/**

	\brief Returns the JWT *payload* as a QJsonDocument.
	\return JWT *payload* as a QJsonDocument.

	*/
	QJsonDocument getPayloadJDoc();

	/**

	\brief Returns the JWT *payload* as a QString.
	\param format Defines the format of the JSON returned.
	\return JWT *payload* as a QString.

	Format can be *QJsonDocument::JsonFormat::Indented* or *QJsonDocument::JsonFormat::Compact*

	*/
	QString       getPayloadQStr(QJsonDocument::JsonFormat format = QJsonDocument::JsonFormat::Indented);

	/**

	\brief Sets the JWT *payload* from a QJsonDocument.
	\param jdocHeader JWT *payload* as a QJsonDocument.
	\return true if the payload was set, false if the payload was not set.

	This method checks for a valid payload format and returns false if the payload is invalid.

	*/
	bool          setPayloadJDoc(QJsonDocument jdocPayload);

	/**

	\brief Sets the JWT *payload* from a QString.
	\param jdocHeader JWT *payload* as a QString.
	\return true if the payload was set, false if the payload was not set.

	This method checks for a valid payload format and returns false if the payload is invalid.

	*/
	bool          setPayloadQStr(QString strPayload);

	/**

	\brief Returns the JWT *signature* as a QByteArray.
	\return JWT *signature* as a decoded QByteArray.

	Recalculates the JWT signature given the current *header*, *payload*, *algorithm* and
	*secret*.

	\warning This method overwrites the old signature internally. This could be undesired when
	the signature was obtained by copying from another QJsonWebToken using the copy constructor.

	*/
	QByteArray    getSignature();		// WARNING overwrites signature

	/**

	\brief Returns the JWT *signature* as a QByteArray.
	\return JWT *signature* as a **base64 encoded** QByteArray.

	Recalculates the JWT signature given the current *header*, *payload*, *algorithm* and
	*secret*. Then encodes the calculated signature using base64 encoding.

	\warning This method overwrites the old signature internally. This could be undesired when
	the signature was obtained by copying from another QJsonWebToken using the copy constructor.

	*/
	QByteArray    getSignatureBase64(); // WARNING overwrites signature

	/**

	\brief Returns the JWT *secret* as a QString.
	\return JWT *secret* as a QString.

	*/
	QString       getSecret();

	/**

	\brief Sets the JWT *secret* from a QString.
	\param strSecret JWT *secret* as a QString.
	\return true if the secret was set, false if the secret was not set.

	This method checks for a valid secret format and returns false if the secret is invalid.

	*/
	bool          setSecret(QString strSecret);

	/**

	\brief Creates and sets a random secret.

	This method creates a random secret with the length defined by QJsonWebToken::getRandLength(),
	and the characters defined by QJsonWebToken::getRandAlphanum().

	\sa QJsonWebToken::getRandLength().
	\sa QJsonWebToken::getRandAlphanum().

	*/
    void          setRandomSecret();

	/**

	\brief Returns the JWT *algorithm* as a QString.
	\return JWT *algorithm* as a QString.

	*/
	QString       getAlgorithmStr();

	/**

	\brief Sets the JWT *algorithm* from a QString.
	\param strAlgorithm JWT *algorithm* as a QString.
	\return true if the algorithm was set, false if the algorithm was not set.

	This method checks for a valid supported algorithm. Valid values are:

	"HS256", "HS384" and "HS512".

	\sa QJsonWebToken::supportedAlgorithms().

	*/
	bool          setAlgorithmStr(QString strAlgorithm);

	/**

	\brief Returns the complete JWT as a QString.
	\return Complete JWT as a QString.

	The token has the form:

	```
	xxxxx.yyyyy.zzzzz
	```

	where:
	
	- *xxxxx* is the *header* enconded in base64.
	- *yyyyy* is the *payload* enconded in base64.
	- *zzzzz* is the *signature* enconded in base64.

	*/
	QString       getToken();

	/**

	\brief Sets the complete JWT as a QString.
	\param strToken Complete JWT as a QString.
	\return true if the complete JWT was set, false if not set.

	This method checks for a valid JWT format. It overwrites the *header*,
	*payload* , *signature* and *algorithm*. It does **not** overwrite the secret.

	\sa QJsonWebToken::getToken().

	*/
	bool          setToken(QString strToken);

	/**

	\brief Returns the current set of characters used to create random secrets.
	\return Set of characters as a QString.

	The default value is "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	\sa QJsonWebToken::setRandomSecret()
	\sa QJsonWebToken::setRandAlphanum()

	*/
    QString       getRandAlphanum();

	/**

	\brief Sets the current set of characters used to create random secrets.
	\param strRandAlphanum Set of characters as a QString.

	\sa QJsonWebToken::setRandomSecret()
	\sa QJsonWebToken::getRandAlphanum()

	*/
    void          setRandAlphanum(QString strRandAlphanum);

	/**

	\brief Returns the current length used to create random secrets.
	\return Length of random secret as a QString.

	The default value is 10;

	\sa QJsonWebToken::setRandomSecret()
	\sa QJsonWebToken::setRandLength()

	*/
    int           getRandLength();

	/**

	\brief Sets the current length used to create random secrets.
	\param intRandLength Length of random secret.

	\sa QJsonWebToken::setRandomSecret()
	\sa QJsonWebToken::getRandLength()

	*/
    void          setRandLength(int intRandLength);

	/**

	\brief Checks validity of current JWT with respect to secret.
	\return true if the JWT is valid with respect to secret, else false.

	Uses the current *secret* to calculate a temporary *signature* and compares it to the
	current signature to check if they are the same. If they are, true is returned, if not then
	false is returned.

	*/
	bool          isValid();

	/**

	\brief Creates a QJsonWebToken instance from the complete JWT and a secret.
	\param strToken Complete JWT as a QString.
	\param srtSecret Secret as a QString.
	\return Instance of QJsonWebToken.

	The JWT provided must have a valid format, else a QJsonWebToken instance with default
	values will be returned.

	*/
	static QJsonWebToken fromTokenAndSecret(QString strToken, QString srtSecret);

	/**

	\brief Returns a list of the supported algorithms.
	\return List of supported algorithms as a QStringList.

	*/
	static QStringList supportedAlgorithms();

	/**

	\brief Convenience method to append a claim to the *payload*.
	\param strClaimType The claim type as a QString.
	\param strValue The value type as a QString.

	Both parameters must be non-empty. If the claim type already exists, the current
	claim value is updated.

	*/
	void appendClaim(QString strClaimType, QString strValue);

	/**

	\brief Convenience method to remove a claim from the *payload*.
	\param strClaimType The claim type as a QString.

	If the claim type does not exist in the *payload*, then this method does nothins.

	*/
	void removeClaim(QString strClaimType);

private:
	// properties
	QJsonDocument m_jdocHeader;	   // unencoded
	QJsonDocument m_jdocPayload;   // unencoded
	QByteArray    m_byteSignature; // unencoded
	QString       m_strSecret;
	QString       m_strAlgorithm;

    int           m_intRandLength  ;
    QString       m_strRandAlphanum;

	// helpers
	QByteArray    m_byteAllData;

	bool isAlgorithmSupported(QString strAlgorithm);
};

#endif // QJSONWEBTOKEN_H
