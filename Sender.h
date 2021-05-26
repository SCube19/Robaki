#ifndef SENDER_H
#define SENDER_H
#include "Helper.h"
#include "err.h"

class Sender
{
    int sock;
    public:
    Sender(int sockArg);
    void send(const MySockaddr& addr, const std::vector<Event>& events, uint32_t from, uint32_t game_id);
    void sendLastToAll(const std::map<MySockaddr, client_ptr>& clients, const std::vector<Event>& events, uint32_t game_id);
};

#endif // !SENDER_H
