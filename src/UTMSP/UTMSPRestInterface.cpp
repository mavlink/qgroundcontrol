/****************************************************************************
 *
 * (c) 2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UTMSPRestInterface.h"
#include "UTMSPLogger.h"

#include <QtCore/QList>
#include <QtNetwork/QNetworkInterface>

UTMSPRestInterface::UTMSPRestInterface(std::string host, std::string port):
    _ssl_ctx{boost::asio::ssl::context::tlsv13_client},
    _host(host),
    _port(port)
{

}

UTMSPRestInterface::~UTMSPRestInterface()
{

}


http::request<http::string_body> UTMSPRestInterface::_request;

bool isNetworkAvailable()
{
    bool hasConnectivity = false;

    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& interface : interfaces) {

        // Check if the interface is up and not loopback
        if (interface.isValid() && interface.flags().testFlag(QNetworkInterface::IsUp)
            && !interface.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            hasConnectivity = true;
            break;
        }
    }

    if (!hasConnectivity)
    {
        UTMSP_LOG_DEBUG() << "UTMSPRestInterfaceLog: No network/internet connectivity";
    }
    return hasConnectivity;
}

void UTMSPRestInterface::setHost(std::string target)
{
    _request.set(http::field::content_length, 0);
    _request.version(11);
    _request.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    if(target == "AuthClient"){
        _request.set(http::field::host, "passport.utm.dev.airoplatform.com");
        _request.set(http::field::content_type, "application/x-www-form-urlencoded");
    }
    else if(target == "BlenderClient"){
        _request.set(http::field::host, "blender.utm.dev.airoplatform.com");
        _request.set(http::field::content_type, "application/json");
    }

    _request.set(http::field::accept,  "*/*");
    _request.set(http::field::accept_encoding, "gzip, deflate, br");
    _request.set(http::field::connection, "keep-alive");
}

void UTMSPRestInterface::setBasicToken(const std::string &basicToken){
    _basicToken = basicToken;
    _request.set(http::field::authorization, "Basic "+ _basicToken);
}

bool UTMSPRestInterface::connectNetwork()
{
    if (!isNetworkAvailable())
        return "";

    _ssl_ctx.set_default_verify_paths();
    _stream = QSharedPointer<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>::create(_ioc, _ssl_ctx);

    if (!SSL_set_tlsext_host_name(_stream->native_handle(), _host.c_str()))
    {
        boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
        throw boost::system::system_error{ ec };
    }

    boost::system::error_code ec;

    const auto results = _resolver.resolve(_host, _port, ec);

    if (ec)
    {
        UTMSP_LOG_ERROR() << "UTMSPRestInterfaceLog: Error during resolving: " << ec.message();
        return false;
    }
    else
    {
        UTMSP_LOG_DEBUG() << "UTMSPRestInterfaceLog: Resolving successfull";
    }

    boost::asio::connect(_stream->lowest_layer(), results.begin(), results.end(), ec);

    if (ec)
    {
        UTMSP_LOG_ERROR() << "UTMSPRestInterfaceLog: Error during connection: " << ec.message();
        return false;
    }
    else
    {
        UTMSP_LOG_DEBUG() << "UTMSPRestInterfaceLog: Connection successfull";
    }

    _stream->set_verify_mode(boost::asio::ssl::verify_peer, ec);
    if (ec)
    {
        UTMSP_LOG_ERROR() << "UTMSPRestInterfaceLog: Error during set_verify_mode: " << ec.message();
        return false;
    }
    else
    {
        UTMSP_LOG_DEBUG() << "UTMSPRestInterfaceLog: set_verify_mode successfull";
    }

    _stream->handshake(boost::asio::ssl::stream_base::client, ec);
    if (ec)
    {
        UTMSP_LOG_ERROR() << "UTMSPRestInterfaceLog: Error during handshake: " << ec.message();
        return false;
    }
    else
    {
        UTMSP_LOG_INFO() << "UTMSPRestInterfaceLog: Handshake successfull";
    }

    return true;

}

void UTMSPRestInterface::modifyRequest(std::string target, http::verb method, std::string body)
{
    _request.target(target);
    _request.method(method);
    _request.body() = body;
    _request.prepare_payload();
}

std::pair<int, std::string> UTMSPRestInterface::executeRequest()
{
    if (!isNetworkAvailable())
        return std::pair<int, std::string>(0,"");

    // sendRequest
    std::lock_guard<std::mutex> lock_guard(_mutex);
    boost::system::error_code ec;
    beast::http::write(*_stream.data(), _request, ec);

    if (ec) {
        UTMSP_LOG_ERROR() << "UTMSPRestInterfaceLog: Error during connection: " << ec.message();
    } else {
        UTMSP_LOG_DEBUG() << "UTMSPRestInterfaceLog: Write successfull";
    }

    // ReceivedResponse
    beast::flat_buffer buffer;
    http::response<http::dynamic_body> response;
    http::read(*_stream.data(), buffer, response, ec);

    if (ec) {
        UTMSP_LOG_ERROR() << "UTMSPRestInterfaceLog: Error during connection: " << ec.message();
    } else {
        UTMSP_LOG_DEBUG() << "UTMSPRestInterfaceLog: Read successfull";
    }

    if (response.result() == beast::http::status::ok)
    {
        // Handle successful response
        UTMSP_LOG_INFO() << "UTMSPRestInterfaceLog: Received OK response.";
    }
    else
    {
        UTMSP_LOG_INFO() << "UTMSPRestInterfaceLog: Received response with status code: "<< response.result_int();
    }
    if (response.body().size() == 0) {
        return std::pair<int, std::string>(response.result_int(),"");
    }

    return std::pair<int, std::string>(response.result_int(), boost::beast::buffers_to_string(response.body().data()));
}

void UTMSPRestInterface::setBearerToken(const std::string& token)
{
    _request.set(http::field::authorization, "Bearer " + token);
}
