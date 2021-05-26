#include "Helper.h"
#include "gui/gui2/err.h"
#include <string.h>

MySockaddr::MySockaddr(const MySockaddr &addr)
{
    this->addrlen = addr.addrlen;
    memcpy(&this->sin6_addr, &addr.sin6_addr, 16);
    this->sin6_family = addr.sin6_family;
    this->sin6_flowinfo = addr.sin6_flowinfo;
    this->sin6_port = addr.sin6_port;
    this->sin6_scope_id = addr.sin6_scope_id;
}

MySockaddr::MySockaddr(const sockaddr_in6 &addr, socklen_t len)
{
    this->addrlen = len;
    memcpy(&this->sin6_addr, &addr.sin6_addr, 16);
    this->sin6_family = addr.sin6_family;
    this->sin6_flowinfo = addr.sin6_flowinfo;
    this->sin6_port = addr.sin6_port;
    this->sin6_scope_id = addr.sin6_scope_id;
}

MySockaddr &MySockaddr::operator=(const MySockaddr &o)
{
    if (this == &o)
        return *this;

    this->addrlen = o.addrlen;
    memcpy(&this->sin6_addr, &o.sin6_addr, 16);
    this->sin6_family = o.sin6_family;
    this->sin6_flowinfo = o.sin6_flowinfo;
    this->sin6_port = o.sin6_port;
    this->sin6_scope_id = o.sin6_scope_id;

    return *this;
}

int MySockaddr::cmpIpv6(const MySockaddr &o) const
{
    return memcmp(this->sin6_addr.s6_addr, o.sin6_addr.s6_addr, 16);
}

int MySockaddr::cmpAddr(const MySockaddr &o) const
{
    if (this->sin6_family != o.sin6_family)
        return this->sin6_family < o.sin6_family ? -1 : 1;

    if (this->sin6_port == o.sin6_port)
        return cmpIpv6(o);
    return this->sin6_port < o.sin6_port ? -1 : 1;
}

void MySockaddr::print() const
{
    std::cout << "SOCK ADDR LEN IS: " << this->addrlen << "\n";
    std::cout << "SOCK FAMILY IS: " << this->sin6_family << "\n";
    std::cout << "SOCK FLOW IS: " << this->sin6_flowinfo << "\n";
    std::cout << "SOCK PORT IS: " << this->sin6_port << "\n";
    std::cout << "SOCK SCOPE IS: " << this->sin6_scope_id << "\n";
    std::cout << "BYTES: ";
    for (int i = 0; i < 16; ++i)
        std::cout << (int)this->sin6_addr.s6_addr[i] << " ";
    std::cout << "\n\n";
}
