#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <time.h>

int keytest(void);


#define UNICAST 1
#define MULTICAST 2
#define BROADCAST 3

//choose the mode
#define CAST 2

// config
#define UDP_PORT 1234
#define MULTICAST_ADDR inet_addr("225.0.0.37");
#define MULTICAST_TTL  32;
#define BROADCAST_ADDR inet_addr("10.0.2.255");
#define UNICAST_ADDR   inet_addr("10.0.2.15");


// context
int NETWORK_INTERFACE = -1;
char hostname[80];
struct in_addr hostip = {0};
int mysocket;

// UDP HEADER = 8bytes
// unsigned short source port; // 2
// unsigned short dest port;   // 2
// unsigned short lenght;	  // 2  --> max = 65535 bytes - 8 bytes = 65527 bytes
// unsigned short checksum;	// 2
#define MAXUDP_PAYLOADSZ 65527 
/* Shouldn't be a good idea to use the max UDB size as if we loose a packet, we loose a lot of data! */

// FRAME HEADER = 8bytes
typedef struct {
	unsigned short framenumber; // 2
	unsigned char  version;	 // 1
	unsigned char  frametype;   // 1
	unsigned long  address;	 // 4
	unsigned long  size;		// 4
} udp_frame_header_t;

#define TEST_FRAME_ID 1
typedef struct {
	unsigned char keycode;
	unsigned char dummy[1024*10]; 
} udp_test_frame_t;


long long milliseconds_now() {
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
    return 1000*(long long)time.tv_sec + ((long long)time.tv_nsec/1E6);
	//return 1000*(long long)time.tv_sec;
}

void changemode(int dir);
int kbhit (void);
int main(int argc, char** argv)
{

	// get interface number
	if (argc>1) {
		NETWORK_INTERFACE = atoi(argv[1]);				
	}

	// create a socket
	mysocket = socket(AF_INET , SOCK_DGRAM, IPPROTO_UDP);
	if (mysocket == -1) {
		fprintf(stderr, "Error in creating socket");
		return 1;
	}
	

	// socket options
	#if CAST == BROADCAST
	{
		// Allow broadcast
		char opt = 1;
		setsockopt(mysocket, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(char));		

		struct in_addr v;		
		v.s_addr = BROADCAST_ADDR
		fprintf(stderr, "BROADCAST_ADDR %s\n",inet_ntoa(v));
	}
	#elif CAST == UNICAST
	{
		struct in_addr v;		
		v.s_addr = BROADCAST_ADDR
		fprintf(stderr, "UNICAST_ADDR %s\n",inet_ntoa(v));
	}
	#elif  CAST == MULTICAST
	{
		// need to choose which network interface is going to send multicast
		struct ifaddrs *ifaddr, *ifa;
		int family, n,i=0;
		char host[NI_MAXHOST];

		if (getifaddrs(&ifaddr) == -1) {
			fprintf(stderr, "Error no interface");
			return 1;
		}

		for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
		{
			if (ifa->ifa_addr == NULL) continue;
			family = ifa->ifa_addr->sa_family;
			if (family != AF_INET) continue;
			if (!getnameinfo(ifa->ifa_addr, 
							sizeof(struct sockaddr_in),
							host,NI_MAXHOST,NULL, 0, NI_NUMERICHOST))
			{
				if (i==NETWORK_INTERFACE && hostip.s_addr==0) {
					inet_aton(host, &hostip);
					printf("[*%u*] %s : %s\n", i, ifa->ifa_name, host);	
				} else {
					printf("[%u] %s : %s\n", i, ifa->ifa_name, host);	
				}
				i++;
			}
		}
        freeifaddrs(ifaddr);
		if (NETWORK_INTERFACE==-1) {
			close(mysocket);
			return 0;
		}

		if (hostip.s_addr==0) {
			close(mysocket);
			fprintf(stderr, "interface [%i] unfound\n", NETWORK_INTERFACE);
			return 1;
		} else {
			struct in_addr v;
			v.s_addr = MULTICAST_ADDR
			fprintf(stderr, "MULTICAST %s\n",inet_ntoa(v));
		}

		// Set TTL>0 to pass router
		int ttl = MULTICAST_TTL ;
		if (setsockopt(mysocket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl)) <0) {
			close(mysocket);
			fprintf(stderr, "Error in IP_MULTICAST_TTL socket option");
			return 1;
		}

		// Use only a specific interface
		if (setsockopt(mysocket, IPPROTO_IP, IP_MULTICAST_IF, (char *)&(hostip.s_addr), sizeof(unsigned long)) < 0) {
			close(mysocket);
			fprintf(stderr, "Error in IP_MULTICAST_IF socket option");
			return 1;
		}

	}
	#endif

	// create destination address
	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(struct sockaddr_in));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(UDP_PORT);
	#if CAST == BROADCAST
		dest_addr.sin_addr.s_addr = `;
	#elif CAST == UNICAST
		dest_addr.sin_addr.s_addr = UNICAST_ADDR;
	#elif  CAST == MULTICAST
		dest_addr.sin_addr.s_addr = MULTICAST_ADDR;
	#endif

   
	size_t udp_packet_sz = sizeof(udp_frame_header_t) + sizeof(udp_test_frame_t);
	if (udp_packet_sz>=MAXUDP_PAYLOADSZ) {
		fprintf(stderr, "Too large packet (%u Bytes), UDP packets max is (%u Bytes)\n", udp_packet_sz, MAXUDP_PAYLOADSZ);
		return 1;
	} else {
		fprintf(stderr, "1 frame = %u Bytes\n", udp_packet_sz);
	}

	char * buffer = (char*)malloc(MAXUDP_PAYLOADSZ);
	if (!buffer) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}

	udp_frame_header_t* pheader = (udp_frame_header_t*)buffer;
	udp_test_frame_t* ptestFrame = (udp_test_frame_t*)((char*)buffer + sizeof(udp_frame_header_t));
	
	pheader->framenumber = 0xFFFF-10;
	pheader->version = 100;
	pheader->address = 0xFFFFFFFF; // all
	pheader->frametype = TEST_FRAME_ID;
	pheader->size = sizeof(udp_test_frame_t);

	// Send
	changemode(1);
	long long start = milliseconds_now();
	//printf("\n");
	while (1)
	{
		usleep(10*1000);

		int ch = 0x00;
		if (kbhit()) {
		  	ch = getchar();
			printf("\nPress on 0x%02X\n",ch);
		}

		pheader->framenumber++;
		ptestFrame->keycode=ch;

		float elapsed = (float)(milliseconds_now() - start)/1000;
		float totaldata = (float)(pheader->framenumber*udp_packet_sz) / 1024.0f / 1024.0f;
		
		if (pheader->framenumber % 1 == 0) {
			fprintf(stdout, "Tx: frame [%u], %0.0fMB (%0.2fMbits/s)   \r",
				     pheader->framenumber, totaldata, 8*totaldata / elapsed);
			fflush(stdout);
		}

		unsigned int ret = sendto(mysocket, buffer, udp_packet_sz, 0, 
								(struct sockaddr_in*)&dest_addr, sizeof(struct sockaddr_in));
		if (ret != udp_packet_sz) {
			fprintf(stderr, "Error broadcasting to the clients %d", ret);
			close(mysocket);
			return 1;
		}

		if (ch==0x0d||ch==0x0a) {
			printf("\nEND!\n");
			break;
		}
	}
	
	close(mysocket);
	changemode(0);
	if (buffer) free(buffer);

	return 0;
}





void changemode(int dir)
{
  static struct termios oldt, newt;
 
  if ( dir == 1 )
  {
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  }
  else
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}
 
int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;
 
  tv.tv_sec = 0;
  tv.tv_usec = 0;
 
  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);
 
  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
 
}
