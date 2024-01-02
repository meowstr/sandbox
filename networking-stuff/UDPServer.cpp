#include "UDPServer.hpp"
#include "Globals.hpp"
#include <memory>

UDPServer::UDPServer(Game & game)
    : game_(game)
{
    using namespace std::placeholders;

    udpHandler_.channelManager_.RegisterChannel<Packets::ConnectRequest>(
        ServerChannels::CONNECT,
        std::bind(&UDPServer::OnConnect, this, _1, _2));

    udpHandler_.channelManager_.RegisterChannel<Packets::Input>(
        ServerChannels::INPUT, std::bind(&UDPServer::OnInput, this, _1, _2));
}

void UDPServer::Start()
{
    udpHandler_.Start(46464);
    LOG(Loggers::DEBUG, "Started server on port 46464.");
};

void UDPServer::Run()
{
    udpHandler_.Run();
}

void UDPServer::Tick(long tick, float timestep)
{
    // tick local player
    PlayerInput input;
    PlayerData  data;
    game_.pollPlayer(input);
    game_.localPlayer->networkTick(input, timestep, data);

    // tick client players
    for (auto & client : clients_)
    {
        PlayerData data;
        // consume next input from clients
        if (client->hasInput())
        {
            client->player.networkTick(
                client->nextInput().input, timestep, data);
            client->popInput();
        }
    }

    // TODO: spread snapshots througout all ticks
    if (tick % Globals::ticksPerSnapshot == 0)
        Snapshot();

    // then do game logic
    game_.tick(timestep);
}

void UDPServer::HandleInput(ClientConnection *          client,
                            const PlayerInputSnapshot & input)
{
    // TODO: could be optimized (since slots are contiguous)
    for (int i = 0; i < Globals::ticksPerSnapshot; i++)
    {
        client->pushInput(
            {.tick = input.slots[i].tick, .input = input.slots[i].input});
    }
}

void UDPServer::Snapshot()
{
    for (auto & client : clients_)
    {
        // send snapshot with the client tick on them
    }
}

auto FindClientByRemote(std::list<ClientConnection::Ptr> & clients,
                        Remote &                           remote)
{
    return std::find_if(
        clients.begin(), clients.end(), [&](ClientConnection::Ptr & x) {
            return x->remote == remote;
        });
}

void UDPServer::OnConnect(Remote & remote, Packets::ConnectRequest request)
{
    auto connection = FindClientByRemote(clients_, remote);

    if (connection == clients_.end())
    {
        LOG(Loggers::DEBUG, "Player just connected: " << request.name_ << ".");

        // make a new player
        Player::Ptr player = std::make_unique<Player>();
        auto        newConnection =
            std::make_unique<ClientConnection>(std::move(remote), *player);
        game_.addPlayer(std::move(player));

        Packets::ConnectResponse response;
        udpHandler_.Send(response, newConnection->remote, ClientChannels::CONNECT);

        clients_.push_back(std::move(newConnection));
    }
    else
    {
        LOG(Loggers::ERROR,
            "Player requesting to connect again: " << request.name_ << ".");
    }
}

void UDPServer::OnInput(Remote & remote, Packets::Input input)
{
    auto connection = FindClientByRemote(clients_, remote);

    if (connection == clients_.end())
    {
        LOG(Loggers::ERROR, "Got input from unknown client.");
    }
    else
    {
        LOG(Loggers::INFO, "Just got some player input.");
    }
}
