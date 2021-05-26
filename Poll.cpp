#include "Poll.h"

namespace
{
    //seconds to timeout client
    constexpr short timeout = 2;
    //session_id + next_expected_event_no + turn_direction size
    constexpr short clientDataSize = 13;
    //max client name size + 1
    constexpr short clientNameMax = 21;
} // namespace

Poll::Poll(unsigned portArg, double roundTime) : port(portArg)
{
    rTime = (long int)(1000000000.f * roundTime);
    
    //udp pollfd setup
    pollfd init;
    init.fd = -1;
    init.events = POLLIN;
    init.revents = 0;
    fds.emplace_back(init);

    //udp socket setup
    fds[0].fd = socket(PF_INET6, SOCK_DGRAM, 0);
    if (fds[0].fd == -1)
        syserr("Opening stream socket");

    int no = 0;
    if (setsockopt(fds[0].fd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no)) < 0)
        syserr("Setsockopt: IPPROTO_IPV6");

    int yes = 1;
    if (ioctl(fds[0].fd, FIONBIO, (char *)&yes) < 0)
        syserr("Ioctl");

    //binding v6 to v4 and v6
    memset(&server, 0, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_addr = in6addr_any;
    server.sin6_port = htons(portArg);
    if (bind(fds[0].fd, (struct sockaddr *)&server,
             (socklen_t)sizeof(server)) == -1)
        syserr("Binding stream socket");

    //creating tick timer
    if (rTime == 1000000000.f)
        updateTimerFd = createTimer(1, 0);
    else
        updateTimerFd = createTimer(0, rTime);
}

//============================================PRIVATE METHODS===========================================//
/////////////////////////////////CLEAR REVENTS////////////////////////////////////////////////////////////
void Poll::clearRevents()
{
    for (size_t i = 0; i < fds.size(); i++)
        fds[i].revents = 0;
}

////////////////////////////////LITTLE MESSAGE/////////////////////////////////////////////////////////////
void Poll::littleMessage(ClientMessage &msg, int nameLen) const
{
    msg.session_id = be64toh(msg.session_id);
    msg.next_expected_event_no = be32toh(msg.next_expected_event_no);
    msg.player_name[nameLen] = '\0';
}

/////////////////////////////////////ADD CLIENT////////////////////////////////////////////////////////////
void Poll::addClient(const MySockaddr &addr)
{
    clients.emplace(addr);

    int timer = createTimer(2, 0);
    timers.emplace(std::pair<int, MySockaddr>(timer, addr));
    timersFromAddr.emplace(std::pair<MySockaddr, int>(addr, timer));
}

//////////////////////////////////CHECK NAME///////////////////////////////////////////////////////////////
bool Poll::checkName(char *name, int len) const
{
    for (int i = 0; i < len; i++)
        if (name[i] < 33 || name[i] > 126)
            return false;
    return true;
}

////////////////////////////////////////////CREATE TIMER////////////////////////////////////////////////////
int Poll::createTimer(int sec, long int nsec)
{
    pollfd p;
    int timerfd;
    itimerspec timerValue;

    //creating timer
    timerfd = timerfd_create(CLOCK_REALTIME, 0);
    if (timerfd < 0)
        syserr("Timer creation");

    timerValue.it_value.tv_sec = sec;
    timerValue.it_value.tv_nsec = nsec;
    timerValue.it_interval.tv_sec = sec;
    timerValue.it_interval.tv_nsec = nsec;

    //setting listener
    p.fd = timerfd;
    p.revents = 0;
    p.events = POLLIN;
    fds.emplace_back(p);

    //starting timer
    if (timerfd_settime(timerfd, 0, &timerValue, NULL) < 0)
        syserr("Timer start");

    return timerfd;
}

////////////////////////////////////////////RESET TIMER////////////////////////////////////////////////
void Poll::resetTimer(int timerFd, int sec, long int nsec)
{
    itimerspec timerValue;
    timerValue.it_value.tv_sec = sec;
    timerValue.it_value.tv_nsec = nsec;
    timerValue.it_interval.tv_sec = sec;
    timerValue.it_interval.tv_nsec = nsec;

    if (timerfd_settime(timerFd, 0, &timerValue, NULL) < 0)
        syserr("Timer reset");
}

//============================PUBLIC======================================//
void Poll::resetTimer()
{
    resetTimer(updateTimerFd, 0, rTime);
}

//========================================PRIVATE===============================//
///////////////////////////////////////DELETE TIMER///////////////////////////////
void Poll::deleteTimer(int timerFd)
{
    fds.erase(std::find_if(fds.begin(), fds.end(), [timerFd](const pollfd &poll)
                           { return poll.fd == timerFd; }));
}

//===================================================PUBLIC METHODS====================================//
///////////////////////////////////////DO POLL///////////////////////////////////////////////////////////
PollOutput Poll::doPoll()
{
    PollOutput out;
    out.calcRound = false;
    out.noMsg = true;
    out.addr.addrlen = 0;

    //polling events
    clearRevents();
    if (poll(&fds[0], fds.size(), -1) < 0)
        syserr("poll");

    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = sizeof(addr);
    ClientMessage message;

    //if got message
    if (fds[0].revents == POLLIN)
    {   
        //receive message
        int recv;
        char buffin[34];
        memset(buffin, 0, 34);
        if ((recv = recvfrom(fds[0].fd, buffin, sizeof(buffin), 0, (sockaddr *)&addr, &addrlen)) < 0)
            syserr("recv");

        //validity checks
        if (recv <= 33 && recv >= clientDataSize)
        {
            memcpy((char *)&message, buffin, sizeof(message));
            if (message.turn_direction <= 2 && checkName(message.player_name, recv - clientDataSize))
            {   
                //saving and possibly adding new client
                littleMessage(message, recv - clientDataSize);
                out.addr = MySockaddr(addr, addrlen);

                auto it = timersFromAddr.find(out.addr);
                if (it != timersFromAddr.end())
                    resetTimer(it->second, 2, 0);

                if (clients.find(out.addr) == clients.end())
                    addClient(out.addr);

                out.noMsg = false;
            }
        }
    }

    //if tick timer expired
    if (fds[1].revents == POLLIN)
    {
        resetTimer();
        out.calcRound = true;
    }

    //disconnecting timeouted clients
    for (size_t i = 2; i < fds.size(); i++)
    {
        if (fds[i].revents == POLLIN)
        {
            auto it = timers.find(fds[i].fd);
            if (it != timers.end())
            {
                auto it2 = clients.find(it->second);
                if (it2 != clients.end())
                {
                    out.toDisconnect.emplace_back(*it2);
                    timersFromAddr.erase(*it2);
                    deleteTimer(it->first);
                    clients.erase(it2);
                    timers.erase(it);
                }
            }
        }
    }

    out.message = message;
    return out;
}

////////////////////////////////GET MAIN SOCK//////////////////////////////////////////////////////
int Poll::getMainSock() const
{
    return fds[0].fd;
}
