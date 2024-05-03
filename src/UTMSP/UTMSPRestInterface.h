/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <string>
#include <QtCore/QSharedPointer>
#include <QtCore/QObject>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class UTMSPRestInterface {

public:
    UTMSPRestInterface(std::string host, std::string port = "443");
    virtual ~UTMSPRestInterface();

    bool connectNetwork();
    void setBearerToken(const std::string& token);
    std::pair<int, std::string> executeRequest();
    void modifyRequest(std::string target, http::verb method, std::string body = "");
    void setBasicToken(const std::string& basicToken);
    void setHost(std::string target);

private:
    net::io_context                                 _ioc;
    net::ssl::context                               _ssl_ctx;
    net::ip::tcp::resolver                          _resolver{_ioc};
    QSharedPointer<net::ssl::stream<tcp::socket>>   _stream;
    std::mutex                                      _mutex;
    std::string                                     _basicToken;
    std::string                                     _host;
    std::string                                     _port;

    static http::request<http::string_body> _request;
};
