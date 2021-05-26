CFLAGS = -c -Wall -Wextra -std=c++17 -O2 -g

default: screen-worms-server

err.o: err.cpp err.h
	g++ $(CFLAGS) err.cpp -o err.o  

Sender.o: Sender.cpp Sender.h Helper.h
	g++ $(CFLAGS) Sender.cpp -o Sender.o

EventHandler.o: EventHandler.h EventHandler.cpp Helper.h Sender.h
	g++ $(CFLAGS) EventHandler.cpp -o EventHandler.o

Client.o: Client.h Client.cpp
	g++ $(CFLAGS) Client.cpp -o Client.o

Poll.o: Poll.h Poll.cpp err.h Helper.h
	g++ $(CFLAGS) Poll.cpp -o Poll.o

Helper.o: Helper.h Helper.cpp
	g++ $(CFLAGS) Helper.cpp -o Helper.o

screen-worms-server.o: screen-worms-server.cpp 
	g++ $(CFLAGS) screen-worms-server.cpp -o screen-worms-server.o
    
screen-worms-server: screen-worms-server.o Poll.o err.o Client.o Helper.o EventHandler.o Sender.o
	g++ err.o screen-worms-server.o Poll.o Client.o Helper.o EventHandler.o Sender.o -o screen-worms-server

clean:
	rm -f screen-worms-server.o
	rm -f screen-worms-server
	rm -f Poll.o
	rm -f err.o
	rm -f Client.o
	rm -f Helper.o
	rm -f EventHandler.o
	rm -f Sender.o

	

