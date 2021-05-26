#ifndef SENDER_H
#define SENDER_H

#include <map>
#include "Helper.h"
#include "err.h"

class Sender
{
    int sock;

public:
    Sender(int sockArg);

    //sends udp packets to addr containing events from "from" to events.size()
    void send(const MySockaddr &addr, const std::vector<Event> &events, uint32_t from, uint32_t game_id) const;
    //using send() function sends packets to all clients specified in clients map
    void sendLastToAll(const std::map<MySockaddr, client_ptr> &clients, const std::vector<Event> &events, uint32_t game_id) const;
};

#endif // !SENDER_H
