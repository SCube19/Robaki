#ifndef ENEVT_HANDLER_H
#define EVENT_HANDLER_H

#include "Helper.h"
#include "Client.h"
#include "Poll.h"
#include "Sender.h"
#include <algorithm>

#define DEFAULT_SEED time(NULL)
constexpr int DEFAULT_PORT = 2021;
constexpr int DEFAULT_TURNING_SPEED = 6;
constexpr int DEFAULT_ROUNDS_PER_SECOND = 50;
constexpr int DEFAULT_WIDTH = 640;
constexpr int DEFAULT_HEIGHT = 480;

struct options
{
    unsigned seed = DEFAULT_SEED;
    unsigned port = DEFAULT_PORT;
    unsigned turningSpeed = DEFAULT_TURNING_SPEED;
    unsigned roundsPerSecond = DEFAULT_ROUNDS_PER_SECOND;
    unsigned width = DEFAULT_WIDTH;
    unsigned height = DEFAULT_HEIGHT;
};

struct Rng
{
    uint64_t seed;
    Rng(unsigned seedArg) : seed(seedArg) {}

    uint32_t rand()
    {
        uint32_t ret = seed;
        seed = (seed * 279410273) % 4294967291;
        return ret;
    }
};

//using client_ptr = std::shared_ptr<Client>;

class EventHandler
{
public:
    using client_map = std::map<MySockaddr, client_ptr>;
    using client_vector = std::vector<client_ptr>;

private:
    Rng rng;
    unsigned width;
    unsigned height;
    unsigned turningSpeed;
    uint32_t game_id;
    std::vector<Event> events;
    std::vector<std::vector<bool>> pixels;
    client_map clients;
    client_vector alphaClients;

    bool started;
    int aliveClients;

    Sender sender;

    bool outOfBounds(const client_coord &coord);
    void gameOver();
    void newGame();
    void pixel(const coord_int &coord, int playerId);
    void playerEliminated(const client_ptr &client, int playerId);

    client_vector::iterator findByPtr(const client_ptr &ptr);

    void makeClient(const PollOutput &out);
    std::pair<uint32_t, uint8_t> makeOuterEvent(uint8_t type);
    uint32_t crc(uint8_t event_type, const std::vector<char> &event_data);

public:
    EventHandler(options opt, const Sender &senderArg);
    void manageResponse(const PollOutput &out);
    void disconnectClients(const std::vector<MySockaddr> toDisconnect);
    bool gameStarted();

    bool clientsReady();

    void tick();
    void startGame();
};

#endif // !ENEVT_HANDLER_H
