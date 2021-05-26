#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <string>
#include <math.h>
#include <iostream>

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
    TurningDirection lastTurningDirection;
    int turningAngle;
    bool disconnected;
    bool observer;
    std::string name;

    client_coord coords;
    bool alive;
    bool sessionDiff;
    bool wantsToStart;

public:
    Client() = default;
    Client(const Client &client) = default;
    Client(Client *client);
    Client(int sessionArg, TurningDirection turnArg, const std::string &nameArg);
    Client(const ClientMessage &msg);

    uint64_t getSessionId() const;
    TurningDirection getTurningDirection() const;
    int getAngle() const;
    std::string getName() const;
    bool isObserver() const;
    bool isDisconnected() const;
    bool isAlive() const;
    bool isSessionIdHigher() const;
    bool willingToStart() const;
    coord_int getPixelCoord() const;

    void disconnect();
    void observe(bool x);
    void sessionDiffDetected(bool x);
    void die(bool x);
    void will(bool x);
    void setTurningDirection(TurningDirection direction);
    void setAngle(int angle);
    void addAngle(int angle);
    void setSessionId(uint64_t sessionArg);
    void setName(std::string nameArg);
    void setCoords(const client_coord &c);
    void changeClient(const ClientMessage &msg);

    bool move();

    bool operator==(Client const &o) const { return this->name.compare(o.name) == 0; }
    bool operator<(Client const &o) const { return (this->observer < o.observer || (this->observer == o.observer && this->name.compare(o.name) < 0));}
};

#endif // !CLIENT_H
