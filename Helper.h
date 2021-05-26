#ifndef HELPER_H
#define HELPER_H

#include <sys/socket.h>
#include <netdb.h>
#include <vector>
#include <endian.h>
#include <memory>
#include <string.h>
#include <string.h>
#include "Client.h"

class Event
{
    std::vector<char> bytes;
    //calculate crc32 for event bytes
    uint32_t crc(size_t size);

public:
    Event(uint32_t len, uint32_t event_no, uint8_t event_type, const std::vector<char> &event_data);
    std::vector<char> getBytes() const;
};

struct MySockaddr : sockaddr_in6
{
    socklen_t addrlen;

    int cmpAddr(const MySockaddr &o) const;
    int cmpIpv6(const MySockaddr &o) const;

    MySockaddr() = default;
    MySockaddr(const MySockaddr &addr);
    MySockaddr(const sockaddr_in6 &addr, socklen_t len);
    MySockaddr &operator=(const MySockaddr &o);
    bool operator==(MySockaddr const &o) const { return cmpAddr(o) == 0; }
    bool operator<(MySockaddr const &o) const { return cmpAddr(o) == -1; }
};

struct client_ptr
{
    std::shared_ptr<Client> ptr;

    client_ptr(const std::shared_ptr<Client> &ptrArg) : ptr(ptrArg) {}
    Client *operator->() const { return &*ptr; }
    bool operator==(client_ptr const &o) const { return *ptr == *o.ptr; }
    bool operator<(client_ptr const &o) const { return *ptr < *o.ptr; }
    bool ptrCompare(client_ptr const &o) const { return ptr == o.ptr; }
};

#endif // !HELPER_H
