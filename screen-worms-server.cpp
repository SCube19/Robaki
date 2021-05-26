#include <iostream>
#include <getopt.h>
#include <string>
#include <map>
#include "Client.h"
#include "Poll.h"
#include "EventHandler.h"
#include "Sender.h"
#include <chrono>

bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(), 
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
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
                if(!is_number(std::string(optarg)))
                        exit(EXIT_FAILURE);
                unsigned long long opt = std::stoull(optarg);
                if(opt < 1 || opt > UINT32_MAX)  
                    exit(EXIT_FAILURE);

                switch (c)
                {
                case 'p':
                    if (opt > UINT16_MAX)
                        exit(EXIT_FAILURE);
                    options.port = opt;
                    break;
                case 's':
                    options.seed = opt;
                    break;
                case 't':
                    if (opt > 90)
                        exit(EXIT_FAILURE);
                    options.turningSpeed = opt;
                    break;
                case 'v':
                    if (opt > 250)
                        exit(EXIT_FAILURE);
                    options.roundsPerSecond = opt;
                    break;
                case 'w':
                    options.width = opt;
                    if (options.width >= 50000)
                        exit(EXIT_FAILURE);
                    break;
                case 'h':
                    options.height = opt;
                    if (options.height >= 50000)
                        exit(EXIT_FAILURE);
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

    Poll poll(options.port, 1.0f / (double)options.roundsPerSecond);
    EventHandler eventHandler(options, Sender(poll.getMainSock()));
    int tickCounter = 0;
    while (1)
    {
        auto start = std::chrono::high_resolution_clock::now();
        PollOutput out = poll.doPoll();
        eventHandler.disconnectClients(out.toDisconnect);
        if (eventHandler.gameStarted() && out.calcRound)
        {
            eventHandler.tick();
            tickCounter++;
            auto end = std::chrono::high_resolution_clock::now();
            //std::cout << "TICKCOUNTER: " << tickCounter << "\nEST. TICK DURATION: " << std::chrono::duration<double>(end - start).count() << '\n';
        }
        
        if (!out.noMsg)
            eventHandler.manageResponse(out);

        if (!eventHandler.gameStarted() && eventHandler.clientsReady())
        {
            poll.resetTimer();
            eventHandler.startGame();     
        }
    }
}