#include "UDP.hpp"

#include <asio.hpp>
#include <memory>

using asio::ip::udp;

struct AsioRemote
{
    udp::endpoint e_;

    AsioRemote(udp::endpoint & e);
};

struct UDPAsio
{
    asio::io_context io_;
    udp::socket      socket_;
    udp::endpoint    remoteEndpoint_;

    UDPAsio()
        : socket_(io_)
    {
    }
};

AsioRemote::AsioRemote(udp::endpoint & e)
    : e_(e)
{
}

Remote::Remote()
    : asioRemote_(nullptr)
{
}

Remote::Remote(std::unique_ptr<AsioRemote> && remote)
    : asioRemote_(std::move(remote))
{
}

bool operator==(Remote & remote1, Remote & remote2)
{
    return remote1.asioRemote_->e_ == remote2.asioRemote_->e_;
}

/// had to define these after AsioRemote is fully defined
Remote & Remote::operator=(Remote && other) = default;
Remote::Remote(Remote && remote)            = default;
Remote::~Remote()                           = default;

UDPHandler::UDPHandler()
    : asio_(std::make_unique<UDPAsio>())
{
}

void UDPHandler::Start(int port)
{
    asio_->socket_.open(udp::v4());
    asio_->socket_.bind(udp::endpoint(udp::v4(), port));

    StartReceive();
}

void UDPHandler::Run()
{
    asio_->io_.run();
}

Remote UDPHandler::Resolve(std::string hostname, std::string service)
{
    udp::resolver resolver(asio_->io_);

    LOG(Loggers::INFO,
        "Resolving server address: " << hostname << " (" << service << ")...");

    udp::endpoint endpoint =
        *resolver.resolve(udp::v4(), hostname, service).begin();

    return std::make_unique<AsioRemote>(endpoint);
}

void UDPHandler::OnReceive(std::size_t n)
{
    LOG(Loggers::INFO, "Received " << n << " bytes.");

    if (n >= sizeof(Packet))
    {
        Remote remote(std::make_unique<AsioRemote>(asio_->remoteEndpoint_));
        channelManager_.Dispatch(std::ref(remote), receivePacket_);
    }
    else
    {
        LOG(Loggers::DEBUG, "Rejected packet.");
    }

    StartReceive();
}

void UDPHandler::StartReceive()
{
    using namespace std::placeholders;

    std::uint8_t * packetData = reinterpret_cast<uint8_t *>(&receivePacket_);

    asio_->socket_.async_receive_from(
        asio::buffer(packetData, sizeof(Packet)),
        asio_->remoteEndpoint_,
        std::bind(&UDPHandler::OnReceive, this, _2));

    LOG(Loggers::INFO, "Listening for packet...");
}

void UDPHandler::OnSend(std::size_t bytesSent)
{
    LOG(Loggers::INFO, "Sent " << bytesSent << " bytes.");
}

void UDPHandler::SendPacket(Remote & remote)
{
    using namespace std::placeholders;

    std::uint8_t * packetData = reinterpret_cast<uint8_t *>(&sendPacket_);
    asio_->socket_.async_send_to(asio::buffer(packetData, sizeof(Packet)),
                                 remote.asioRemote_->e_,
                                 std::bind(&UDPHandler::OnSend, this, _2));
}

UDPHandler::~UDPHandler() = default;
