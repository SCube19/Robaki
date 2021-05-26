#ifndef ENEVT_HANDLER_H
#define EVENT_HANDLER_H

#include <algorithm>

#include "Helper.h"
#include "Client.h"
#include "Poll.h"
#include "Sender.h"

//constants for default values
#define DEFAULT_SEED time(NULL)
constexpr int DEFAULT_PORT = 2021;
constexpr int DEFAULT_TURNING_SPEED = 6;
constexpr int DEFAULT_ROUNDS_PER_SECOND = 50;
constexpr int DEFAULT_WIDTH = 640;
constexpr int DEFAULT_HEIGHT = 480;

//subclass for deterministic random number generation
class Rng
{
    uint64_t seed;

public:
    Rng(unsigned seedArg) : seed(seedArg) {}

    uint32_t rand();
};

//option structure for argument line inputs
struct options
{
    unsigned seed = DEFAULT_SEED;
    unsigned port = DEFAULT_PORT;
    unsigned turningSpeed = DEFAULT_TURNING_SPEED;
    unsigned roundsPerSecond = DEFAULT_ROUNDS_PER_SECOND;
    unsigned width = DEFAULT_WIDTH;
    unsigned height = DEFAULT_HEIGHT;
};

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
    bool started;
    int aliveClients;

    //<client addr, Client object> map
    client_map clients;
    //vector of Client objects for alphabetic ticks and events
    client_vector alphaClients;

    std::vector<Event> events;
    std::vector<std::vector<bool>> pixels;

    //sender object to handle response sending to clients
    Sender sender;

    //checks whether given client_coord is OOB
    bool outOfBounds(const client_coord &coord) const;
    //finds client_ptr in alphaClients based solely on its pointer
    client_vector::iterator findByPtr(const client_ptr &ptr);
    //makes new client inside object structures based on PollOutput
    void makeClient(const PollOutput &out);
    //makes event_no and event_type bytes
    std::pair<uint32_t, uint8_t> makeOuterEvent(uint8_t type) const;

    //handles GameOver event and sends it to all clients
    void gameOver();
    //handles NewGame event and sends it to all clients
    void newGame();
    //handles Pixel event and sends it to all clients
    void pixel(const coord_int &coord, int playerId);
    //handles PlayerEliminated event and sends it to all clients
    void playerEliminated(const client_ptr &client, int playerId);

public:
    EventHandler(options opt, const Sender &senderArg);

    //makes according response ie. changes internal game state and sends response
    //based on client's message (PollOutput out)
    void manageResponse(const PollOutput &out);
    //disconnects all clients specified in vector
    void disconnectClients(const std::vector<MySockaddr> toDisconnect);

    //starts new game (starting new game during another one is UB)
    void startGame();
    //makes one frame of game state calculation
    void tick();

    //checks wheter all players are ready to play
    bool clientsReady() const;
    //returns whether game has started
    bool gameStarted() const;
};

#endif // !EVENT_HANDLER_H
