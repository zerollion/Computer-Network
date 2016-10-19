//Tan Zhen 872692777
//COMP 3621
//tcp-client.cpp

#include "tcp.h"

//--------------------------create a socket---------------------------------------------
typedef struct {
	int client_socket;
	const char *hostname;
	const char *portnum;
	// 2k size buffers
	char recv_buffer[2048];
	char send_buffer[2048];
	// address info
	struct sockaddr addr;
	socklen_t addrlen;

	//game values
	float x;
	float y;
	float z;
	int ID;

} Client;

//get address info
struct addrinfo *getClientAddress(const char *hostname,
	const char *portnum)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));

	// set up case for TCP socket
	hints.ai_family = AF_INET; // IP4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	// store result here
	struct addrinfo *result;

	int res = getaddrinfo(hostname, portnum, &hints, &result);

	if (res != 0) {
		fprintf(stderr, "Error resolving host %s on port %s: %s\n", hostname, portnum,
			gai_strerror(res));

		return 0;
	}

	return result;
}

/* create a socket based on the getaddrinfo result */
int create_socket(const struct addrinfo *res)
{
	int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (s == -1) {
		fprintf(stderr, "Error creating socket: %s\n", strerror(errno));
		return 0;
	}

	return s;
}

//----------create a tcp client with info specified---------------------------------
Client *create_client(const char *hostname, const char *portnum)
{
	struct addrinfo *addr = getClientAddress(hostname, portnum);

	// if it returns 0, just return null
	if (addr == 0)
		return 0;

	// create socket
	int s = create_socket(addr);
	if (s == -1)
		return 0;

	// init struct
	Client *client = (Client *)malloc(sizeof(Client));
	memset(client, 0, sizeof(Client));

	client->client_socket = s;
	client->portnum = portnum;
	client->hostname = hostname;
	memcpy(&client->addr, addr->ai_addr, sizeof(struct sockaddr));
	client->addrlen = addr->ai_addrlen;

	// init player info
	client->ID = -1;
	client->x = -1.0;
	client->y = -1.0;
	client->z = -1.0;

	// connet
	int c = connect(client->client_socket, &client->addr, client->addrlen);

	if (c == -1) {
		fprintf(stderr, "Error connecting socket: %s\n", strerror(errno));
		return NULL;
	}

	return client;
}

//----------------------send message---------------------------------------
int send_msg(Client *client, const char *msg, int l)
{
	fprintf(stdout, "sending message of %d bytes\n", l);

	strncpy(&client->send_buffer[0], msg, l);

	ssize_t bytes = send(client->client_socket,
		&client->send_buffer[0],
		l, 0);


	if (bytes == -1) {
		fprintf(stderr, "Error sending message: %s\n", strerror(errno));
		return 0;
	}

	//fprintf(stdout, "------sent %d bytes-----\n", bytes);
	return bytes;
}
//----------------------recv message-----------------------------------------
int recv_msg(Client *client, char *msg)
{
	ssize_t bytes = recv(client->client_socket,
		&client->recv_buffer[0],
		2047, 0);

	if (bytes == -1 && errno != EWOULDBLOCK  && errno != EAGAIN) {
		fprintf(stderr, "Error receiving message: %s\n", strerror(errno));
		return 0;
	}

	if (bytes == -1) {
		return 0;
	}

	const char* ptr = &(client->recv_buffer[0]);
	strncpy(msg, ptr, bytes);

	fprintf(stdout, "--recieved %ld bytes, message type: %d\n", bytes, msg[1]);

	//-------------------- message reading section ---------------------
	if (client->recv_buffer[1] == 4) {	//PlayerJoinResponse
		memcpy(&client->ID, &(client->recv_buffer[2]), sizeof(int));
		fprintf(stdout, "recieved ID: %d \n", client->ID);
	}

	if (client->recv_buffer[1] == 5) {	//ServerMapUpdate 
		for (int i = 4; i < bytes; i += 16) { //read IDs
			int * d = (int *)&(client->recv_buffer[i]);
			if (*d == client->ID) { //find my ID and update position
				float * x = (float *)&(client->recv_buffer[i + 4]);
				float * y = (float *)&(client->recv_buffer[i + 8]);
				float * z = (float *)&(client->recv_buffer[i + 12]);

				client->x = *x;
				client->y = *y;
				client->z = *z;
				break;
			}
		}
		fprintf(stdout, "recieved current position: %f %f %f \n", client->x, client->y, client->z);

		if (client->x == -1.0) {
			//printf("--------------------------------------A\n");
			return -2; // mark of Annihilated
		}
	}

	//printf("---------------------------------------\n");

	return bytes;

}
//----------------------PlayerMove message---------------------------------------
int PlayerMove(Client *client, float x, float y, float z)
{
	if (client->ID == -1) {
		printf("cannot move, robot does not exist.   \n");
		return -1;
	}

	char *s = (char*)malloc(sizeof(char) * 14); // create message
	s[0] = 1; // version
	s[1] = 1; // type 1: player move
	memcpy(s + 2, &x, sizeof(float));
	memcpy(s + 6, &y, sizeof(float));
	memcpy(s + 10, &z, sizeof(float));

	float* x1 = (float*)(s + 2);
	float* y1 = (float*)(s + 6);
	float* z1 = (float*)(s + 10);

	printf("Sent Message Move: x:%f y:%f z:%f\n", *x1, *y1, *z1);
	ssize_t bytes = send_msg(client, s, 14);

	return bytes;

}
//----------------------PlayerSelf Annihilate--------------------------------------
int PlayerSelfAnnihilate(Client *client)
{
	if (client->ID == -1) {
		printf("cannot Annihilate, robot does not exist.   \n");
		return -1;
	}
	char *s = (char*)malloc(sizeof(char) * 2); // create message
	s[0] = 1; // version
	s[1] = 2; // type 2: PlayerSelf Annihilate

	ssize_t bytes = send_msg(client, s, 2);

	printf("Self Annihilate \n");

	return bytes;
}
//----------------------Player Spawn---------------------------------------
int PlayerSpawn(Client *client, float x, float y, float z)
{
	client->x = x;
	client->y = y;
	client->z = z;

	char *s = (char*)malloc(sizeof(char) * 14); // create message
	s[0] = 1; // version
	s[1] = 3; // type 3: Player Spawn
	memcpy(s + 2, &x, sizeof(float));
	memcpy(s + 6, &y, sizeof(float));
	memcpy(s + 10, &z, sizeof(float));

	ssize_t bytes = send_msg(client, s, 14);

	printf("Spawn at: x:%f y:%f z:%f\n", x, y, z);

	return bytes;

}


int main(int argc, const char *argv[])
{
	char addrname[INET_ADDRSTRLEN];

	if (argc >= 3) {
		printf("Creating a TCP client to connect to %s on port %s", argv[1], argv[2]);
		// create the client
		Client *client = create_client(argv[1], argv[2]);

		if (client != 0) {
			printf(", created at %s, done.\n",
				inet_ntop(AF_INET,
					&(((struct sockaddr_in *)&client->addr)->sin_addr), &addrname[0], INET_ADDRSTRLEN));
		}
		//recieve message before send
		char *s = (char*)malloc(sizeof(char) * 2048);

		//random genrator
		srand(static_cast <unsigned> (time(0)));
		float x = ((float)rand()) / (float)RAND_MAX;
		float y = ((float)rand()) / (float)RAND_MAX;
		float z = ((float)rand()) / (float)RAND_MAX;

		recv_msg(client, s);
		PlayerSpawn(client, x, y, z);

		// timer
		std::clock_t start;
		start = std::clock();

		//----------------------make socket nonblocking----------------------
		int flags = fcntl(client->client_socket, F_GETFL);
		if (flags == -1) {
			perror("can¡¯t get flags using fcntl()");
			exit(EXIT_FAILURE);
		}
		flags = fcntl(client->client_socket, F_SETFL, flags | O_NONBLOCK);
		if (flags == -1) {
			perror("can¡¯t set O_NONBLOCK using fcntl()");
			exit(EXIT_FAILURE);
		}
		//--------------------------------------------------------------------

		// time tick
		int tick = 1;

		//infinite loop, sending and reciving message
		while (true) {
			recv_msg(client, s);

			if (((std::clock() - start) / (double)CLOCKS_PER_SEC) >= 0.05) {
				start = std::clock();

				float x = ((float)rand()) / (float)RAND_MAX;
				float y = ((float)rand()) / (float)RAND_MAX;
				float z = ((float)rand()) / (float)RAND_MAX;

				if (client->x == -1.0) { // respawn if not exist
					PlayerSpawn(client, x, y, z);
				}
				else {
					PlayerMove(client, x, y, z);
				}

				//self Annihilate every 100 tick
				if (tick % 100 == 0) {
					tick = 1;
					PlayerSelfAnnihilate(client);
				}
				tick++;
			}
		}

		return 0;
	}
	else {
		printf("Usage: tcp-client ip port  \n");
		return 0;
	}
}
