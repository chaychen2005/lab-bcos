/**
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 *
 * @file RpcTest.cpp
 * @author: caryliao
 * @date 2018-11-14
 */
#include "FakeModule.h"
#include <libdevcrypto/Common.h>
#include <libethcore/CommonJS.h>
#include <librpc/Rpc.h>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <string>

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
using namespace dev;
using namespace dev::rpc;
using namespace dev::ledger;
using namespace dev::demo;
using namespace dev::rpc;
using namespace jsonrpc;

// Report a failure
void fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
    websocket::stream<tcp::socket> m_ws;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    boost::beast::multi_buffer m_buffer;
    std::map<std::string, std::function<void(const Json::Value&, Json::Value&)> > m_mapRpc;
    std::shared_ptr<RpcFace> m_rpcFace;

    void setMapRpc()
    {
        m_mapRpc.insert(
            std::make_pair("blockNumber", std::bind(&RpcFace::blockNumberI, m_rpcFace,
                                              std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(
            std::make_pair("pbftView", std::bind(&RpcFace::pbftViewI, m_rpcFace,
                                           std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(
            std::make_pair("consensusStatus", std::bind(&RpcFace::consensusStatusI, m_rpcFace,
                                                  std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(
            std::make_pair("syncStatus", std::bind(&RpcFace::syncStatusI, m_rpcFace,
                                             std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(
            std::make_pair("version", std::bind(&RpcFace::versionI, m_rpcFace,
                                          std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(std::make_pair("peers",
            std::bind(&RpcFace::peersI, m_rpcFace, std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(
            std::make_pair("groupPeers", std::bind(&RpcFace::groupPeersI, m_rpcFace,
                                             std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(
            std::make_pair("groupList", std::bind(&RpcFace::groupListI, m_rpcFace,
                                            std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(
            std::make_pair("getBlockByHash", std::bind(&RpcFace::getBlockByHashI, m_rpcFace,
                                                 std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(
            std::make_pair("getBlockByNumber", std::bind(&RpcFace::getBlockByNumberI, m_rpcFace,
                                                   std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(
            std::make_pair("numberHash", std::bind(&RpcFace::numberHashI, m_rpcFace,
                                             std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(std::make_pair(
            "getTransactionByHash", std::bind(&RpcFace::getTransactionByHashI, m_rpcFace,
                                        std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(std::make_pair("getTransactionByBlockHashAndIndex",
            std::bind(&RpcFace::getTransactionByBlockHashAndIndexI, m_rpcFace,
                std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(std::make_pair("getTransactionByBlockNumberAndIndex",
            std::bind(&RpcFace::getTransactionByBlockNumberAndIndexI, m_rpcFace,
                std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(std::make_pair(
            "getTransactionReceipt", std::bind(&RpcFace::getTransactionReceiptI, m_rpcFace,
                                         std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(std::make_pair(
            "pendingTransactions", std::bind(&RpcFace::pendingTransactionsI, m_rpcFace,
                                       std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(std::make_pair("call",
            std::bind(&RpcFace::callI, m_rpcFace, std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(std::make_pair(
            "sendRawTransaction", std::bind(&RpcFace::sendRawTransactionI, m_rpcFace,
                                      std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(
            std::make_pair("getCode", std::bind(&RpcFace::getCodeI, m_rpcFace,
                                          std::placeholders::_1, std::placeholders::_2)));
        m_mapRpc.insert(std::make_pair(
            "totalTransactionCount", std::bind(&RpcFace::totalTransactionCountI, m_rpcFace,
                                         std::placeholders::_1, std::placeholders::_2)));
    }

public:
    // Take ownership of the socket
    explicit session(tcp::socket socket) : m_ws(std::move(socket)), m_strand(m_ws.get_executor())
    {
        auto m_service = std::make_shared<MockService>();
        std::string configurationPath = "";
        KeyPair m_keyPair = KeyPair::create();
        auto m_ledgerManager = std::make_shared<LedgerManager>(m_service, m_keyPair);
        m_ledgerManager->initSingleLedger<FakeLedger>(1, "", configurationPath);

        m_rpcFace = std::make_shared<Rpc>(m_ledgerManager, m_service);

        setMapRpc();
    }

    // Start the asynchronous operation
    void run()
    {
        // Accept the websocket handshake
        m_ws.async_accept(boost::asio::bind_executor(
            m_strand, std::bind(&session::on_accept, shared_from_this(), std::placeholders::_1)));
    }

    void on_accept(boost::system::error_code ec)
    {
        if (ec)
            return fail(ec, "accept");

        // Read a message
        do_read();
    }

    void do_read()
    {
        // Read a message into our buffer
        m_ws.async_read(m_buffer, boost::asio::bind_executor(
                                      m_strand, std::bind(&session::on_read, shared_from_this(),
                                                    std::placeholders::_1, std::placeholders::_2)));
    }

    void on_read(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if (ec == websocket::error::closed)
            return;

        if (ec)
            fail(ec, "read");

        LOG(INFO) << boost::beast::buffers(m_buffer.data()) << std::endl;

        Json::Value resultJson;
        resultJson["jsonrpc"] = "2.0";
        Json::FastWriter fastWriter;
        try
        {
            // 1. parse request message
            std::stringstream ss;
            ss << boost::beast::buffers_to_string(m_buffer.data());
            Json::Value requestJson;
            ss >> requestJson;

            std::string jsonrpc = requestJson["jsonrpc"].asString();
            if (jsonrpc != "2.0")
                BOOST_THROW_EXCEPTION(JsonRpcException(-32600));

            if (!requestJson.isMember("id"))
                BOOST_THROW_EXCEPTION(JsonRpcException(-32604));
            resultJson["id"] = requestJson["id"];

            std::string method = requestJson["method"].asString();
            if (!m_mapRpc.count(method))
                BOOST_THROW_EXCEPTION(JsonRpcException(-32601));

            Json::Value request;
            if (requestJson["params"].size() < 1 && method != "peers" && method != "version")
                BOOST_THROW_EXCEPTION(JsonRpcException(-32602));
            for (auto param : requestJson["params"])
                request.append(param);

            // 2. call RPC
            Json::StreamWriterBuilder builder;
            Json::Value result;

            auto callRpc = m_mapRpc[method];
            callRpc(request, result);
            resultJson["result"] = Json::writeString(builder, result);
        }
        catch (JsonRpcException& e)
        {
            switch (e.GetCode())
            {
            case -32600:
                resultJson["error"]["code"] = -32600;
                resultJson["error"]["message"] =
                    "INVALID_JSON_REQUEST: The JSON sent is not a valid JSON-RPC Request object";
                break;
            case -32601:
                resultJson["error"]["code"] = -32601;
                resultJson["error"]["message"] =
                    "METHOD_NOT_FOUND: The method being requested is not available on this server";
                break;
            case -32602:
                resultJson["error"]["code"] = -32602;
                resultJson["error"]["message"] =
                    "INVALID_PARAMS: Invalid method parameters (invalid name and/or type) "
                    "recognised";
                break;
            case -32604:
                resultJson["error"]["code"] = -32604;
                resultJson["error"]["message"] =
                    "PROCEDURE_IS_METHOD: The requested notification is declared as a method";
                break;
            default:
                resultJson["error"]["code"] = e.GetCode();
                resultJson["error"]["message"] = e.GetMessage();
            }
        }
        catch (Json::Exception& e)
        {
            resultJson["id"] = Json::nullValue;
            resultJson["error"]["code"] = -32700;
            resultJson["error"]["message"] = "JSON_PARSE_ERROR: The JSON-Object is not JSON-Valid";
        }
        catch (std::exception& e)
        {
            resultJson["error"]["code"] = -32603;
            resultJson["error"]["message"] = boost::diagnostic_information(e);
        }
        const std::string responseJson = fastWriter.write(resultJson);

        // 3.send response message
        m_ws.text(m_ws.got_text());
        m_ws.async_write(boost::asio::buffer(responseJson),
            boost::asio::bind_executor(
                m_strand, std::bind(&session::on_write, shared_from_this(), std::placeholders::_1,
                              std::placeholders::_2)));
    }

    void on_write(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        // Clear the buffer
        m_buffer.consume(m_buffer.size());

        // Do another read
        do_read();
    }
};

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    tcp::acceptor m_acceptor;
    tcp::socket m_socket;

public:
    listener(boost::asio::io_context& ioc, tcp::endpoint endpoint) : m_acceptor(ioc), m_socket(ioc)
    {
        boost::system::error_code ec;

        // Open the acceptor
        m_acceptor.open(endpoint.protocol(), ec);
        if (ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        m_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        m_acceptor.bind(endpoint, ec);
        if (ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        m_acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void run()
    {
        LOG(INFO) << "WebSocket Server Started! " << std::endl;
        if (!m_acceptor.is_open())
            return;
        do_accept();
    }

    void do_accept()
    {
        m_acceptor.async_accept(
            m_socket, std::bind(&listener::on_accept, shared_from_this(), std::placeholders::_1));
    }

    void on_accept(boost::system::error_code ec)
    {
        if (ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(std::move(m_socket))->run();
        }

        // Accept another connection
        do_accept();
    }
};
