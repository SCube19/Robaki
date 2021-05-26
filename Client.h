#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <math.h>

struct __attribute__((packed)) ClientMessage
{
    uint64_t session_id;
    uint8_t turn_direction;
    uint32_t next_expected_event_no;
    char player_name[21];
};

enum TurningDirection
{
    NONE,
    RIGHT,
    LEFT
};

using client_coord = std::pair<long double, long double>;
using coord_int = std::pair<uint32_t, uint32_t>;

class Client
{
private:
    uint64_t session_id;
    //last received turn_direction
    TurningDirection lastTurningDirection;
    //client flag due to -Wreorder
    bool disconnected;
    bool observer;
    std::string name;

    int turningAngle;
    client_coord coords;

    //client flags
    bool alive;
    bool sessionDiff;
    bool wantsToStart;

public:
    Client() = default;
    Client(const Client &client) = default;
    Client(Client *client);
    Client(int sessionArg, TurningDirection turnArg, const std::string &nameArg);
    Client(const ClientMessage &msg);

    //getters
    uint64_t getSessionId() const;
    TurningDirection getTurningDirection() const;
    int getAngle() const;
    std::string getName() const;
    coord_int getPixelCoord() const;
    bool isObserver() const;
    bool isDisconnected() const;
    bool isAlive() const;
    bool isSessionIdHigher() const;
    bool willingToStart() const;

    //setters
    void disconnect();
    void observe(bool x);
    void sessionDiffDetected(bool x);
    void die(bool x);
    void will(bool x);
    void setTurningDirection(TurningDirection direction);
    void setAngle(int angle);
    void setSessionId(uint64_t sessionArg);
    void setName(std::string nameArg);
    void setCoords(const client_coord &c);

    void addAngle(int angle);
    void changeClient(const ClientMessage &msg);

    //moves client and return whether his integer coords have changed
    bool move();

    //comparator operators
    bool operator==(Client const &o) const { return this->name.compare(o.name) == 0; }
    bool operator<(Client const &o) const { return (this->observer < o.observer || (this->observer == o.observer && this->name.compare(o.name) < 0)); }
};

#endif // !CLIENT_H
