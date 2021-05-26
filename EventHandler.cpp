#include "EventHandler.h"

EventHandler::EventHandler(options opt, const Sender& senderArg) 
: rng(opt.seed), width(opt.width), height(opt.height), turningSpeed(opt.turningSpeed), started(false), aliveClients(0), sender(senderArg)
{
    pixels.resize(opt.height);
    for (size_t i = 0; i < pixels.size(); i++)
        pixels[i].resize(opt.width);
}

EventHandler::client_vector::iterator EventHandler::findByPtr(const client_ptr &ptr)
{
    for (auto x : alphaClients)
        if (x.ptrCompare(ptr))
            return alphaClients.begin() + std::distance(alphaClients.data(), &x);
    return alphaClients.end();
}

void EventHandler::makeClient(const PollOutput &out)
{
    std::cout << "NEW CLIENT: " << std::string(out.message.player_name) << " OLD SIZE: " << clients.size() << '\n';
    out.addr.print();
    
    client_ptr tmp(std::make_shared<Client>(new Client(out.message)));
    if (std::find(alphaClients.begin(), alphaClients.end(), tmp) != alphaClients.end())
    {
        std::cout << "FOUND SAME NAME\n";
        return;
    }

    clients.insert(std::pair<MySockaddr, client_ptr>(out.addr, tmp));
    alphaClients.emplace_back(tmp);
    
    if(out.message.turn_direction != NONE)
        tmp->will(true);

    if (started)
    {
        std::cout << "SENDING TO NEW OBSERVER\n";
        tmp->observe(true);
        sender.send(out.addr, events, 0, game_id);
    }
    std::sort(alphaClients.begin(), alphaClients.end());
}

void EventHandler::manageResponse(const PollOutput &out)
{
    auto it = clients.find(out.addr);

    if (it == clients.end())
        makeClient(out);
    else if (out.message.session_id == it->second->getSessionId() && !it->second->isDisconnected() && !it->second->isSessionIdHigher())
    {
        it->second->changeClient(out.message);
        if(out.message.turn_direction != NONE)
            it->second->will(true);
        sender.send(it->first, events, out.message.next_expected_event_no, game_id);
    }
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

void EventHandler::disconnectClients(const std::vector<MySockaddr> toDisconnect)
{
    for (auto addr : toDisconnect)
    {   
        auto it = clients.find(addr);
        if (it != clients.end() && started)
        {
            std::cout << "DISCONNECTION1\n";
            it->first.print();
            it->second->disconnect();
        }
        else if (it != clients.end())
        {   
            std::cout << "DISCONNECTION2 SIZE: " << alphaClients.size() << '\n';
            it->first.print();
            alphaClients.erase(findByPtr(it->second.ptr));
            clients.erase(it);  
            std::cout << "SIZE AFTER DC: " << alphaClients.size();       
        }
    }
}

void EventHandler::tick()
{   
    // std::cout << "GAME STARTED: " << started << '\n';
    // std::cout << "SIZE: " << alphaClients.size() << '\n';
    int i = 0;
    for (auto client : alphaClients)
    {
        // std::cout << "CLIENT " << client->getName() << " ALIVE: " << client->isAlive() << " OBSERVER: " << client->isObserver() << " WANTS TO PLAY: " 
        // << client->willingToStart() << " HIGHER SESSION ID: " << client->isSessionIdHigher() << "\n";
        if (!client->isAlive() || client->isObserver())
        {
            i++;
            continue;
        }
        //std::cout << "TU3\n";
        if (client->getTurningDirection() == RIGHT)
            client->addAngle(turningSpeed);
        else if (client->getTurningDirection() == LEFT)
            client->addAngle(-turningSpeed);

        if (!client->move())
        {
            i++;
            continue;
        }
        //std::cout << "TU\n";
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

bool EventHandler::clientsReady()
{
    //std::cout << "CHECKING READINESS\n";
    if (clients.size() < 2)
        return false;
    int nonObservers = 0;
    for (auto client : alphaClients)
    {
        std::cout << "Will: " << client->willingToStart() << " observer: " << " " << client->isObserver() << '\n';
        if (!client->isObserver() && !client->willingToStart())
            return false;
        if (!client->isObserver() && client->willingToStart())
            nonObservers++;
    }
    return nonObservers >= 2;
}

bool EventHandler::gameStarted()
{
    return started;
}

void EventHandler::startGame()
{
    std::cout << "STARTING GAME\n";
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

std::pair<uint32_t, uint8_t> EventHandler::makeOuterEvent(uint8_t type)
{
    return std::pair<uint32_t, uint8_t>(events.size(), type);
}

void EventHandler::newGame()
{
    std::vector<char> event_data;
    event_data.resize(8);
    uint32_t maxx = htobe32(width);
    uint32_t maxy = htobe32(height);
    memcpy(&event_data[0], (char*)&maxx, 4);
    memcpy(&event_data[4], (char*)&maxy, 4);
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

    sender.sendLastToAll(clients, events, game_id);
}

void EventHandler::pixel(const coord_int &coord, int playerId)
{
    std::vector<char> event_data;
    event_data.resize(9);
    uint32_t x = htobe32(coord.first);
    uint32_t y = htobe32(coord.second);

    event_data[0] = (uint8_t)playerId;
    memcpy(&event_data[1], (char*)&(x), 4);
    memcpy(&event_data[5], (char*)&(y), 4);

    auto outerData = makeOuterEvent(1);
    events.emplace_back(Event(5 + event_data.size(), outerData.first, outerData.second, event_data));

    pixels[coord.second][coord.first] = false;

    sender.sendLastToAll(clients, events, game_id);
}

void EventHandler::playerEliminated(const client_ptr &client, int playerId)
{
    std::vector<char> event_data;
    event_data.emplace_back((uint8_t)playerId);
    auto outerData = makeOuterEvent(2);
    events.emplace_back(Event(5 + event_data.size(), outerData.first, outerData.second, event_data));

    client->die(true);
    aliveClients--;

    sender.sendLastToAll(clients, events, game_id);
}

void EventHandler::gameOver()
{
    std::vector<char> event_data;
    auto outerData = makeOuterEvent(3);
    events.emplace_back(Event(5 + event_data.size(), outerData.first, outerData.second, event_data));
    sender.sendLastToAll(clients, events, game_id);

    started = false;
    events.clear();
    
    client_map newMap;
    client_vector newVec;

    std::cout << "START::SIZE: " << clients.size() << '\n';
    for (auto client : clients)
    {
        std::cout << "CLIENT DCED: " << client.second->isDisconnected() << '\n';
        if(!client.second->isDisconnected())
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


bool EventHandler::outOfBounds(const client_coord &coord)
{
    return coord.first < 0 || coord.second < 0 || coord.first >= width || coord.second >= height;
}