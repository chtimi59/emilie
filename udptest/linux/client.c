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

int keytest(void);


#define UNICAST 1
#define MULTICAST 2
#define BROADCAST 3

//choose the mode
#define CAST 2

// config
#define UDP_PORT 1234
#define MULTICAST_ADDR inet_addr("225.0.0.37");

// context
int NETWORK_INTERFACE = -1;
int mysocket;
struct in_addr hostip = {0};

#define MAXUDP_PAYLOADSZ 65527

// FRAME HEADER = 8Bytes
typedef struct {
	unsigned short framenumber; // 2
	unsigned char  version;     // 1
	unsigned char  frametype;   // 1
	unsigned long  address;     // 4
	unsigned long  size;        // 4
} upd_frame_header_t;

#define TEST_FRAME_ID 1
typedef struct {
	unsigned char keycode;
	unsigned char dummy[1024*10];
} upd_test_frame_t;

char* rxbuff = NULL;

bool first = true;
unsigned long old_framenumber;
unsigned long stats_reordered_count=0;  // nb of frames reorder
unsigned long stats_missing_count=0;    // nb of frames missing
unsigned long stats_receivedok_count=0; // nb of frames ok
unsigned long stats_total_count=0;	    // nb of frame missing + ok
unsigned long stats_total_bytes=0; // bytes reveived



int ontime(struct sockaddr* src_addr) {
	upd_frame_header_t* pheader = NULL;

    fd_set fds;
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(mysocket, &fds);
	
    int rc = select(sizeof(fds)* 8, &fds, NULL, NULL, &timeout);
    if (rc > 0) {

		socklen_t len = sizeof(struct sockaddr_in);
		unsigned int recvbytes = recvfrom(mysocket, rxbuff, MAXUDP_PAYLOADSZ, 0, src_addr, &len);
		if (recvbytes > sizeof(upd_frame_header_t))
		{			
			pheader = (upd_frame_header_t*)rxbuff;			
			if (first) {
				first=false;
				old_framenumber = pheader->framenumber;					
			} else {
				signed long dist = pheader->framenumber - old_framenumber;
				if ((dist<0) && (abs(dist)>(0xFFFF/2))) {
					old_framenumber = pheader->framenumber;	// simple rollover
					fprintf(stdout, "rollover!\n");
					fflush(stdout);
				} else {
					if (old_framenumber >= pheader->framenumber) {
						stats_reordered_count++;
					} else {	
						stats_receivedok_count++;
						stats_missing_count += pheader->framenumber-old_framenumber-1;
						old_framenumber = pheader->framenumber;	
					}

					stats_total_count = stats_receivedok_count + stats_missing_count;
					stats_total_bytes += sizeof(upd_frame_header_t) + pheader->size;
					fprintf(stdout, "Rx: frame [%u], total %0.0fMB, missing %lu, reorder %lu, quality: %0.2f%%  \r", 
						pheader->framenumber,
						(float)(stats_total_bytes)/1024.0f/1024.0f,
						stats_missing_count,
						stats_reordered_count,
						100*(float)stats_receivedok_count/(float)stats_total_count);
					fflush(stdout);
					
				}
			}		
			
			switch(pheader->frametype) {
					case TEST_FRAME_ID: {
						if (recvbytes>=sizeof(upd_frame_header_t)+sizeof(upd_test_frame_t)) {
							upd_test_frame_t* pfrm = (upd_test_frame_t*)(rxbuff+sizeof(upd_frame_header_t));
							if (pfrm->keycode==0x0d||pfrm->keycode==0x0a) {
								printf("\n");
								return 1;
							}
							if (pfrm->keycode) {
								printf("\nKey pressed 0x%02X\n",pfrm->keycode);
							}
						break;
					}
				}
			}
        }
		
	}
	return 0;
}


void changemode(int dir);
int kbhit (void);
int main(int argc, char ** argv)
{
	struct sockaddr_in src_addr;

	// get interface number
	if (argc>1) {
		NETWORK_INTERFACE = atoi(argv[1]);				
	}

	// init rxbuffer
	rxbuff = (char* )malloc(MAXUDP_PAYLOADSZ);
	if (!rxbuff) {
		fprintf(stderr, "out of memory");
        return 1;
	}

	// create a socket
	mysocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (mysocket == -1) {
		fprintf(stderr, "Error in creating socket");
		return 1;
	}

	#if CAST == BROADCAST
		fprintf(stderr, "BROADCAST\n");
	#elif CAST == UNICAST
		fprintf(stderr, "UNICAST\n");
	#elif  CAST == MULTICAST
	{
		// need to choose which network interface is going to listen multicast

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
		
		// socket option : Allow to reuse multicast socket by others app
		unsigned int yes=1;
		if (setsockopt(mysocket,SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof(yes)) < 0) {
			close(mysocket);
			fprintf(stderr, "Error in SO_REUSEADDR socket option");
		}

		// socket option : Request that the kernel join a multicast group */
		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr=MULTICAST_ADDR;
		mreq.imr_interface.s_addr=hostip.s_addr;
		if (setsockopt(mysocket,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mreq,sizeof(mreq)) < 0) {
			close(mysocket);
			fprintf(stderr, "Error in IP_ADD_MEMBERSHIP socket option");
		}
	}
	#endif


	// create source address
	memset(&src_addr, 0, sizeof(struct sockaddr_in));
	src_addr.sin_family = AF_INET;
	src_addr.sin_port = htons(UDP_PORT);
	src_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   
	// BIND !
	if (bind(mysocket, (struct sockaddr*) &src_addr, sizeof(struct sockaddr)) < 0) {
		fprintf(stderr, "Error binding in the server socket");
		close(mysocket);
		return 1;
	}	


	changemode(1);
	while (!kbhit()) {
		if (ontime((struct sockaddr *)&src_addr)) break;
	}
	changemode(0);

	
	close(mysocket);
	if (rxbuff) free(rxbuff);
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


