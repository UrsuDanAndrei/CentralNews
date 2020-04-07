# Portul pe care asculta serverul
PORT_SERVER = 10000

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

all: server client 

# Compileaza server.c
server: server.cpp
	g++ server.cpp -Wall -Wextra -o server

# Compileaza client.c
subscriber: subscriber.cpp
	g++ subscriber.cpp -Wall -Wextra -o subscriber

# Ruleaza serverul
run_server:
	./server ${PORT_SERVER}

# Ruleaza clientul 	
run_subscriber:
	./subscriber ${IP_SERVER} ${PORT_SERVER}

clean:
	rm -f server subscriber
