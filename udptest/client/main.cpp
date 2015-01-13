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
#define MULTICAST_ADDR inet_addr("225.0.0.37");

// context
SOCKET mysocket;
SOCKADDR_IN src_addr;
char hostname[80];
struct in_addr hostip = {0};




void ontime() {

    fd_set fds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100;

    FD_ZERO(&fds);
    FD_SET(mysocket, &fds);

    int rc = select(sizeof(fds)* 8, &fds, NULL, NULL, &timeout);
    if (rc > 0)
    {
        char rbuf[1024] = {0};
        SOCKADDR_IN clientaddr;
        int len = sizeof(clientaddr);
        if (recvfrom(mysocket, rbuf, 1024, 0, (sockaddr*)&clientaddr, &len) > 0) {
            printf("received: %s\n", rbuf);
        }
    }


}



int main(int argc, char** argv)
{
    // init WinSock 
    WORD w = MAKEWORD(1, 1);
    WSADATA wsadata;
    ::WSAStartup(w, &wsadata);

    // create a socket
    mysocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (mysocket == -1) {
        fprintf(stderr, "Error in creating socket");
        return 1;
    }

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

    // socket options
    #if CAST == BROADCAST
    #elif CAST == UNICAST
    #elif  CAST == MULTICAST
       // Allow to reuse multicast socket by others app
       unsigned int yes=1;
       if (setsockopt(mysocket,SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof(yes)) < 0) {
            ::closesocket(mysocket);
             fprintf(stderr, "Error in SO_REUSEADDR socket option");
       }

       // Request that the kernel join a multicast group */
       struct ip_mreq mreq;
       mreq.imr_multiaddr.s_addr=MULTICAST_ADDR;
       mreq.imr_interface.s_addr=hostip.S_un.S_addr;
       if (setsockopt(mysocket,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mreq,sizeof(mreq)) < 0) {
            ::closesocket(mysocket);
             fprintf(stderr, "Error in IP_ADD_MEMBERSHIP socket option");
       }
    #endif

     // create destination address
    memset(&src_addr, 0, sizeof(SOCKADDR_IN));
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(UDP_PORT);
    src_addr.sin_addr.s_addr = INADDR_ANY;
    #if CAST == BROADCAST
    #elif CAST == UNICAST
    #elif  CAST == MULTICAST
    #endif

       
    // BIND !
    if (bind(mysocket, (SOCKADDR*)&src_addr, sizeof(SOCKADDR_IN)) < 0) {
        fprintf(stderr, "Error binding in the server socket");
         ::closesocket(mysocket);
        return 1;
    }

    
    while (!keytest()) {
        Sleep(500);
        ontime();
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