#include "Sender.h"

Sender::Sender(int sockArg) : sock(sockArg)
{
}

void Sender::send(const MySockaddr &addr, const std::vector<Event> &events, uint32_t from, uint32_t game_id)
{
    uint32_t gid = htobe32(game_id);
    sockaddr_in6 *actualAddr = (sockaddr_in6 *)&addr;

    while (from < events.size())
    {
        size_t toWrite = 4;
        char bytes[550];
        memcpy(bytes, (char *)&gid, 4);
        while (toWrite < 550 && from < events.size() && toWrite + events[from].getBytes().size() <= 550)
        {
            const std::vector<char> eventBytes = events[from].getBytes();
            memcpy(bytes + toWrite, &eventBytes[0], eventBytes.size());
            toWrite += eventBytes.size();
            from++;
        }
        if (sendto(sock, bytes, toWrite, 0, (sockaddr *)actualAddr, addr.addrlen) < 0)
        {
            if(errno != EWOULDBLOCK)
                syserr("sendto");
        }
    }
}

void Sender::sendLastToAll(const std::map<MySockaddr, client_ptr> &clients, const std::vector<Event> &events, uint32_t game_id)
{
    for (auto client : clients)
        if (!client.second->isDisconnected())
            send(client.first, events, events.size() - 1, game_id);
}