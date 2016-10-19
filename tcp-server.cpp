//Tan Zhen 872692777
//COMP 3621
//tcp-server.cpp

#include "tcp.h"

//--------------------------store each client---------------------------------------
struct Player {
	//socket_fd
	int client_socket;
	//game values
	float x;
	float y;
	float z;

	//constructor
	Player(int s, float x1, float y1, float z1) {
		client_socket = s;
		x = x1;
		y = y1;
		z = z1;
	}

};

//--------------------------distance function---------------------------------------
float dist(Player p1, Player p2) {

	if (p1.x < 0 || p2.x < 0) return 2.0;

	float d = (p1.x - p2.x)*(p1.x - p2.x) + (p1.y - p2.y)*(p1.y - p2.y)
		+ (p1.z - p2.z)*(p1.z - p2.z);
	return d;
}


//--------------------------create a socket-----------------------------------------
typedef struct {
	int server_socket;
	const char *hostname;
	const char *portnum;
	// 2k size buffers
	char recv_buffer[2048];
	char send_buffer[2048];
	// address info
	struct sockaddr addr;
	socklen_t addrlen;

	// client info
	std::vector<Player> p;
} Server;

//get address info
struct addrinfo *getServerAddress(const char *portnum)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));

	// set up case for TCP socket
	hints.ai_family = AF_INET; // IP4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_PASSIVE;

	// store result here
	struct addrinfo *result;

	int res = getaddrinfo(NULL, portnum, &hints, &result);

	if (res != 0) {
		fprintf(stderr, "Error resolving on port %s: %s\n", portnum,
			gai_strerror(res));

		return 0;
	}

	return result;
}

/* create a socket based on the getaddrinfo result */
int create_socket(const struct addrinfo *res)
{
	int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	//----------------------make socket nonblocking----------------------
	int flags = fcntl(s, F_GETFL);
	if (flags == -1) {
		perror("can¡¯t get flags using fcntl()");
		exit(EXIT_FAILURE);
	}
	flags = fcntl(s, F_SETFL, flags | O_NONBLOCK);
	if (flags == -1) {
		perror("can¡¯t set O_NONBLOCK using fcntl()");
		exit(EXIT_FAILURE);
	}
	//--------------------------------------------------------------------

	if (s == -1) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		return 0;
	}

	return s;
}

//----------create a tcp server with info specified---------------------------------
Server *create_server(const char *portnum)
{
	struct addrinfo *addr = getServerAddress(portnum);

	// if it returns 0, just return null
	if (addr == 0)
		return 0;

	// create socket
	int s = create_socket(addr);
	if (s == -1)
		return 0;

	// bind socket
	int res = bind(s, addr->ai_addr, addr->ai_addrlen);
	if (res == -1) {
		fprintf(stderr, "Unable to bind: %s\n", strerror(errno));
	}

	// server object
	Server *server = (Server *)malloc(sizeof(Server));
	memset(server, 0, sizeof(Server));
	server->server_socket = s;
	server->portnum = portnum;
	memcpy(&server->addr, addr->ai_addr, sizeof(struct sockaddr));
	server->addrlen = addr->ai_addrlen;

	return server;
}

// playerJoinResponse method
int playerJoinResponse(Server *server, int socket) {

	int n;
	char sendbuf[6];
	bzero(sendbuf, 6);
	sendbuf[0] = 1;
	sendbuf[1] = 4;
	memcpy(sendbuf + 2, &socket, sizeof(int));

	printf("player join response ID: %d socket: %d \n", sendbuf[2], socket);
	n = send(socket, sendbuf, 6, 0);

	if (n < 0) {
		perror("ERROR writing to socket, player join response");
		exit(1);
	}

	//add player to the vector
	server->p.emplace_back(socket, -1.0, -1.0, -1.0);

	return 0;
}

// PlayerSelfAnnihilate message
int SelfAnnihilate(Server *server, int socket) {

	// creating AnnihilationResults message
	char sendbuf[1024];
	bzero(sendbuf, 1024);
	sendbuf[0] = 1;
	sendbuf[1] = 7; // AnnihilationResults
	int numAffectd = 1; //player itself

	int source;
	int numPlayer = server->p.size();

	// find source of PlayerSelfAnnihilate
	for (int i = 0; i < numPlayer; i++) {

		if (server->p[i].client_socket == socket) {

			source = i;
			server->p[source].x = -1.0;
			server->p[source].y = -1.0;
			server->p[source].z = -1.0;

			memcpy(sendbuf + 2, &socket, sizeof(int)); //ID of self-annihilation initiator

			printf("Player ID:%d, SelfAnnihilated \n", socket);
			printf("Annihilated ID:%d, set to x:%f y:%f z:%f \n", socket,
				server->p[source].x, server->p[source].y, server->p[source].z);
		}
	}

	// find others affected
	for (int i = 0; i < numPlayer; i++) {

		if (dist(server->p[i], server->p[source]) < 0.25
			&& server->p[i].client_socket != socket && server->p[i].x != -1.0) { // square of 0.5

	//-------------------------chain reaction------------------------
			//recursive call
			SelfAnnihilate(server, server->p[i].client_socket);
			//----------------------------------------------------------------		

			server->p[i].x = -1.0;
			server->p[i].y = -1.0;
			server->p[i].z = -1.0;

			// id of affected players
			memcpy(sendbuf + (4 + 4 * numAffectd), &(server->p[i].client_socket), sizeof(int));
			numAffectd++;

			printf("Player ID:%d, affectd by SelfAnnihilated of Player ID: %d \n", server->p[i].client_socket, socket);
		}
	}

	// num of affected players 16bits
	short nump = (short)numAffectd;
	memcpy(sendbuf + 6, &nump, sizeof(short));

	int n = send(socket, sendbuf, 18, 0);

	if (n < 0) {
		perror("ERROR writing to socket: AnnihilationResults");
		exit(1);
	}
	return 0;
}


// response method: for type 1 2 and 3
int moveResponse(Server *server, int socket) {
	int n = 0;
	char buffer[14];
	bzero(buffer, 14);
	n = recv(socket, buffer, 14, 0);

	if (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
		perror("ERROR reading from socket");
		exit(1);
	}

	//-------- identify message type ----------------------------------------
	if (buffer[1] == 3) { // playerSpwan message
		float* x = (float*)(buffer + 2);
		float* y = (float*)(buffer + 6);
		float* z = (float*)(buffer + 10);

		// create PlayerSpawnWithID message
		char sendbuf[18];
		bzero(sendbuf, 18);
		sendbuf[0] = 1;
		sendbuf[1] = 6; // PlayerSpawnWithID

		memcpy(sendbuf + 2, &socket, sizeof(int)); // spawn player id
		memcpy(sendbuf + 6, buffer + 2, sizeof(float));
		memcpy(sendbuf + 10, buffer + 6, sizeof(float));
		memcpy(sendbuf + 14, buffer + 10, sizeof(float));

		int numPlayer = server->p.size();
		for (int i = 0; i < numPlayer; i++) {
			if (server->p[i].client_socket == socket) {
				server->p[i].x = *x;
				server->p[i].y = *y;
				server->p[i].z = *z;

				printf("Player ID:%d, Spawn at x:%f y:%f z:%f \n", socket, *x, *y, *z);
			}
			else { //send PlayerSpawnWithID message

				printf("Send to player: %d NewPlayer ID:%d, Spawn at x:%f y:%f z:%f \n",
					server->p[i].client_socket, socket, *x, *y, *z);
				n = send(server->p[i].client_socket, sendbuf, 18, 0);

				if (n < 0) {
					perror("ERROR writing to socket");
					exit(1);
				}
			}
		}
	}

	if (buffer[1] == 1) { // playerMove message
		float* x = (float*)(buffer + 2);
		float* y = (float*)(buffer + 6);
		float* z = (float*)(buffer + 10);

		int numPlayer = server->p.size();
		for (int i = 0; i < numPlayer; i++) {
			if (server->p[i].client_socket == socket) {

				if (server->p[i].x == -1.0) { // player not spawn or Annihilated
					return 0;
				}

				//-------------------------Set Warp to Minimum---------------------	
				float d = (server->p[i].x - *x)*(server->p[i].x - *x)
					+ (server->p[i].y - *y)*(server->p[i].y - *y)
					+ (server->p[i].z - *z)*(server->p[i].z - *z);//distance square

				if (d <= 0.25) {
					server->p[i].x = *x;
					server->p[i].y = *y;
					server->p[i].z = *z;
				}
				else {
					// linear interpret
					float k = 0.5 / sqrt(d);
					server->p[i].x = (*x - server->p[i].x)*k + server->p[i].x;
					server->p[i].y = (*y - server->p[i].y)*k + server->p[i].y;
					server->p[i].z = (*z - server->p[i].z)*k + server->p[i].z;
				}
				//-----------------------------------------------------------------

				printf("Player ID:%d, Moved to x:%f y:%f z:%f \n", socket, *x, *y, *z);
			}
		}
	}

	if (buffer[1] == 2) { // PlayerSelfAnnihilate message
		SelfAnnihilate(server, socket);
	}

	return 0;

}

// ServerMapUpdate message
int ServerMapUpdate(Server *server) {

	// make message
	char sendbuf[1024];
	bzero(sendbuf, 1024);
	sendbuf[0] = 1;
	sendbuf[1] = 5; // AnnihilationResults

	short numPlayer = (short)server->p.size();
	memcpy(sendbuf + 2, &numPlayer, sizeof(short));

	for (int i = 0; i < numPlayer; i++) {
		memcpy(sendbuf + (4 + i * 16), &(server->p[i].client_socket), sizeof(int));
		memcpy(sendbuf + (8 + i * 16), &(server->p[i].x), sizeof(float));
		memcpy(sendbuf + (12 + i * 16), &(server->p[i].y), sizeof(float));
		memcpy(sendbuf + (16 + i * 16), &(server->p[i].z), sizeof(float));
	}

	int bytes = 4 + 16 * numPlayer;

	// send message
	int n = 0;
	for (int i = 0; i < numPlayer; i++) {
		n = send(server->p[i].client_socket, sendbuf, bytes, 0);

		if (n < 0) {
			printf("ERROR writing to socket: ServerMapUpdate at index %d, socket %d", i, server->p[i].client_socket);
		}
	}

	return 0;
}

int main(int argc, const char *argv[])
{
	// ignore sigpipe
	signal(SIGPIPE, SIG_IGN);

	//char addrname[INET_ADDRSTRLEN];
	printf("Creating a TCP socket on port %s\n", argv[1]);
	Server *server = create_server(argv[1]);
	if (server == 0) {
		fprintf(stderr, "server error: %s\n", strerror(errno));
		return 0;
	}

	// create socket-list
	int fdmax = server->server_socket;
	int fdtot;
	fd_set master;
	fd_set readset;
	FD_ZERO(&master);
	FD_ZERO(&readset);

	// push the server socket into master
	FD_SET(server->server_socket, &master);
	FD_SET(server->server_socket, &readset);

	// time value for select
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	// try to listen for connections
	if (listen(server->server_socket, 5)) {
		fprintf(stderr, "failed to listen for connections: %s\n", strerror(errno));
		return 0;
	}

	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	int newsocket;

	// timer
	std::clock_t start;
	start = std::clock();

	// accept all connections
	while (true) {

		readset = master;

		// select socket
		fdtot = select(fdmax + 1, &readset, 0, 0, &timeout);
		if (fdtot < 0) {
			fprintf(stderr, "failed to select: %s\n", strerror(errno));
			return 0;
		}

		for (int i = 0; i < fdmax + 1; ++i) {

			if (FD_ISSET(i, &readset)) {

				if (i == server->server_socket) {
					newsocket = accept(server->server_socket, (struct sockaddr *)&cli_addr, &clilen);

					if (newsocket < 0) {
						fprintf(stderr, "accept error: %s\n", strerror(errno));
						continue; // if accept failed. do nothing
					}
					// add new socket into master
					FD_SET(newsocket, &master);
					if (newsocket > fdmax) fdmax = newsocket;

					// response
					playerJoinResponse(server, newsocket);
				}
				else {
					moveResponse(server, i);
				}
			}

		}

		// ServerMapUpdata
		if (((std::clock() - start) / (double)CLOCKS_PER_SEC) >= 0.05) {
			start = std::clock();

			ServerMapUpdate(server);
		}

	}

	return 0;
}

