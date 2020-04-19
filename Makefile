# Portul pe care asculta serverul
PORT_SERVER = 10000

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

# Nume subscriber
NUME = gigel

all: server subscriber

# Pentru server
server: server.o client.o tcp_client_handler.o udp_client_handler.o
	g++ server.o client.o tcp_client_handler.o udp_client_handler.o -Wall -Wextra -o server

server.o: server.cpp
	g++ server.cpp -Wall -Wextra -c

client.o: client.cpp
	g++ client.cpp -Wall -Wextra -c

tcp_client_handler.o: tcp_client_handler.cpp
	g++ tcp_client_handler.cpp -Wall -Wextra -c

udp_client_handler.o: udp_client_handler.cpp
	g++ udp_client_handler.cpp -Wall -Wextra -c

# Pentru subscriber
subscriber: subscriber.o
	g++ subscriber.o -Wall -Wextra -o subscriber

subscriber.o: subscriber.cpp
	g++ subscriber.cpp -Wall -Wextra -c


# Ruleaza serverul
run_server:
	./server ${PORT_SERVER}

# Ruleaza clientul 	
run_subscriber:
	./subscriber ${IP_SERVER} ${PORT_SERVER} ${NUME}

clean:
	rm -f server subscriber *.o
