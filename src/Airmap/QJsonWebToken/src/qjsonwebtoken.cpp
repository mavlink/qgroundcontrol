// The MIT License(MIT)
// Copyright(c) <2016> <Juan Gonzalez Burgos>
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "qjsonwebtoken.h"

#include <QDebug>

QJsonWebToken::QJsonWebToken()
{
	// create the header with default algorithm
	setAlgorithmStr("HS256");
	m_jdocPayload = QJsonDocument::fromJson("{}");
    // default for random generation
    m_intRandLength   = 10;
    m_strRandAlphanum = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
}

QJsonWebToken::QJsonWebToken(const QJsonWebToken &other)
{
	this->m_jdocHeader    = other.m_jdocHeader;
	this->m_jdocPayload   = other.m_jdocPayload;
	this->m_byteSignature = other.m_byteSignature;
	this->m_strSecret     = other.m_strSecret;
	this->m_strAlgorithm  = other.m_strAlgorithm;
}

QJsonDocument QJsonWebToken::getHeaderJDoc()
{
	return m_jdocHeader;
}

QString QJsonWebToken::getHeaderQStr(QJsonDocument::JsonFormat format /*= QJsonDocument::JsonFormat::Indented*/)
{
	return m_jdocHeader.toJson(format);
}

bool QJsonWebToken::setHeaderJDoc(QJsonDocument jdocHeader)
{
	if (jdocHeader.isEmpty() || jdocHeader.isNull() || !jdocHeader.isObject())
	{
		return false;
	}

	// check if supported algorithm
	QString strAlgorithm = jdocHeader.object().value("alg").toString();
	if (!isAlgorithmSupported(strAlgorithm))
	{
		return false;
	}

	m_jdocHeader = jdocHeader;

	// set also new algorithm
	m_strAlgorithm = strAlgorithm;

	return true;
}

bool QJsonWebToken::setHeaderQStr(QString strHeader)
{
	QJsonParseError error;
	QJsonDocument tmpHeader = QJsonDocument::fromJson(strHeader.toUtf8(), &error);

	// validate and set header
	if (error.error != QJsonParseError::NoError || !setHeaderJDoc(tmpHeader))
	{
		return false;
	}

	return true;
}

QJsonDocument QJsonWebToken::getPayloadJDoc()
{
	return m_jdocPayload;
}

QString QJsonWebToken::getPayloadQStr(QJsonDocument::JsonFormat format /*= QJsonDocument::JsonFormat::Indented*/)
{
	return m_jdocPayload.toJson(format);
}

bool QJsonWebToken::setPayloadJDoc(QJsonDocument jdocPayload)
{
	if (jdocPayload.isEmpty() || jdocPayload.isNull() || !jdocPayload.isObject())
	{
		return false;
	}

	m_jdocPayload = jdocPayload;

	return true;
}

bool QJsonWebToken::setPayloadQStr(QString strPayload)
{
	QJsonParseError error;
	QJsonDocument tmpPayload = QJsonDocument::fromJson(strPayload.toUtf8(), &error);

	// validate and set payload
	if (error.error != QJsonParseError::NoError || !setPayloadJDoc(tmpPayload))
	{
		return false;
	}

	return true;
}

QByteArray QJsonWebToken::getSignature()
{
	// recalculate
	// get header in compact mode and base64 encoded
	QByteArray byteHeaderBase64  = getHeaderQStr(QJsonDocument::JsonFormat::Compact).toUtf8().toBase64();
	// get payload in compact mode and base64 encoded
	QByteArray bytePayloadBase64 = getPayloadQStr(QJsonDocument::JsonFormat::Compact).toUtf8().toBase64();
	// calculate signature based on chosen algorithm and secret
	m_byteAllData = byteHeaderBase64 + "." + bytePayloadBase64;
	if (m_strAlgorithm.compare("HS256", Qt::CaseInsensitive) == 0)      // HMAC using SHA-256 hash algorithm
	{
		m_byteSignature = QMessageAuthenticationCode::hash(m_byteAllData, m_strSecret.toUtf8(), QCryptographicHash::Sha256);
	}
	else if (m_strAlgorithm.compare("HS384", Qt::CaseInsensitive) == 0) // HMAC using SHA-384 hash algorithm
	{
		m_byteSignature = QMessageAuthenticationCode::hash(m_byteAllData, m_strSecret.toUtf8(), QCryptographicHash::Sha384);
	}
	else if (m_strAlgorithm.compare("HS512", Qt::CaseInsensitive) == 0) // HMAC using SHA-512 hash algorithm
	{
		m_byteSignature = QMessageAuthenticationCode::hash(m_byteAllData, m_strSecret.toUtf8(), QCryptographicHash::Sha512);
	}
	// TODO : support other algorithms
	else
	{
		m_byteSignature = QByteArray();
	}
	// return recalculated
	return m_byteSignature;
}

QByteArray QJsonWebToken::getSignatureBase64()
{
	// note we return through getSignature() to force recalculation
	return getSignature().toBase64();
}

QString QJsonWebToken::getSecret()
{
	return m_strSecret;
}

bool QJsonWebToken::setSecret(QString strSecret)
{
	if (strSecret.isEmpty() || strSecret.isNull())
	{
		return false;
	}

	m_strSecret = strSecret;

    return true;
}

void QJsonWebToken::setRandomSecret()
{
    m_strSecret.resize(m_intRandLength);
    for (int i = 0; i < m_intRandLength; ++i)
    {
        m_strSecret[i] = m_strRandAlphanum.at(rand() % (m_strRandAlphanum.length() - 1));
    }
}

QString QJsonWebToken::getAlgorithmStr()
{
	return m_strAlgorithm;
}

bool QJsonWebToken::setAlgorithmStr(QString strAlgorithm)
{
	// check if supported algorithm
	if (!isAlgorithmSupported(strAlgorithm))
	{
		return false;
	}
	// set algorithm
	m_strAlgorithm = strAlgorithm;
	// modify header
	m_jdocHeader = QJsonDocument::fromJson(QObject::trUtf8("{\"typ\": \"JWT\", \"alg\" : \"").toUtf8()
		                                 + m_strAlgorithm.toUtf8()
		                                 + QObject::trUtf8("\"}").toUtf8());

	return true;
}

QString QJsonWebToken::getToken()
{
	// important to execute first to update m_byteAllData which contains header + "." + payload in base64
	QByteArray byteSignatureBase64 = getSignatureBase64();
	// compose token and return it
	return m_byteAllData + "." + byteSignatureBase64;
}

bool QJsonWebToken::setToken(QString strToken)
{
	// assume base64 encoded at first, if not try decoding
	bool isBase64Encoded = true;
	QStringList listJwtParts = strToken.split(".");
	// check correct size
	if (listJwtParts.count() != 3)
	{
		return false;
	}
	// check all parts are valid using another instance,
	// so we dont overwrite this instance in case of error
	QJsonWebToken tempTokenObj;
	if ( !tempTokenObj.setHeaderQStr(QByteArray::fromBase64(listJwtParts.at(0).toUtf8())) ||
		 !tempTokenObj.setPayloadQStr(QByteArray::fromBase64(listJwtParts.at(1).toUtf8())) )
	{
		// try unencoded
		if (!tempTokenObj.setHeaderQStr(listJwtParts.at(0)) ||
			!tempTokenObj.setPayloadQStr(listJwtParts.at(1)))
		{
			return false;
		}
		else
		{
			isBase64Encoded = false;
		}
	}
	// set parts on this instance
	setHeaderQStr(tempTokenObj.getHeaderQStr());
	setPayloadQStr(tempTokenObj.getPayloadQStr());
	if (isBase64Encoded)
	{ // unencode
		m_byteSignature = QByteArray::fromBase64(listJwtParts.at(2).toUtf8());
	} 
	else
	{
		m_byteSignature = listJwtParts.at(2).toUtf8();
	}
	// allData not valid anymore
	m_byteAllData.clear();
	// success
    return true;
}

QString QJsonWebToken::getRandAlphanum()
{
    return m_strRandAlphanum;
}

void QJsonWebToken::setRandAlphanum(QString strRandAlphanum)
{
    if(strRandAlphanum.isNull())
    {
        return;
    }
    m_strRandAlphanum = strRandAlphanum;
}

int QJsonWebToken::getRandLength()
{
    return m_intRandLength;
}

void QJsonWebToken::setRandLength(int intRandLength)
{
    if(intRandLength < 0 || intRandLength > 1e6)
    {
        return;
    }
    m_intRandLength = intRandLength;
}

bool QJsonWebToken::isValid()
{
	// calculate token on other instance,
	// so we dont overwrite this instance's signature
	QJsonWebToken tempTokenObj = *this;
	if (m_byteSignature != tempTokenObj.getSignature())
	{
		return false;
	}
	return true;
}

QJsonWebToken QJsonWebToken::fromTokenAndSecret(QString strToken, QString srtSecret)
{
	QJsonWebToken tempTokenObj;
	// set Token
	tempTokenObj.setToken(strToken);
	// set Secret
	tempTokenObj.setSecret(srtSecret);
	// return
	return tempTokenObj;
}

void QJsonWebToken::appendClaim(QString strClaimType, QString strValue)
{
	// have to make a copy of the json object, modify the copy and then put it back, sigh
	QJsonObject jObj = m_jdocPayload.object();
	jObj.insert(strClaimType, strValue);
	m_jdocPayload = QJsonDocument(jObj);
}

void QJsonWebToken::removeClaim(QString strClaimType)
{
	// have to make a copy of the json object, modify the copy and then put it back, sigh
	QJsonObject jObj = m_jdocPayload.object();
	jObj.remove(strClaimType);
	m_jdocPayload = QJsonDocument(jObj);
}

bool QJsonWebToken::isAlgorithmSupported(QString strAlgorithm)
{
	// TODO : support other algorithms
	if (strAlgorithm.compare("HS256", Qt::CaseInsensitive) != 0 && // HMAC using SHA-256 hash algorithm
		strAlgorithm.compare("HS384", Qt::CaseInsensitive) != 0 && // HMAC using SHA-384 hash algorithm
		strAlgorithm.compare("HS512", Qt::CaseInsensitive) != 0 /*&& // HMAC using SHA-512 hash algorithm
		strAlgorithm.compare("RS256", Qt::CaseInsensitive) != 0 && // RSA using SHA-256 hash algorithm
		strAlgorithm.compare("RS384", Qt::CaseInsensitive) != 0 && // RSA using SHA-384 hash algorithm
		strAlgorithm.compare("RS512", Qt::CaseInsensitive) != 0 && // RSA using SHA-512 hash algorithm
		strAlgorithm.compare("ES256", Qt::CaseInsensitive) != 0 && // ECDSA using P-256 curve and SHA-256 hash algorithm
		strAlgorithm.compare("ES384", Qt::CaseInsensitive) != 0 && // ECDSA using P-384 curve and SHA-384 hash algorithm
		strAlgorithm.compare("ES512", Qt::CaseInsensitive) != 0*/)  // ECDSA using P-521 curve and SHA-512 hash algorithm
	{
		return false;
	}
	return true;
}

QStringList QJsonWebToken::supportedAlgorithms()
{
	// TODO : support other algorithms
	return QStringList() << "HS256" << "HS384" << "HS512";
}
