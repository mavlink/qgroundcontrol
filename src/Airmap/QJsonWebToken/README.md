## Introduction

QJsonWebToken : JWT (JSON Web Token) Implementation in Qt C++

This class implements a subset of the [JSON Web Token](https://en.wikipedia.org/wiki/JSON_Web_Token) 
open standard [RFC 7519](https://tools.ietf.org/html/rfc7519).

Currently this implementation **only supports** the following algorithms:

Alg   | Parameter Value	Algorithm
----- | ------------------------------------
HS256 | HMAC using SHA-256 hash algorithm
HS384 | HMAC using SHA-384 hash algorithm
HS512 | HMAC using SHA-512 hash algorithm

### Include

In order to include this class in your project, in the qt project **.pro** file add the lines:

```cmake
HEADERS  += ./src/qjsonwebtoken.h
SOURCES  += ./src/qjsonwebtoken.cpp
```

### Usage

The repository of this project includes examples that demonstrate the use of this class:

* ```./examples/jwtcreator/```  : Example that shows how to create a JWT with your custom *payload*.

* ```./examples/jwtverifier/``` : Example that shows how to validate a JWT with a given *secret*.

### Limitations

Currently, `QJsonWebToken` validator, can **only** validate tokens created by `QJsonWebToken` itself. This limitation is due to the usage of Qt's [QJsonDocument API](http://doc.qt.io/qt-5/qjsondocument.html), see [this issue for further explanation](https://github.com/juangburgos/QJsonWebToken/issues/3#issuecomment-333056575).

### License

MIT

```
The MIT License(MIT)
Copyright(c) <2016> <Juan Gonzalez Burgos>
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
