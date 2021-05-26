#include "Poll.h"
#include "gui/gui2/err.h"

Poll::Poll(unsigned portArg, double roundTime) : port(portArg)
{
    rTime = (long int)(1000000000.f * roundTime);
    /* Inicjujemy tablicę z gniazdkami klientów, client[0] to gniazdko centrali */
    pollfd init;
    init.fd = -1;
    init.events = POLLIN;
    init.revents = 0;
    fds.emplace_back(init);

    /* Tworzymy gniazdko centrali */
    fds[0].fd = socket(PF_INET6, SOCK_DGRAM, 0);
    if (fds[0].fd == -1)
        syserr("Opening stream socket");

    int no = 0;
    if (setsockopt(fds[0].fd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no)) < 0)
        syserr("setsockopt");

    // int yes = 1;
    // if (setsockopt(fds[0].fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes)) < 0)
    //     syserr("reuseaddr");

    int yes = 1;
    if (ioctl(fds[0].fd, FIONBIO, (char *)&yes) < 0)
        syserr("ioctl");

    /* Co do adresu nie jesteśmy wybredni */
    memset(&server, 0, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_addr = in6addr_any;
    server.sin6_port = htons(portArg);
    if (bind(fds[0].fd, (struct sockaddr *)&server,
             (socklen_t)sizeof(server)) == -1)
        syserr("Binding stream socket");

    if (rTime == 1000000000.f)
        updateTimerFd = createTimer(1, 0);
    else
        updateTimerFd = createTimer(0, rTime);

    std::cout << "starting with socket " << fds[0].fd << '\n';
}

void Poll::clearRevents()
{
    for (size_t i = 0; i < fds.size(); i++)
        fds[i].revents = 0;
}

void Poll::littleMessage(ClientMessage &msg, int nameLen)
{
    msg.session_id = be64toh(msg.session_id);
    msg.next_expected_event_no = be32toh(msg.next_expected_event_no);
    msg.player_name[nameLen] = '\0';
}

int Poll::createTimer(int sec, long int nsec)
{
    // std::cout << "CREATING TIMER WITH NSEC: " << nsec << '\n';
    pollfd p;
    int timerfd;
    itimerspec timerValue;

    /* set timerfd */
    timerfd = timerfd_create(CLOCK_REALTIME, 0);
    if (timerfd < 0)
        syserr("timer");

    timerValue.it_value.tv_sec = sec;
    timerValue.it_value.tv_nsec = nsec;
    timerValue.it_interval.tv_sec = sec;
    timerValue.it_interval.tv_nsec = nsec;

    /* set events */
    p.fd = timerfd;
    p.revents = 0;
    p.events = POLLIN;

    fds.emplace_back(p);
    /* start timer */
    if (timerfd_settime(timerfd, 0, &timerValue, NULL) < 0)
        syserr("timer start");

    return timerfd;
}

void Poll::hardResetMainTimer()
{
    if (rTime == 1000000000.f)
        updateTimerFd = createTimer(1, 0);
    else
        updateTimerFd = createTimer(0, rTime);
    fds[1] = fds[fds.size() - 1];
    fds.erase(fds.end() - 1);
}

void Poll::addClient(const MySockaddr &addr)
{
    //std::cout << "ADDING CLIENT!  ";
    clients.emplace(addr);

    int timer = createTimer(2, 0);
    timers.emplace(std::pair<int, MySockaddr>(timer, addr));
    timersFromAddr.emplace(std::pair<MySockaddr, int>(addr, timer));
}

void Poll::deleteTimer(int timerFd)
{
    //std::cout << "DELETING TIMER : " << timerFd << "  SIZE:" << fds.size();
    fds.erase(std::find_if(fds.begin(), fds.end(), [timerFd](const pollfd &poll)
                           { return poll.fd == timerFd; }));
    //std::cout << "  SIZE:" << fds.size() << '\n';
}

void Poll::resetTimer(int timerFd, int sec, long int nsec)
{
    itimerspec timerValue;
    timerValue.it_value.tv_sec = sec;
    timerValue.it_value.tv_nsec = nsec;
    timerValue.it_interval.tv_sec = sec;
    timerValue.it_interval.tv_nsec = nsec;

    if (timerfd_settime(timerFd, 0, &timerValue, NULL) < 0)
        syserr("timer reset");
}

void Poll::resetTimer()
{
    resetTimer(updateTimerFd, 0, rTime);
}

bool Poll::checkName(char *name, int len)
{
    for (int i = 0; i < len; i++)
        if (name[i] < 33 || name[i] > 126)
            return false;
    return true;
}

PollOutput Poll::doPoll()
{
    PollOutput out;
    out.calcRound = false;
    out.noMsg = true;
    out.addr.addrlen = 0;

    //std::cout << "Polling\n";
    clearRevents();
    if (poll(&fds[0], fds.size(), -1) < 0)
        syserr("poll");

    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = sizeof(addr);
    ClientMessage message;

    if (fds[0].revents == POLLIN)
    {
        int recv;
        char buffin[34];
        memset(buffin, 0, 34);
        if ((recv = recvfrom(fds[0].fd, buffin, sizeof(buffin), 0, (sockaddr *)&addr, &addrlen)) < 0)
            syserr("recv");
        
        if (recv <= 33 && recv >= clientDataSize )
        {
            memcpy((char *)&message, buffin, sizeof(message));
            if (message.turn_direction <= 2 && checkName(message.player_name, recv - clientDataSize))
            {
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

    if (fds[1].revents == POLLIN)
    {
        resetTimer();
        out.calcRound = true;
    }

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
                    //std::cout << "removing  clients : " << clients.size() << " and timers: " << timers.size() << '\n';
                    timersFromAddr.erase(*it2);
                    //std::cout << "removed  clients : " << clients.size() << " and timers: " << timers.size() << '\n';
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

int Poll::getMainSock() const
{
    return fds[0].fd;
}
