#ifndef POLL_H
#define POLL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <netdb.h>
#include <vector>
#include <algorithm>
#include <endian.h>
#include <map>
#include <set>
#include <string.h>

#include "err.h"
#include "Client.h"
#include "Helper.h"

using sock_ptr = std::shared_ptr<MySockaddr>;

struct PollOutput
{
    //list of clients to be cleaned out
    std::vector<MySockaddr> toDisconnect;
    //client address
    MySockaddr addr;
    //actual message
    ClientMessage message;
    //whether to do another tick
    bool calcRound;
    //null structure identifier
    bool noMsg;
};

class Poll
{
private:
    int port;
    struct sockaddr_in6 server;

    //event descriptors
    std::vector<pollfd> fds;
    //timer descriptor to address
    std::map<int, MySockaddr> timers;
    //address to timer descriptor
    std::map<MySockaddr, int> timersFromAddr;

    //tick timer descriptor
    int updateTimerFd;
    //tick time
    long int rTime;
    
    //client address set
    std::set<MySockaddr> clients;
    
    //clears pollfd revents
    void clearRevents();
    //converts received message to little endian and adds \0 at the end
    void littleMessage(ClientMessage &msg, int nameLen) const;

    //add new client address to set
    void addClient(const MySockaddr &addr);
    //checks if given name is valid
    bool checkName(char *name, int len) const;

    //creates new timer and starts tracking it
    int createTimer(int sec, long int nsec);
    //resets timer's elapsed time
    void resetTimer(int timerFd, int sec, long int nsec);
    //makes object stop tracking of given timer
    void deleteTimer(int timerFd);
public:
    Poll(unsigned portArg, double roundTime);

    //main object function
    //it waits for some event to happen and returns PollOutput structure accordingly
    PollOutput doPoll();

    //returns main udp connection socket
    int getMainSock() const;

    //resets main tick timer
    void resetTimer();
};

#endif // !POLL_H
