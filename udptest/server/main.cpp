#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
int keytest(void);


#define UNICAST 1
#define MULTICAST 2
#define BROADCAST 3

//choose the mode
#define CAST 2

// config
#define NETWORK_INTERFACE 0
#define UDP_PORT 1234
//#define UNICAST_ADDR   inet_addr("172.16.50.197");
#define UNICAST_ADDR   inet_addr("10.0.229.220");
#define MULTICAST_ADDR inet_addr("225.0.0.37");
#define MULTICAST_TTL  32;
#define BROADCAST_ADDR inet_addr("172.16.32.255");

// context
char hostname[80];
struct in_addr hostip = {0};
SOCKET mysocket;
SOCKADDR_IN dest_addr;

// UDP HEADER = 8bytes
// unsigned short source port; // 2
// unsigned short dest port;   // 2
// unsigned short lenght;      // 2  --> max = 65535 bytes - 8 bytes = 65527 bytes
// unsigned short checksum;    // 2
#define MAXUDP_PAYLOADSZ 65527 
/* Shouldn't be a good idea to use the max UDB size as if we loose a packet, we loose a lot of data! */

// FRAME HEADER = 8bytes
typedef struct {
    unsigned short framenumber; // 2
    unsigned char  version;     // 1
    unsigned char  frametype;   // 1
    unsigned long  address;     // 4
    unsigned long  size;        // 4
} udp_frame_header_t;

#define TEST_FRAME_ID 1
typedef struct {
    unsigned char keycode;
    unsigned char dummy[1024*10]; 
} udp_test_frame_t;


long long milliseconds_now() {
    static LARGE_INTEGER s_frequency;
    static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
    if (s_use_qpc) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (1000LL * now.QuadPart) / s_frequency.QuadPart;
    } else {
        return GetTickCount();
    }
}


int main(int argc, char** argv)
{
    // init WinSock 
    WORD w = MAKEWORD(2, 2);
    WSADATA wsadata;
    ::WSAStartup(w, &wsadata);

    // get my hostname
    char ac[80];
    if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
        fprintf(stderr, "gethostname error");
        return 1;
    } else {
        fprintf(stderr, "hostname '%s'\n", ac);
    }

    // get network interface ip (for NETWORK_INTERFACE)
    struct hostent *phe = gethostbyname(ac);
    if (phe == 0) {
        fprintf(stderr, "gethostbyname error");
        return 1;
    }
    for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
        struct in_addr tmp;
        memcpy(&tmp, phe->h_addr_list[i], sizeof(struct in_addr));
        fprintf(stderr, "[%i] %s\n", i, inet_ntoa(tmp));
        if (i==NETWORK_INTERFACE) hostip=tmp;
    }
    if (hostip.S_un.S_addr==0) {
        fprintf(stderr, "interface [%i] unfound\n", NETWORK_INTERFACE);
    } else {
         fprintf(stderr, "\n", inet_ntoa(hostip));
         fprintf(stderr, "%s\n", inet_ntoa(hostip));
         #if CAST == BROADCAST
            fprintf(stderr, "BROADCAST\n");
         #elif CAST == UNICAST
            fprintf(stderr, "UNICAST\n");
        #elif  CAST == MULTICAST
            {   
                struct in_addr v;
                v.S_un.S_addr = MULTICAST_ADDR
                fprintf(stderr, "MULTICAST %s\n",inet_ntoa(v));
            }
        #endif
    }

    // create a socket
    char opt = 1;
    mysocket = socket(AF_INET , SOCK_DGRAM, IPPROTO_UDP);
    if (mysocket == -1) {
        fprintf(stderr, "Error in creating socket");
        return 1;
    }

    // socket options
    #if CAST == BROADCAST
        // Allow broadcast
        setsockopt(mysocket, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(char));
    #elif CAST == UNICAST
    #elif  CAST == MULTICAST
        // Set TTL>0 to pass router
        int ttl = MULTICAST_TTL ;
        if (setsockopt(mysocket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl)) <0) {
             ::closesocket(mysocket);
             fprintf(stderr, "Error in IP_MULTICAST_TTL socket option");
            return 1;
        }
        // Use only a specific interface
        if (setsockopt(mysocket, IPPROTO_IP, IP_MULTICAST_IF, (char *)&(hostip.S_un.S_addr), sizeof(unsigned long)) < 0) {
            ::closesocket(mysocket);
             fprintf(stderr, "Error in IP_MULTICAST_IF socket option");
            return 1;
        }
    #endif

    // create destination address
    memset(&dest_addr, 0, sizeof(SOCKADDR_IN));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);
    #if CAST == BROADCAST
        dest_addr.sin_addr.s_addr = BROADCAST_ADDR;
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
        fprintf(stderr, "out of memory\n", udp_packet_sz);
        return 1;
    }

    udp_frame_header_t* pheader = (udp_frame_header_t*)buffer;
    udp_test_frame_t* ptestFrame = (udp_test_frame_t*)((char*)buffer + sizeof(udp_frame_header_t));
    
    pheader->framenumber = 0;
    pheader->version = 100;
    pheader->address = 0; // all
    pheader->frametype = TEST_FRAME_ID;
    pheader->size = sizeof(udp_test_frame_t);


    // Send Broadcast
    int count = 0;
    long long start = milliseconds_now();
    while (1)
    {
        
        Sleep(100);

        CHAR ch = keytest();
        if (ch) printf("\nPress on 0x%02X\n",ch);

        pheader->framenumber++;
        ptestFrame->keycode=ch;

        float elapsed = (float)(milliseconds_now() - start)/1000;
        float totaldata = (float)(pheader->framenumber*udp_packet_sz)/1024.0f;
        printf("Tx: %u frames, %0.0fkB (%0.2fkB/s)\r", pheader->framenumber,  totaldata, totaldata / elapsed);

        int ret = sendto(mysocket, buffer, udp_packet_sz, 0, (sockaddr*)&dest_addr, sizeof(SOCKADDR_IN));
        if (ret != udp_packet_sz) {
            fprintf(stderr, "Error broadcasting to the clients %d", ret);
             ::closesocket(mysocket);
            return 1;
        }

        if (ch==0x0d) {
            printf("\nEND!\n",ch);
            break;
        }
    }
    
    ::closesocket(mysocket);
    if (buffer) free(buffer);
    return 0;
}




int keytest(void)
{
    CHAR ch;
    DWORD dw;
    HANDLE keyboard;
    INPUT_RECORD input;
    keyboard = GetStdHandle(STD_INPUT_HANDLE);
    dw = WaitForSingleObject(keyboard, 0);
    if (dw != WAIT_OBJECT_0)
        return 0;
    dw = 0;
    // Read an input record.
    ReadConsoleInput(keyboard, &input, 1, &dw);
    ch = 0;
    // Process a key down input event.
    if (!(input.EventType == KEY_EVENT
        && input.Event.KeyEvent.bKeyDown))
    {
        return 0;
    }
    // Retrieve the character that was pressed.
    ch = input.Event.KeyEvent.uChar.AsciiChar;
    // Function keys filtration
    if (input.Event.KeyEvent.dwControlKeyState &
        (LEFT_ALT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED |
        RIGHT_CTRL_PRESSED)
        )
        return 0;
    // if( ch == 13 )
    //  ...;  // enter pressed
    return ch;
}