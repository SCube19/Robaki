#include <getopt.h>
#include <iostream>
#include "Poll.h"
#include "EventHandler.h"
#include "err.h"

//option recognizion section
bool is_number(const std::string &s)
{
    return !s.empty() && std::find_if(s.begin(),
                                      s.end(), [](unsigned char c)
                                      { return !std::isdigit(c); }) == s.end();
}

void setOptions(options &options, int argc, char *argv[])
{
    try
    {
        int c;
        while (optind < argc)
        {
            if ((c = getopt(argc, argv, "p:ns:nt:nv:nw:nh:n")) != -1)
            {
                if (!is_number(std::string(optarg)))
                    syserr("Argument isn't a number");
                unsigned long long opt = std::stoull(optarg);
                if (opt < 1 || opt > UINT32_MAX)
                    syserr("Argument out of range");

                switch (c)
                {
                case 'p':
                    if (opt > UINT16_MAX)
                        syserr("Port number too high");
                    options.port = opt;
                    break;
                case 's':
                    options.seed = opt;
                    break;
                case 't':
                    if (opt > 90)
                        syserr("Turning speed can't be more than pi/2");
                    options.turningSpeed = opt;
                    break;
                case 'v':
                    if (opt > 250)
                        syserr("Tickrate can't be higher than 250");
                    options.roundsPerSecond = opt;
                    break;
                case 'w':
                    options.width = opt;
                    if (options.width >= 50000)
                        syserr("Max width is 50000");
                    break;
                case 'h':
                    options.height = opt;
                    if (options.height >= 50000)
                        syserr("Max height is 50000");
                    break;
                default:
                    std::cout << "Usage: ./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]\n";
                    exit(EXIT_FAILURE);
                    break;
                }
            }
            else
                exit(EXIT_FAILURE);
        }
    }
    catch (const std::exception &e)
    {
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    options options;
    setOptions(options, argc, argv);

    //making poll interface
    Poll poll(options.port, 1.0f / (double)options.roundsPerSecond);
    //making event handler interface
    EventHandler eventHandler(options, Sender(poll.getMainSock()));

    while (1)
    {
        //Checking for existing network events
        PollOutput out = poll.doPoll();
        //Clean eventHandler out of disconnected adresses
        eventHandler.disconnectClients(out.toDisconnect);

        //if game started and tick timer expired we calculate tick
        if (eventHandler.gameStarted() && out.calcRound)
            eventHandler.tick();

        //if we got any message from client
        if (!out.noMsg)
            eventHandler.manageResponse(out);

        //if game hasn't started check for readyness of clients
        if (!eventHandler.gameStarted() && eventHandler.clientsReady())
        {
            //resetting timer and creating starting game
            poll.resetTimer();
            eventHandler.startGame();
        }
    }
}