#include "EventHandler.h"

////////////////////////////////////RAND////////////////////////////////////////
uint32_t Rng::rand()
{
    uint32_t ret = seed;
    seed = (seed * 279410273) % 4294967291;
    return ret;
}
///////

EventHandler::EventHandler(options opt, const Sender &senderArg)
    : rng(opt.seed), width(opt.width), height(opt.height), turningSpeed(opt.turningSpeed), started(false), aliveClients(0), sender(senderArg)
{
    pixels.resize(opt.height);
    for (size_t i = 0; i < pixels.size(); i++)
        pixels[i].resize(opt.width);
}

//====================================== PRIVATE METHODS ========================================================//
///////////////////////////////////////OUT OF BOUNDS//////////////////////////////////////////////////////////////
bool EventHandler::outOfBounds(const client_coord &coord) const
{
    return coord.first < 0 || coord.second < 0 || coord.first >= width || coord.second >= height;
}

////////////////////////////////////////FIND BY PTR//////////////////////////////////////////////////////////////
EventHandler::client_vector::iterator EventHandler::findByPtr(const client_ptr &ptr)
{
    for (auto x : alphaClients)
        if (x.ptrCompare(ptr))
            return alphaClients.begin() + std::distance(alphaClients.data(), &x);
    return alphaClients.end();
}

///////////////////////////////////////MAKE CLIENT///////////////////////////////////////////////////////////////
void EventHandler::makeClient(const PollOutput &out)
{   
    //make client_ptr and try to find someone with same name
    client_ptr tmp(std::make_shared<Client>(new Client(out.message)));
    if (std::find(alphaClients.begin(), alphaClients.end(), tmp) != alphaClients.end())
        return;

    //insert new client
    clients.insert(std::pair<MySockaddr, client_ptr>(out.addr, tmp));
    alphaClients.emplace_back(tmp);

    //set will to play to true
    if (out.message.turn_direction != NONE)
        tmp->will(true);

    //if game started he's an observer, if game hasnt started there's nothing to send
    if (started)
    {
        tmp->observe(true);
        sender.send(out.addr, events, 0, game_id);
    }
    //operators < gives order (alphabethical non-observers, alphabetical observers)
    std::sort(alphaClients.begin(), alphaClients.end());
}

///////////////////////////////////////MAKE OUTER EVENT/////////////////////////////////////////////////////////
std::pair<uint32_t, uint8_t> EventHandler::makeOuterEvent(uint8_t type) const
{
    return std::pair<uint32_t, uint8_t>(events.size(), type);
}

//******************************************EVENT METHODS****************************************************//
//-----------------------------NEW GAME------------------------------------------//
void EventHandler::newGame()
{
    //making new event
    std::vector<char> event_data;
    event_data.resize(8);
    uint32_t maxx = htobe32(width);
    uint32_t maxy = htobe32(height);

    memcpy(&event_data[0], (char *)&maxx, 4);
    memcpy(&event_data[4], (char *)&maxy, 4);

    size_t filled = 8;
    for (auto client : alphaClients)
    {
        if (client->getName() == "")
            continue;
        event_data.resize(event_data.size() + client->getName().size() + 1);
        memcpy(&event_data[filled], client->getName().c_str(), client->getName().size() + 1);
        filled += client->getName().size() + 1;
    }

    auto outerData = makeOuterEvent(0);
    events.emplace_back(Event(5 + event_data.size(), outerData.first, outerData.second, event_data));

    //sending it to all
    sender.sendLastToAll(clients, events, game_id);
}

//-----------------------------PIXEL------------------------------------------//
void EventHandler::pixel(const coord_int &coord, int playerId)
{
    //event making
    std::vector<char> event_data;
    event_data.resize(9);
    uint32_t x = htobe32(coord.first);
    uint32_t y = htobe32(coord.second);

    event_data[0] = (uint8_t)playerId;
    memcpy(&event_data[1], (char *)&(x), 4);
    memcpy(&event_data[5], (char *)&(y), 4);

    auto outerData = makeOuterEvent(1);
    events.emplace_back(Event(5 + event_data.size(), outerData.first, outerData.second, event_data));

    //state change
    pixels[coord.second][coord.first] = false;

    //sending
    sender.sendLastToAll(clients, events, game_id);
}

//-----------------------------PLAYER ELIMINATED------------------------------------------//
void EventHandler::playerEliminated(const client_ptr &client, int playerId)
{   
    //event making
    std::vector<char> event_data;
    event_data.emplace_back((uint8_t)playerId);
    auto outerData = makeOuterEvent(2);
    events.emplace_back(Event(5 + event_data.size(), outerData.first, outerData.second, event_data));

    //state change
    client->die(true);
    aliveClients--;

    //sending
    sender.sendLastToAll(clients, events, game_id);
}

//-----------------------------GAME OVER------------------------------------------//
void EventHandler::gameOver()
{   
    //making event and sending
    std::vector<char> event_data;
    auto outerData = makeOuterEvent(3);
    events.emplace_back(Event(5 + event_data.size(), outerData.first, outerData.second, event_data));
    sender.sendLastToAll(clients, events, game_id);

    //resetting EventHandler states
    started = false;
    events.clear();

    client_map newMap;
    client_vector newVec;

    for (auto client : clients)
    {
        if (!client.second->isDisconnected())
        {
            client.second->sessionDiffDetected(false);
            if (client.second->getName() != "")
                client.second->observe(false);
            client.second->die(false);
            client.second->setTurningDirection(NONE);
            client.second->will(false);
            newMap.insert(client);
            newVec.emplace_back(client.second);
        }
    }

    clients = newMap;
    alphaClients = newVec;
    std::sort(alphaClients.begin(), alphaClients.end());
}

//===================================================PUBLIC METHODS======================================================//
///////////////////////////////////////////////MANAGE RESPONSE/////////////////////////////////////////////////////////////
void EventHandler::manageResponse(const PollOutput &out)
{
    auto it = clients.find(out.addr);

    if (it == clients.end())
        makeClient(out);
    //normal response 
    else if (out.message.session_id == it->second->getSessionId() && !it->second->isDisconnected() && !it->second->isSessionIdHigher())
    {
        it->second->changeClient(out.message);
        if (out.message.turn_direction != NONE)
            it->second->will(true);
        sender.send(it->first, events, out.message.next_expected_event_no, game_id);
    }
    //session id was higher
    else if (out.message.session_id > it->second->getSessionId() && !it->second->isDisconnected())
    {
        if (started)
        {
            it->second->sessionDiffDetected(true);
            sender.send(it->first, events, out.message.next_expected_event_no, game_id);
        }
        else
        {
            alphaClients.erase(findByPtr(it->second));
            clients.erase(it);
            makeClient(out);
        }
    }
}

////////////////////////////////////////////////DISCONNECT CLIENTS//////////////////////////////////////////////////////
void EventHandler::disconnectClients(const std::vector<MySockaddr> toDisconnect)
{
    for (auto addr : toDisconnect)
    {
        auto it = clients.find(addr);
        if (it != clients.end() && started)
            it->second->disconnect();
        else if (it != clients.end())
        {
            alphaClients.erase(findByPtr(it->second.ptr));
            clients.erase(it);
        }
    }
}

///////////////////////////////////////////////START GAME//////////////////////////////////////////////////////////////
void EventHandler::startGame()
{
    for (size_t i = 0; i < pixels.size(); i++)
        std::fill(pixels[i].begin(), pixels[i].end(), true);

    game_id = rng.rand();
    newGame();

    aliveClients = 0;
    int i = 0;
    for (auto client : alphaClients)
    {
        if (client->isObserver())
            continue;

        aliveClients++;
        double x = (rng.rand() % width) + 0.5f;
        double y = (rng.rand() % height) + 0.5f;
        client->setCoords(client_coord(x, y));
        client->setAngle(rng.rand() % 360);

        coord_int clientCoord = client->getPixelCoord();

        if (!pixels[clientCoord.second][clientCoord.first])
            playerEliminated(client, i);
        else
            pixel(clientCoord, i);
        i++;
    }
    started = true;
}

/////////////////////////////////////////////////////////TICK///////////////////////////////////////////////////////
void EventHandler::tick()
{
    int i = 0;
    for (auto client : alphaClients)
    {
        //skip obserevers and dead clients
        if (!client->isAlive() || client->isObserver())
        {
            i++;
            continue;
        }

        //turning
        if (client->getTurningDirection() == RIGHT)
            client->addAngle(turningSpeed);
        else if (client->getTurningDirection() == LEFT)
            client->addAngle(-turningSpeed);
        
        //if integer position didnt change skip
        if (!client->move())
        {
            i++;
            continue;
        }

        coord_int clientCoord = client->getPixelCoord();
        if (outOfBounds(clientCoord) || !pixels[clientCoord.second][clientCoord.first])
            playerEliminated(client, i);
        else
            pixel(clientCoord, i);

        if (aliveClients <= 1)
        {
            gameOver();
            return;
        }

        i++;
    }
}

////////////////////////////////////////CLIENTS READY////////////////////////////////////////////////////////
bool EventHandler::clientsReady() const
{
    if (clients.size() < 2)
        return false;
        
    int nonObservers = 0;
    for (auto client : alphaClients)
    {
        if (!client->isObserver() && !client->willingToStart())
            return false;
        if (!client->isObserver() && client->willingToStart())
            nonObservers++;
    }
    return nonObservers >= 2;
}

///////////////////////////////////////GAME STARTED//////////////////////////////////////////////////////////
bool EventHandler::gameStarted() const
{
    return started;
}
