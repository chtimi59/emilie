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
#define UNICAST_ADDR   inet_addr("172.16.32.74");
#define MULTICAST_ADDR inet_addr("225.0.0.37");
#define MULTICAST_TTL  255;
#define BROADCAST_ADDR inet_addr("172.16.32.255");

// context
char hostname[80];
struct in_addr hostip = {0};
SOCKET mysocket;
SOCKADDR_IN dest_addr;

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
            fprintf(stderr, "MULTICAST\n");
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




    // Send Broadcast
    int count = 0;
    while (!keytest()) {
        
        Sleep(1000);
        
        char sbuf[1024] = {0};
        sprintf(sbuf, "Hello %u", count++);
        printf("Send: %s\n", sbuf);

        unsigned int ret = sendto(mysocket, sbuf, strlen(sbuf), 0, (sockaddr*)&dest_addr, sizeof(SOCKADDR_IN));
        if (ret < 0) {
            fprintf(stderr, "Error broadcasting to the clients");
             ::closesocket(mysocket);
            return 1;
        }
        else if (ret < strlen(sbuf)) {
            fprintf(stderr, "fragmented broadcast");
             ::closesocket(mysocket);
            return 1;
        }
    }
    

    ::closesocket(mysocket);
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