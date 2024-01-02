#include "UDPClient.hpp"

#include <iostream>

void UDPClient::tick(long tick, float timestep)
{
    int slot = tick % Globals::ticksPerSnapshot;

    // tick the player
    PlayerInput input;
    game.pollPlayer(input);

    PlayerData data;
    game.localPlayer->networkTick(input, timestep, data);

    PlayerPrediction prediction = {.tick = tick, .input = input, .data = data};
    inputSnapshot.slots[slot] = prediction;

    predictions.push(prediction);

    if (slot == Globals::ticksPerSnapshot - 1)
    {
        // send snapshot
        sendInput();
    }

    // tick the game
    game.tick(timestep);
}

void UDPClient::validatePlayer(long currentTick,
                               long responseTick,
                               float timestep,
                               const PlayerData & serverData)
{
    int ticksAgo = currentTick - responseTick;
    PlayerPrediction & prediction = predictions[ticksAgo];

    // check if tick even exists in our ring buffer (if not ignore validation)
    if (prediction.tick != responseTick)
        return;

    if (prediction.data != serverData)
    {
        // apply correction
        predictions.rewind(ticksAgo);
        predictions.push({.tick = responseTick,
                          .input = prediction.input,
                          .data = serverData});
        game.localPlayer->networkSet(serverData);

        for (int i = 0; i < ticksAgo; i++)
        {
            // get the old input used to compute (access target buffer cell by
            // indexing with -1)
            PlayerInput & input = predictions[-1].input;
            PlayerData data;
            // simulate
            game.localPlayer->networkTick(input, timestep, data);
            // and push (overwriting the target buffer cell with new snapshot)
            predictions.push(
                {.tick = responseTick + i + 1, .input = input, .data = data});

            // TODO: then begin interpolation for smoothness
        }
    }
}

void UDPClient::sendInput()
{
}
