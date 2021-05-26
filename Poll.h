#ifndef POLL_H
#define POLL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <endian.h>
#include <map>
#include <set>
#include <memory>
#include <string.h>

#include "gui/gui2/err.h"
#include "Client.h"
#include "Helper.h"

using sock_ptr = std::shared_ptr<MySockaddr>;

namespace
{
    constexpr short timeout = 2;
    constexpr short clientMax = 26;
    constexpr short clientDataSize = 13;
    constexpr short clientNameMax = 21;
} // namespace

struct PollOutput
{
    std::vector<MySockaddr> toDisconnect;
    MySockaddr addr;
    ClientMessage message;
    bool calcRound;
    bool noMsg;
};

class Poll
{

private:
    int port;
    long int rTime;
    std::vector<pollfd> fds;
    int updateTimerFd;
    struct sockaddr_in6 server;

    std::set<MySockaddr> clients;
    std::map<int, MySockaddr> timers;
    std::map<MySockaddr, int> timersFromAddr;

    void clearRevents();
    void littleMessage(ClientMessage &msg, int nameLen);
    void deleteTimer(int timerFd);
    int createTimer(int sec, long int nsec);
    void resetTimer(int timerFd, int sec, long int nsec);
    void addClient(const MySockaddr &addr);
    bool checkName(char* name, int len);
public:
    Poll(unsigned portArg, double roundTime);
    PollOutput doPoll();
    int getMainSock() const;
    void resetTimer();
    void hardResetMainTimer();
};

#endif // !POLL_H
