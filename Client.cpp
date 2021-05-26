#include "Client.h"

Client::Client(int sessionArg, TurningDirection turnArg, const std::string &nameArg)
    : session_id(sessionArg), lastTurningDirection(turnArg), disconnected(false), observer(nameArg.size() == 0), name(nameArg), alive(true), sessionDiff(false), wantsToStart(false)
{
}

Client::Client(Client *client) : Client(*client)
{
}

Client::Client(const ClientMessage &msg) : disconnected(false), observer(std::string(msg.player_name).size() == 0), alive(true), sessionDiff(false), wantsToStart(false)
{
    changeClient(msg);
    name = msg.player_name;
}

uint64_t Client::getSessionId() const
{
    return session_id;
}

TurningDirection Client::getTurningDirection() const
{
    return lastTurningDirection;
}

int Client::getAngle() const
{
    return turningAngle;
}

std::string Client::getName() const
{
    return name;
}

bool Client::isObserver() const
{
    return observer;
}

bool Client::isDisconnected() const
{
    return disconnected;
}

bool Client::isAlive() const
{
    return alive;
}

bool Client::isSessionIdHigher() const
{
    return sessionDiff;
}

coord_int Client::getPixelCoord() const
{
    return coord_int((uint32_t)coords.first, (uint32_t)coords.second);
}

void Client::observe(bool x)
{
    observer = x;
}

void Client::disconnect()
{
    disconnected = true;
}

void Client::sessionDiffDetected(bool x)
{
    sessionDiff = x;
}

void Client::setTurningDirection(TurningDirection direction)
{
    lastTurningDirection = direction;
}

void Client::setAngle(int angle)
{
    turningAngle = angle;
}

void Client::addAngle(int angle)
{
    turningAngle = (turningAngle + angle + 360) % 360;
}

void Client::setSessionId(uint64_t sessionArg)
{
    session_id = sessionArg;
}

void Client::setName(std::string nameArg)
{
    name = nameArg;
}

void Client::setCoords(const client_coord &c)
{
    coords = c;
}

void Client::changeClient(const ClientMessage &msg)
{
    setTurningDirection((TurningDirection)msg.turn_direction);
    setSessionId(msg.session_id);
}

static long double degrees(long double degree)
{
    return (degree / 180 * M_PI);
}
bool Client::move()
{
    // std::cout << getName() << '\n';
    // std::cout << "WITH ANGLE: " << getAngle() << "\n"
    //           << coords.first << " " << coords.second << '\n';
    client_coord tmp(coords.first + cosl(degrees(turningAngle)), coords.second + sinl(degrees(turningAngle)));
    bool rVal = (coord_int((uint32_t)tmp.first, (uint32_t)tmp.second) != coord_int((uint32_t)coords.first, (uint32_t)coords.second));
    coords = tmp;
    // std::cout << coords.first << " " << coords.second << "\n\n";
    return rVal;
}

void Client::die(bool x)
{
    alive = !x;
}

bool Client::willingToStart() const
{
    return wantsToStart;
}

void Client::will(bool x)
{
    wantsToStart = x;
}