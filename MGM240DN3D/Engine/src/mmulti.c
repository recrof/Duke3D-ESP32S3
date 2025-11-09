/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */
 #define TEST_MULTIPLAYER 1

#include "maincfg.h"

// oh man...
#if WIN32
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "mmulti.h"
#include "platform.h"


#define BAKSIZ 1024 //16384     // next-hack: 1024 is more than enough :)
#define SIMULATEERRORS 0
#define SHOWSENDPACKETS 0
#define SHOWGETPACKETS 0
#define PRINTERRORS 0

#define updatecrc16(crc,dat) crc = (((crc<<8)&65535)^crctable[((((unsigned short)crc)>>8)&65535)^dat])
#if WIN32
#include "errno.h"
#define Error(t, f,...) Error2(t, __FILE__, __FUNCTION__, __LINE__,  f, ##__VA_ARGS__)
void   Error2 (int errorType, const char *file, const char *function, int line, const char  *error, ...);
#endif

short numplayers = 0, myconnectindex = 0;
short connecthead, connectpoint2[MAXPLAYERS];
char syncstate = 0;


static long incnt[MAXPLAYERS], outcntplc[MAXPLAYERS], outcntend[MAXPLAYERS];
static char errorgotnum[MAXPLAYERS];
static char errorfixnum[MAXPLAYERS];
static char errorresendnum[MAXPLAYERS];
#if (PRINTERRORS)
    static char lasterrorgotnum[MAXPLAYERS];
#endif

extern long crctable[256];
int tmpmax[8]; //addfaz variable addition (you could probs think of something better)
int itmp = 0; //addfaz router fix STUN

static char lastpacket[576], inlastpacket = 0;
static short lastpacketfrom, lastpacketleng;

extern volatile int32_t totalclock;  /* MUST EXTERN 1 ANNOYING VARIABLE FROM GAME */
static long timeoutcount = 60, resendagaincount = 4, lastsendtime[MAXPLAYERS];

extern unsigned short g_bStun;
#define MAX_PTR         128     // shall be a power of 2

static short bakpacketptr[MAXPLAYERS][MAX_PTR], bakpacketlen[MAXPLAYERS][MAX_PTR];
static char bakpacketbuf[BAKSIZ];
static long bakpacketplc = 0;

extern short myconnectindex, numplayers;
extern short connecthead, connectpoint2[MAXPLAYERS];
extern char syncstate;// = 0;

extern int32_t _argc;
extern char **_argv;

#define MAXPACKETSIZE 100 // next-hack: was 2048.
typedef struct
{
    short intnum;                /* communication between Game and the driver */
    short command;               /* 1-send, 2-get */
    short other;                 /* dest for send, set by get (-1 = no packet) */
    short numbytes;
    short myconnectindex;
    short numplayers;
    short gametype;              /* gametype: 1-serial,2-modem,3-net */
    short filler;
    unsigned char buffer[MAXPACKETSIZE];
    long longcalladdress;
} gcomtype;
static gcomtype *gcom;
static gcomtype gcomstruct;
#define MMULTI_FILE 1
#include "duke3d.h"
/*
//
// Packet queue data structs
//
typedef struct packet_wrapper
{
    unsigned long packetnumber;
    gcomtype packet;
} PACKETWRAPPER;

typedef PACKETWRAPPER packet_buffer;

unsigned long currentpacketnumber[MAXPLAYERS];

//
// Packet Buffer
//
packet_buffer pBuff[256];
*/


/*
typedef struct
{
    unsigned char buffer[MAXPACKETSIZE];
}PACKET;
*/

//typedef std::vector<PACKET> PacketQueue;

enum ECommitCMDs
{
    COMMIT_CMD_SEND             = 1,
    COMMIT_CMD_GET              = 2,
    COMMIT_CMD_SENDTOALL        = 3,
    COMMIT_CMD_SENDTOALLOTHERS  = 4,
    COMMIT_CMD_SCORE            = 5,
};
#if WIN32
void sendpacket(int32_t other, const uint8_t *bufptr, int32_t messleng);
#define USE_RCRC                0
#else
#define USE_RCRC                1
#endif
// Queue of out going packets.
//PacketQueue outgoingPacketQueue;
//outgoingPacketQueue.reserve(128);
#if WIN32
static gcomtype *init_network_transport(char **ARGV, int argpos);
#else
static gcomtype *init_network_transport();
#endif
static void deinit_network_transport(gcomtype *gcom);
static void dosendpackets(long other);
static void callcommit(void);
void broadcastPacket(uint8_t * packet, int length, int notme)
{
    int i;
    #if WIN32
    for(i=connecthead;i>=0;i=connectpoint2[i])
    {
        if( i != myconnectindex || !notme)
        {
            sendpacket(i,packet, length);
        }
    }
    #endif
}
#if USE_CRC
static void initcrc(void)
{
    long i, j, k, a;

    for(j=0;j<256;j++)      /* Calculate CRC table */
    {
        k = (j<<8); a = 0;
        for(i=7;i>=0;i--)
        {
            if (((k^a)&0x8000) > 0)
                a = ((a<<1)&65535) ^ 0x1021;   /* 0x1021 = genpoly */
            else
                a = ((a<<1)&65535);
            k = ((k<<1)&65535);
        }
        crctable[j] = (a&65535);
    }
}


static int32_t getcrc(uint8_t *buffer, short bufleng)
{
    int32_t i, j;

    j = 0;
    for(i=bufleng-1;i>=0;i--) updatecrc16(j,buffer[i]);
    return(j&65535);
}
#endif
void initmultiplayers()
{
    long i;
#if USE_CRC
    initcrc();
#endif
    for(i=0;i<MAXPLAYERS;i++)
    {
        incnt[i] = 0L;
        outcntplc[i] = 0L;
        outcntend[i] = 0L;
        bakpacketlen[i][MAX_PTR - 1] = -1;
    }

    // clear out the packet ordering
//  memset(&currentpacketnumber, 0, sizeof(unsigned long) * MAXPLAYERS);
#if WIN32
    for (i = _argc - 1; i > 0; i--)
    {
        const char *arg = _argv[i];
        char ch = *arg;
        if ((ch == '-') || (ch == '/'))
        {
            if (stricmp(arg + 1, "net") == 0)
                break;
        }
    }
#if ORIGINAL_MULTIPLAYER
    if ((i == 0) || (i+1 == _argc))
    {
        numplayers = 1; myconnectindex = 0;
        connecthead = 0; connectpoint2[0] = -1;
        return;
    }
#endif
    gcom = init_network_transport(_argv, i+1);
    if (gcom == NULL)
    {
        Error(EXIT_SUCCESS, "Network transport initialization failed!\n"
                            "Be sure you have closed *all* the previous instances of\n"
                            "the xDuke and that no other application is using the\n"
                            "same port# as Duke. If you can't solve this problem, try\n"
                            "changing the port# or reboot.\n");
    }

    numplayers = gcom->numplayers;
    int oldmyconnectindex = myconnectindex;
    myconnectindex = gcom->myconnectindex-1;
    if (myconnectindex != oldmyconnectindex)
        memcpy(&ps[myconnectindex], &ps[oldmyconnectindex], sizeof(ps[0]));
#if (SIMULATEERRORS != 0)
    srand(myconnectindex*24572457+345356);
#endif
#else
    gcom = init_network_transport();
    numplayers = gcom->numplayers;
    int oldmyconnectindex = myconnectindex;
    myconnectindex = gcom->myconnectindex-1;
    if (myconnectindex != oldmyconnectindex)
        memcpy(&ps[myconnectindex], &ps[oldmyconnectindex], sizeof(ps[0]));
#endif

    connecthead = 0;
    for(i=0;i<numplayers-1;i++)
    {
        connectpoint2[i] = i+1;
    }
    connectpoint2[numplayers-1] = -1;

    for(i=0;i<numplayers;i++)
    {
        lastsendtime[i] = totalclock;
    }
}


static void dosendpackets(long other)
{
    long i, j, k, messleng;
    unsigned short dacrc;

    if (outcntplc[other] == outcntend[other]) return;

#if (PRINTERRORS)
    if (errorgotnum[other] > lasterrorgotnum[other])
    {
        lasterrorgotnum[other]++;
        printf(" MeWant %ld",incnt[other]&255);
    }
#endif
/*
    buffer[0] = outbound message counter
    buffer[1] = (IS_DOUBLE_PACKET: bit 7) | ERROR_RESEND[0:2] (bits 3-6) | ERROR_GOT[0:2] (bits 0-2)
    buffer[2] = counter of last received packet.
*/
    // next-hack: added check for too big dual-packets
    int maxMsgLen = bakpacketlen[other][outcntplc[other]&(MAX_PTR - 1)];
    maxMsgLen +=  bakpacketlen[other][(outcntplc[other] + 1)&(MAX_PTR - 1)];

    if (outcntplc[other]+1 == outcntend[other] || (maxMsgLen > MAXPACKETSIZE))
    {     /* Send 1 sub-packet */
        k = 0;
        gcom->buffer[k++] = (outcntplc[other]&(MAX_PTR - 1));     // message counter of this message
        gcom->buffer[k++] = (errorgotnum[other]&7)+((errorresendnum[other]&7)<<3);   //error information + packet information
        gcom->buffer[k++] = (incnt[other] & (MAX_PTR - 1));         // counter of received packets
        // get position on circular buffer
        j = bakpacketptr[other][outcntplc[other]&(MAX_PTR - 1)];
        messleng = bakpacketlen[other][outcntplc[other]&(MAX_PTR - 1)];
        for(i=0;i<messleng;i++)
            gcom->buffer[k++] = bakpacketbuf[(i+j)&(BAKSIZ-1)];
        outcntplc[other]++;
    }
    else
    {     /* Send 2 sub-packets */
        k = 0;
        gcom->buffer[k++] = (outcntplc[other]&(MAX_PTR - 1));
        gcom->buffer[k++] = (errorgotnum[other]&7)+((errorresendnum[other]&7)<<3)+128;
        gcom->buffer[k++] = (incnt[other]&(MAX_PTR - 1));

            /* First half-packet */
        j = bakpacketptr[other][outcntplc[other]&(MAX_PTR - 1)];
        messleng = bakpacketlen[other][outcntplc[other]&(MAX_PTR - 1)];
        gcom->buffer[k++] = (char)(messleng&255);
        gcom->buffer[k++] = (char)(messleng>>8);
        for(i=0;i<messleng;i++)
            gcom->buffer[k++] = bakpacketbuf[(i+j)&(BAKSIZ-1)];
        outcntplc[other]++;

            /* Second half-packet */
        j = bakpacketptr[other][outcntplc[other]&(MAX_PTR - 1)];
        messleng = bakpacketlen[other][outcntplc[other]&(MAX_PTR - 1)];
        for(i=0;i<messleng;i++)
            gcom->buffer[k++] = bakpacketbuf[(i+j)&(BAKSIZ-1)];
        outcntplc[other]++;

    }
#if USE_RCRC
    dacrc = radioComputeCRC32(gcom->buffer,(short)k);
#else
    dacrc = 0;
#endif
    gcom->buffer[k++] = (dacrc&255);
    gcom->buffer[k++] = (dacrc>>8);
    // other is 0-3, but in gcom->other is added to 1, because 0 is not valid.
    gcom->other = other+1;
    gcom->numbytes = k;

#if (SHOWSENDPACKETS)
    printf("Send(%ld): ",gcom->other);
    for(i=0;i<gcom->numbytes;i++) printf("0x%02x ",255 & gcom->buffer[i]);
    printf("\n");
#endif

#if (SIMULATEERRORS != 0)
    if (!(rand()&SIMULATEERRORS)) gcom->buffer[rand()%gcom->numbytes] = (rand()&255);
    if (rand()&SIMULATEERRORS)
#endif
        {
            gcom->command = COMMIT_CMD_SEND;
            callcommit();
        }
}

//next-hack: just added some comments.
void sendpacket(int32_t other, const uint8_t *bufptr, int32_t messleng)
{
    long i = 0;
    long j = 0;

    if (numplayers < 2) return;
    // check if the message is the same as last packet.
    i = 0;
    if (bakpacketlen[other][(outcntend[other]-1)&(MAX_PTR - 1)] == messleng)
    {
        j = bakpacketptr[other][(outcntend[other]-1)&(MAX_PTR - 1)];
        for(i=messleng-1;i>=0;i--)
            if (bakpacketbuf[(i+j)&(BAKSIZ-1)] != bufptr[i]) break;
    }
    bakpacketlen[other][outcntend[other]&(MAX_PTR - 1)] = messleng;
    // if the message is the same, then just use the pointer.
    if (i < 0)   /* Point to last packet to save space on bakpacketbuf */
        bakpacketptr[other][outcntend[other]&(MAX_PTR - 1)] = j;
    else
    {
        // queue the message to the circular buffer
        bakpacketptr[other][outcntend[other]&(MAX_PTR - 1)] = bakpacketplc;
        for(i=0;i<messleng;i++)
            bakpacketbuf[(bakpacketplc+i)&(BAKSIZ-1)] = bufptr[i];
        bakpacketplc = ((bakpacketplc+messleng)&(BAKSIZ-1));
    }
    // increase the number of packets being sent to the "other"
    outcntend[other]++;

    lastsendtime[other] = totalclock;
    dosendpackets(other);
}


void setpackettimeout(int32_t datimeoutcount, int32_t daresendagaincount)
{
    // Don't do this it keeps '/f4' from working
    // Though /f4 feels weird on my mouse.... slugish is the word...
    /*
    long i;

    timeoutcount = datimeoutcount;
    resendagaincount = daresendagaincount;
    for(i=0;i<numplayers;i++) lastsendtime[i] = totalclock;
    */
}


void uninitmultiplayers(void)
{
    deinit_network_transport(gcom);
    gcom = NULL;
}

void sendlogon(void)
{
}

void sendlogoff(void)
{
	uint8_t tempbuf[2];

    tempbuf[0] = 255;
    tempbuf[1] = myconnectindex;
//    for(i=connecthead;i>=0;i=connectpoint2[i])
//        if (i != myconnectindex)
//            sendpacket(i,tempbuf,2L);
    broadcastPacket(tempbuf, 2, true);

}

int32_t getoutputcirclesize(void)
{
    return(0);
}

short getpacket (short *other, uint8_t *bufptr)
{
    long i, messleng;
    unsigned short dacrc;

    if (numplayers < 2) return(0);

    for(i=connecthead;i>=0;i=connectpoint2[i])
    {
        if (i != myconnectindex)
        {
            // fix if the lastsendtime is smaller than totalclock (is this for wrap-around handling?)
            if (totalclock < lastsendtime[i])
                lastsendtime[i] = totalclock;
            // is it a long time since we are not sending packets?
            if (totalclock > lastsendtime[i]+timeoutcount)
            {
#if (PRINTERRORS)
                    printf(" TimeOut!");
#endif
                    // outcntend is the number of sent packets.
                    // outcntplc is the current packet number being sent.
                    // This handles the case we are retransmitting last packet
                    if ((outcntplc[i] == outcntend[i]) && (outcntplc[i] > 0))
                    {
                        // get previous packet, send again.
                        // if we lost just this one, we are not really in a hurry
                        outcntplc[i]--;
                        lastsendtime[i] = totalclock;
                    }
                    else
                    {
                        // in this case either outcntplc is negative (Yes it will happen. after 2 BILLIONS packets...)
                        // or, more likely this is not the last packet to be sent. We want to send as quickly as possible
                        // all the missing packets, so don't set the lastsendtime as totalclock, just increase by a little.
                        lastsendtime[i] += resendagaincount; //+=4
                    }
// note: we send the 3 lsbs of errorgotnum and errorresendnum
// WFT are these?
                    errorgotnum[i] = errorfixnum[i]+1;

                    dosendpackets(i);

            }
            }
        }

    if (inlastpacket != 0)
    {
            /* 2ND half of good double-packet */
        inlastpacket = 0;
        *other = lastpacketfrom;
        memcpy(bufptr,lastpacket,lastpacketleng);
        return(lastpacketleng);
    }

    gcom->command = COMMIT_CMD_GET;
    callcommit();

#if (SHOWGETPACKETS)
    if (gcom->other != -1)
    {
        printf(" Get(%ld): ",gcom->other);
        for(i=0;i<gcom->numbytes;i++) printf("%2x ",gcom->buffer[i]);
        printf("\n");
    }
#endif

    if (gcom->other < 0)
    {
        // If we did not get any data, other is -1. If we got some, other is 1 through MAXPLAYERS
        return(0);
    }
    // set the index of the player we got from. *other will be 0 to MAXPLAYERS-1
    *other = gcom->other-1;

    messleng = gcom->numbytes;

    dacrc = ((unsigned short)gcom->buffer[messleng-2]);
    dacrc += (((unsigned short)gcom->buffer[messleng-1])<<8);
#if USE_RCRC
    if (0 != (0xFFFF) & (radioComputeCRC32(gcom->buffer,messleng) >> 16 ))        /* CRC check */
    {
#if (PRINTERRORS)
        printf("\n%ld CRC",gcom->buffer[0]);
#endif
        printf("crc 0x%04x computed 0x%08x\r\n", dacrc, radioComputeCRC32(gcom->buffer,(short)(messleng)));
        // We received garbage, so return 0. We must signal this.
        errorgotnum[*other] = errorfixnum[*other]+1;
        return(0);
    }
#endif
    // gcom->buffer[1] contains (see dosendpackets):  (errorgotnum[other]&7)+((errorresendnum[other]&7)<<3);
    // Therefore the cycle below will make sure the 3 lsb of (LOCAL) errorfixnum are the same as
    //  the 3 lsb of errorresendnum (REMOTE)
    // note: this will increment errorfixnum[*other] & 8 as well, if errorfixnum & 7 was higher.
    while ((errorfixnum[*other]&7) != ((gcom->buffer[1]>>3)&7))
        errorfixnum[*other]++;
    // if the errorgotnum (remote) is different from the erroresendnum (local)
    if ((gcom->buffer[1]&7) != (errorresendnum[*other]&7))
    {
        // increase resend count
        errorresendnum[*other]++;
        // buffer[2] contains the packet number of last received packet
        // the remote is asking us to resend the packet
        outcntplc[*other] = (outcntend[*other] & ~(MAX_PTR - 1))+gcom->buffer[2];
        // wrap if greater.
        if (outcntplc[*other] > outcntend[*other]) outcntplc[*other] -= MAX_PTR;
    }
    // Buffer[0] contains the outcntplc (REMOTE).
    // are we in sync?
    if (gcom->buffer[0] != (incnt[*other]&(MAX_PTR - 1)))   /* CNT check */
    {
        // incnt is how many packets we have received. buffer[0] is how many these were sent
        // are we off more than 32 ? (note this can be either we have received 32 more or we have
        // lost MAX_PTR - 32 (most probably). So we are returning 0
        // this will make the program stall (returning 0), and asking to resend again
        if (((incnt[*other]-gcom->buffer[0])&(MAX_PTR - 1)) > 32)
        {
            errorgotnum[*other] = errorfixnum[*other]+1;
#if (PRINTERRORS)
            printf("\n%ld CNT",gcom->buffer[0]);
#endif
        }
#if (PRINTERRORS)
        else
        {
            if (!(gcom->buffer[1]&128))           /* single else double packet */
                printf("\n%ld cnt single ",gcom->buffer[0]);
            else
            {
                if (((gcom->buffer[0]+1)&(MAX_PTR - 1)) == (incnt[*other]&(MAX_PTR - 1)))
                {
                                 /* GOOD! Take second half of double packet */
#if (PRINTERRORS)
                    printf("\n%ld-%ld . ",gcom->buffer[0],(gcom->buffer[0]+1)&(MAX_PTR - 1));
#endif
                    messleng = ((long)gcom->buffer[3]) + (((long)gcom->buffer[4])<<8);
                    lastpacketleng = gcom->numbytes-7-messleng;
                    memcpy(bufptr,&gcom->buffer[messleng+5],lastpacketleng);
                    incnt[*other]++;
                    return(lastpacketleng);
                }
                else
                    printf("\n%ld-%ld cntb ",gcom->buffer[0],(gcom->buffer[0]+1)&(MAX_PTR - 1));
            }
        }
#endif
        return(0);
    }

        /* PACKET WAS GOOD! */
    if ((gcom->buffer[1]&128) == 0)           /* Single packet */
    {
#if (PRINTERRORS)
        printf("\n%ld  cmd  ",gcom->buffer[0]);
#endif

        messleng = gcom->numbytes-5;

        memcpy(bufptr,&gcom->buffer[3],messleng);

        incnt[*other]++;
        return(messleng);
    }

                                                         /* Double packet */
#if (PRINTERRORS)
    printf("\n%ld-%ld cmds ",gcom->buffer[0],(gcom->buffer[0]+1)&(MAX_PTR - 1));
#endif

    messleng = ((long)gcom->buffer[3]) + (((long)gcom->buffer[4])<<8);
    lastpacketleng = gcom->numbytes-7-messleng;
    inlastpacket = 1; lastpacketfrom = *other;

    memcpy(bufptr,&gcom->buffer[5],messleng);
    memcpy(lastpacket,&gcom->buffer[messleng+5],lastpacketleng);

    incnt[*other] += 2;
    return(messleng);
}

void flushpackets()
{
#if 1
    long i;

    if (numplayers < 2) return;

    do
    {
        gcom->command = COMMIT_CMD_GET;
        callcommit();
    } while (gcom->other >= 0);

    for(i=connecthead;i>=0;i=connectpoint2[i])
    {
        incnt[i] = 0L;
        outcntplc[i] = 0L;
        outcntend[i] = 0L;
        errorgotnum[i] = 0;
        errorfixnum[i] = 0;
        errorresendnum[i] = 0;
        lastsendtime[i] = totalclock;
    }
#endif
}

void genericmultifunction(int32_t other, const uint8_t *bufptr, int32_t messleng, int32_t command)
{
    if (numplayers < 2) return;

    gcom->command = command;
    gcom->numbytes = min(messleng,MAXPACKETSIZE);
    memcpy(gcom->buffer,bufptr,gcom->numbytes);
    gcom->other = other+1;
    callcommit();

}

#if WIN32
#  include <winsock.h>
//#  define EAGAIN WSAEWOULDBLOCK
//#  define EWOULDBLOCK WSAEWOULDBLOCK
//#  define ECONNREFUSED WSAECONNRESET
#  define socklen_t size_t
#  define netstrerror() win32netstrerror()
#  define neterrno() WSAGetLastError()
#  define sockettype SOCKET
#  define socketclose(x) closesocket(x)
#endif

#define SOCKET_SHUTDOWN_BOTH 2

#include <signal.h>
#include "filesystem.h"  /* kopen4load for cfg file. */
#include "display.h"  /* getticks */

#define IPSEG1(ip) ((((unsigned int) ip) & 0xFF000000) >> 24)
#define IPSEG2(ip) ((((unsigned int) ip) & 0x00FF0000) >> 16)
#define IPSEG3(ip) ((((unsigned int) ip) & 0x0000FF00) >>  8)
#define IPSEG4(ip) ((((unsigned int) ip) & 0x000000FF)      )

#define BUILD_DEFAULT_UDP_PORT 1635  /* eh...why not? */
#define CLIENT_POLL_DELAY 100  /* ms between pings at peer-to-peer startup. next-hack: was 3000 */
#define HEADER_PEER_GREETING 245
#if WIN32
static sockettype udpsocket = -1;
static short udpport = BUILD_DEFAULT_UDP_PORT;
static struct {
  int host;
  short port;
} allowed_addresses[MAXPLAYERS+1];  /* only respond to these IPs. */
#else
static struct
{
  uint16_t host;
} allowed_addresses[MAXPLAYERS+1];  /* only respond to these IPs. */

#endif
volatile uint8_t ctrlc_pressed = 0;
#if WIN32
static void siginthandler(int sigint)
{
    ctrlc_pressed = 1;
}

/*
 * Figure out what the last failing Win32 API call was, and
 *  generate a human-readable string for the error message.
 *
 * The return value is a static buffer that is overwritten with
 *  each call to this function.
 *
 * Code lifted from PhysicsFS: http://icculus.org/physfs/
 */
static const char *win32netstrerror(void)
{

    static TCHAR msgbuf[255];
    TCHAR *ptr = msgbuf;

    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        WSAGetLastError(), /*GetLastError(),*/
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
        msgbuf,
        sizeof (msgbuf) / sizeof (TCHAR),
        NULL
    );

        /* chop off newlines. */
    for (ptr = msgbuf; *ptr; ptr++)
    {
        if ((*ptr == '\n') || (*ptr == '\r'))
        {
            *ptr = ' ';
            break;
        } /* if */
    } /* for */

    return((const char *) msgbuf);
} /* win32strerror */

typedef enum
{
    udpmode_peer,
    udpmode_server,
    udpmode_client
} udpmodes;
static udpmodes udpmode = udpmode_peer;

static char *static_ipstring(int ip)
{
    static char s[16];
    sprintf(s, "%u.%u.%u.%u", IPSEG1(ip), IPSEG2(ip), IPSEG3(ip), IPSEG4(ip));
    return(s);
}


static int send_udp_packet(int ip, short port, void *pkt, size_t pktsize)
{

    /* !!! FIXME: See if this would ever block. */
    /* !!! FIXME: See if this would send a partial packet. */
    struct sockaddr_in addr;
    int rc;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    addr.sin_port = htons(port);

    rc = sendto(udpsocket, pkt, pktsize, 0,
                (struct sockaddr *) &addr, sizeof (addr));

    if (rc != (int) pktsize)
    {
        printf("sendto err rc==%d when sending %d to %s:%d [%s].\n",
                rc, (int) pktsize, static_ipstring(ip), (int) port,
                netstrerror());
        return(0);
    }

/*printf("Sent %d byte packet to %s:%d\n", (int) pktsize, static_ipstring(ip), (int) port);*/
    return(1);
}


static void process_udp_send_queue(void)
{
    /* Fill this in if the non-blocking socket becomes an issue. */
}


static int get_udp_packet(int *ip, short *_port, void *pkt, size_t pktsize)
{

    int err = 0;
    struct sockaddr_in addr;
    short port;
    socklen_t fromlen = sizeof (addr);
    int valid = 0;
    int i;

    /* FIXME: Will this ever receive a partial packet? */
    int rc = recvfrom(udpsocket, pkt, pktsize, 0,
                      (struct sockaddr *) &addr, (int *)&fromlen);

    if (rc == -1)
        err = neterrno();

#if !WIN32
    /* !!! FIXME: Linux specific? */
    if (rc == -1)  /* fill in the addr structure on error... */
    {
        socklen_t l = sizeof (addr);
        recvfrom(udpsocket, NULL, 0, MSG_ERRQUEUE,
                 (struct sockaddr *) &addr, &l);
    }
#endif

    *ip = ntohl(addr.sin_addr.s_addr);
    port = ntohs(addr.sin_port);
    if (_port)
        *_port = port;

    /*
     * Reject packets from unallowed IPs. Prevents (ha) DoS attacks and
     *  other confusion...
     */
    if (gcom == NULL)
    {
        if(g_bStun) //if stun is enabled
        {
            if ((allowed_addresses[itmp].host == *ip)
            && (allowed_addresses[itmp].port == port))
            {
                valid = 1; //only accept packets from the current player (itmp) and no one else
            }
            else
            {
                return 0; //reject packets from other players
            }
        }
        else
        {
            valid = 1;
        }
    }
    else
    {
        for (i = 1; i <= gcom->numplayers; i++)
        {
            if ((allowed_addresses[i].host == *ip)
                && (allowed_addresses[i].port == port))

            {
                valid = i;

                //printf ("Packet received from %s:%d\n",static_ipstring(*ip), (int) port);

                break;
            }

        }
    }

    /*if (!valid)
    {

        static int unallowed_ip_spam = 0;
        if (unallowed_ip_spam <= 100)
        {
            printf("Packet received from unallowed IP %s:%d\n",
                    static_ipstring(*ip), (int) port);
            if (unallowed_ip_spam == 100)
                printf("(Disabling further unallowed IP spam.)\n");
            unallowed_ip_spam++;
        }

        return(0);
    } */

    if (rc == -1)
    {
        if ((err == EAGAIN) || (err == EWOULDBLOCK))
            rc = 0;

        else if (err == ECONNREFUSED)   //"connection reset by peer" in winsock
        {

             //  This means that we sent a packet to an unopened port, and
             //  it responded by telling us to piss off. Take them out of the
             //  allowed list. We check gcom so that we don't worry about this
             //  during detection when game might still be loading elsewhere.

            if (gcom != NULL)
            {
                allowed_addresses[valid].host = 0;
                printf("%s:%d refused packets. He either crashed or quited abruptly. Please restart.\n",
                        static_ipstring(*ip), (int) port);
            }
            // !!! FIXME: Actually boot player, too.
        }

        else
        {
            printf("recvfrom err rc==%d when getting %d from %s:%d [%s].\n",
                    rc, (int) pktsize, static_ipstring(*ip), (int) port,
                    netstrerror());
        }
    }
//else printf("Got %d byte packet from %s:%d\n", (int) rc, static_ipstring(*ip), (int) port);
    //printf( "IP from client %d", *ip);
    return(rc);
}

static char *get_token(char **ptr)
{
        #if NOT_YET_IMPLEMENTED || 1

    char *retval;
    char *p = *ptr;
    if (*p == '\0')
        return(NULL);

    while ((*p != '\0') && (isspace(*p)))
        p++;

    if (*p == '\0')  /* nothing but whitespace. */
        return(NULL);

    retval = p;
    while ((*p != '\0') && (!isspace(*p)))
        p++;

    *p = '\0';
    *ptr = p + 1;

    /*printf("Got token [%s].\n", retval);*/
    return(retval);
    #else
    return NULL;
    #endif
}
#endif

static int set_socket_blockmode(int onOrOff)
{

    unsigned long flags;
    int rc = 0;
    #if NOT_YET_IMPLEMENTED || WIN32

    /* set socket to be (non-)blocking. */

#if WIN32
    flags = (onOrOff) ? 0 : 1;
    rc = (ioctlsocket(udpsocket, FIONBIO, &flags) == 0);
#else
    flags = fcntl(udpsocket, F_GETFL, 0);
    if (flags != -1)
    {
        if (onOrOff)
            flags &= ~O_NONBLOCK;
        else
            flags |= O_NONBLOCK;
        rc = (fcntl(udpsocket, F_SETFL, flags) == 0);
    }
#endif

    if (!rc)
    {
        printf("set socket %sblocking failed: %s\n",
            ((onOrOff) ? "" : "non-"), netstrerror());
    }
#endif
    return(rc);
}


static int set_socket_broadcast(int onOrOff)
{
        #if NOT_YET_IMPLEMENTED || WIN32

    int f = (onOrOff) ? 1 : 0;
    int rc;

    /* give socket clearance to broadcast. */
    rc = setsockopt(udpsocket, SOL_SOCKET, SO_BROADCAST, (char *)(&f), sizeof (f)) == 0;
    if (!rc)
    {
        printf("%sset SO_BROADCAST failed: %s\n",
            ((onOrOff) ? "" : "un"), netstrerror());
    }
    return rc;
    #else
    return 0;

#endif
}

#if WIN32
static int open_udp_socket(int ip, int port)
{

    struct sockaddr_in addr;

    printf("Setting up UDP interface %s:%d...\n", static_ipstring(ip), port);
    printf("Stun is currently %s\n", (g_bStun) ? "Enabled":"Disabled");

    udpsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

#if WIN32
    if (udpsocket == INVALID_SOCKET)
#else
    if (udpsocket == -1)
#endif
    {
        printf("socket creation failed: %s\n", netstrerror());
        return(0);
    }

    if (!set_socket_blockmode(0))
        return(0);

    #if !WIN32 && !__APPLE__
    {
        /* !!! FIXME: Might be Linux (not Unix, not BSD, not WinSock) specific. */
        int flags = 1;
        setsockopt(udpsocket, SOL_IP, IP_RECVERR, &flags, sizeof (flags));
    }
    #endif

    memset(&addr, '\0', sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    addr.sin_port = htons((unsigned short)port);
    if (bind(udpsocket, (struct sockaddr *) &addr, sizeof (addr)) == -1)
    {
        printf("socket binding failed: %s\n", netstrerror());
        return(0);
    }
    return 1;
}
#endif
/* server init. */
static int wait_for_other_players(gcomtype *gcom, int myip)
{
    printf("Server code NOT implemented!\n");
    return(0);
}

/* client init. */
static int connect_to_server(gcomtype *gcom, int myip)
{
    printf("Client code NOT implemented!\n");
    return(0);
}

typedef struct
{
    unsigned char dummy1;   /* so these don't confuse game after load. */
    unsigned char dummy2;   /* so these don't confuse game after load. */
    unsigned char dummy3;   /* so these don't confuse game after load. */
    unsigned char header;   /* always HEADER_PEER_GREETING (245). */
    unsigned short id;
    uint8_t name[MAX_NAME_LEN];
} PacketPeerGreeting;

#if WIN32
static void send_peer_greeting(int ip, short port, short myid)
{
    PacketPeerGreeting packet;
    memset(&packet, '\0', sizeof (packet));
    packet.header = HEADER_PEER_GREETING;
    packet.id = BUILDSWAP_INTEL16(myid);
    memcpy (packet.name, myname, sizeof(packet.name));
    packet.name[sizeof (packet.name) - 1] = 0;   // force null terminator
    send_udp_packet(ip, port, &packet, sizeof (packet));
}

/* peer to peer init. */
static int connect_to_everyone(gcomtype *gcom, int myip, int bcast)
{
    PacketPeerGreeting packet;
    unsigned short my_id = 0;
    int i;
    int rc;
    int ip;
    short port;
    int first_send = 1;
    unsigned short heard_from[MAXPLAYERS];
    unsigned long resendat;
    int max;
    int remaining;

    printf("peer-to-peer init. CTRL-C to abort...\n");

    if (bcast)
    {
        if (gcom->numplayers > 1)
        {
            printf("ERROR: Can't do both 'broadcast' and 'allow'.\n");
            return(0);
        }

        if (!set_socket_broadcast(1))
            return(0);

        gcom->numplayers = bcast + 1;
    }

    memset(heard_from, '\0', sizeof (heard_from));
    //
     srand(time(NULL));
    //
    while (my_id == 0 || my_id == 0xFFFF)  /* player number is based on id, low to high. */
        my_id = (unsigned short) rand();

    printf("(This client's ID for this round is 0x%X.)\n\n", my_id);

    resendat = getticks();
    remaining = max = gcom->numplayers - 1;

    printf("Waiting for %d player%s...\n", remaining, remaining==1 ? "":"s");
    if (remaining == 0)
    {
        printf("Hmmm...don't have time to play with myself.\n");
        return(0);
    }

    while ((remaining) && (!ctrlc_pressed))
    {
        if (resendat <= getticks())
        {
            if (bcast)
            {
                printf("%sroadcasting greeting...\n", first_send ? "B":"Reb");
                /* !!! FIXME: This is...probably not right. */
                send_peer_greeting(0xFFFFFFFF, udpport, my_id);
            }
            else
            {
                for (i = 0; (i < max) || g_bStun ; i++)
                {

                    //only checking one player at a time works
                    //this is where special formatting of allow lines comes in
                    if(g_bStun)
                    {
                        i = itmp; //addfaz router fix
                    }

                    if (!heard_from[i])
                    {
                       printf("%sending greeting to %s:%d...\n",
                                first_send ? "S" : "Res",
                                static_ipstring(allowed_addresses[i].host),
                                allowed_addresses[i].port);

                       send_peer_greeting(allowed_addresses[i].host,
                                           allowed_addresses[i].port,
                                           my_id);
                    }

                    // If this is stun-enabled then don't loop.
                    if(g_bStun)
                    {
                        break;
                    }
                }
            }
            first_send = 0;
            resendat += CLIENT_POLL_DELAY;
        }

        _idle();
        process_udp_send_queue();

        rc = get_udp_packet(&ip, &port, &packet, sizeof (packet));

        //this is so we don't get unexpected packet errors from players already heard from

        if(g_bStun)
        {
            //addfaz router/stun addition *Start*
            //this is so we are not dealing with players already heard from.
            for (i = 0; i < max; i++)
            {
                if (ip == allowed_addresses[i].host &&
                port == allowed_addresses[i].port)
                {
                    if (heard_from[i] != 0) //if we've heard from player already.
                    {
                        rc = 0;
                    }
                }
            }
            //addfaz router/stun addition *End*
        }


        if ( (rc > 0) && (ip) && ((ip != myip) || (port != udpport)) )
        {
            char *ipstr = static_ipstring(ip);

            for (i = 0; i < max; i++)
            {

                ////addfaz NAT addition *START*////
                if(!g_bStun)
                {
                    if(tmpmax[i] != 1)
                    {
                        if (allowed_addresses[i].host == ip)
                        {
                            if(allowed_addresses[i].port != port)
                            {
                                printf("Different player Port Number detected. %s:%i\n",ipstr,
                                allowed_addresses[i].port);
                                printf("Changed to %s:%i, player may be behind a firewall.\n", ipstr, port); //addfaz NAT addition
                                allowed_addresses[i].port = port;
                            }
                        }
                    }
                }
                ////addfaz NAT addition *END*////

                if ((ip == allowed_addresses[i].host) &&
                    (port == allowed_addresses[i].port))  //addfaz NAT line addition
                {
                    break;
                }

                if ((bcast) && (allowed_addresses[i].host == 0))
                    break;  /* select this slot. */

            }

            if (i == max)
                printf("%s:%d is not an allowed player.\n", ipstr, port);

            else if (rc != sizeof (packet))
                printf("Missized packet/packet fragment from %s:%i\n", ipstr, port);

            else if (packet.header != HEADER_PEER_GREETING)
                printf("Unexpected packet type from %s:%i\n", ipstr, port);

            else if (heard_from[i] == 0)
            {
                packet.id = BUILDSWAP_INTEL16(packet.id);
                heard_from[i] = packet.id;
                allowed_addresses[i].host = ip;   /* bcast needs this. */
                allowed_addresses[i].port = port;
                remaining--;
                menuEnableMultiplayerReady(true);   // at least 2 players

                printf("Heard from %s:%i (id 0x%X). %d player%s to go.\n",
                        ipstr, port ,(int) packet.id,
                        remaining, remaining == 1 ? "" : "s");

                /* make sure they've heard from us at all... */
                /* !!! FIXME: Could be fatal if packet is dropped... */
                send_peer_greeting(allowed_addresses[i].host,
                                   allowed_addresses[i].port,
                                   my_id);
                memcpy( ud.user_name[i + 1], packet.name, sizeof (ud.user_name[i + 1]));
                ud.user_name[i+1][sizeof( ud.user_name[i + 1]) - 1] = 0; // force null terminator.

                if(g_bStun)
                {
                    itmp++; //addfaz router/stun addition (goto next player)
                }
                else
                {
                    tmpmax[i] = 1; //addfaz line addition
                }
            }
        }
        drawbackground();
        menus();
        updateAudio();
        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);
    }

    if (ctrlc_pressed)
    {
        printf("Connection attempt aborted.\n");
        return(0);
    }

    /* ok, now everyone is talking to you. Sort them into player numbers... */

    heard_from[max] = my_id; /* so we sort, too... */
    allowed_addresses[max].host = myip;
    allowed_addresses[max].port = udpport;

    do
    {
        remaining = 0;
        for (i = 0; i < max; i++)
        {
            if (heard_from[i] == heard_from[i+1])  /* blah. */
            {
                printf("ERROR: Two players have the same random ID!\n");
                printf("ERROR: Please restart the game to generate new IDs.\n");
                return(0);
            }

            else if (heard_from[i] > heard_from[i+1])
            {
                int tmpi;
                short tmps;

                tmps = heard_from[i];
                heard_from[i] = heard_from[i+1];
                heard_from[i+1] = tmps;

                tmpi = allowed_addresses[i].host;
                allowed_addresses[i].host = allowed_addresses[i+1].host;
                allowed_addresses[i+1].host = tmpi;

                tmps = allowed_addresses[i].port;
                allowed_addresses[i].port = allowed_addresses[i+1].port;
                allowed_addresses[i+1].port = tmps;

                remaining = 1;  /* yay for bubble sorting! */
            }
        }
    } while (remaining);

    /*
     * Now we're sorted. But, the local player is referred to by both his
     *  player number and player index ZERO, so bump everyone up one to
     *  their actual index and fill in local player as item zero.
     */

    memmove(&allowed_addresses[1], &allowed_addresses[0],
            sizeof (allowed_addresses) - sizeof (allowed_addresses[0]));
    allowed_addresses[0].host = myip;

    gcom->myconnectindex = 0;
    for (i = 1; i <= gcom->numplayers; i++)
    {
        ip = (allowed_addresses[i].host);


        if (ip == myip)
        {
            if (udpport == allowed_addresses[i].port)
                gcom->myconnectindex = i;
        }

        printf("%s:%i is player #%i.\n", static_ipstring(ip),allowed_addresses[i].port,i);
    }
    assert(gcom->myconnectindex);

    printf("Everyone ready! We are player #%i\n", gcom->myconnectindex);

    /*
     * Ok, we should have specific IPs and ports for all players, and
     *  therefore shouldn't broadcast anymore. Disable permission to do so,
     *  just in case, so we aren't flooding the LAN with broadcasted packets.
     */
    set_socket_broadcast(0);
    return(1);
}
#else
static void send_peer_greeting(uint16_t id,  uint16_t myid)
{
    PacketPeerGreeting packet;
    memset(&packet, '\0', sizeof (packet));
    packet.header = HEADER_PEER_GREETING;
    packet.id = BUILDSWAP_INTEL16(myid);
    memcpy (packet.name, myname, sizeof(packet.name));
    packet.name[sizeof (packet.name) - 1] = 0;   // force null terminator
    radioEnqueuePacket(id,  &packet, sizeof (packet));
}

static int connect_to_everyone(gcomtype *gcom, uint16_t my_id)
{
#define bcast 1
    PacketPeerGreeting packet;
    int i;
    int rc;
    uint16_t id;
    short port;
    int first_send = 1;
    unsigned short heard_from[MAXPLAYERS];
    unsigned long resendat;
    int max;
    int remaining;
    printf("peer-to-peer init. CTRL-C to abort...\n");

    memset(heard_from, '\0', sizeof (heard_from));
    //
    printf("(This client's ID for this round is 0x%X.)\n\n", my_id);

    resendat = getticks();
    remaining = max = gcom->numplayers - 1;

    printf("Waiting for %d player%s...\n", remaining, remaining==1 ? "":"s");
    if (remaining == 0)
    {
        printf("Hmmm...don't have time to play with myself.\n");
        return(0);
    }
    uint32_t now= getticks();
    while ((!ctrlc_pressed))
    {
        if (resendat <= getticks())
        {
            if (bcast)
            {
                //printf("%sroadcasting greeting...\n", first_send ? "B":"Reb");
                /* !!! FIXME: This is...probably not right. */
                send_peer_greeting(0xFFFF,  my_id);
            }
            else
            {
                for (i = 0; (i < max) ; i++)
                {

                    //only checking one player at a time works

                    if (!heard_from[i])
                    {
                       printf("%sending greeting to %s...\n",
                                first_send ? "S" : "Res",
                                allowed_addresses[i].host);

                       send_peer_greeting(allowed_addresses[i].host, my_id);
                    }

                    // If this is stun-enabled then don't loop.
                    if(g_bStun)
                    {
                        break;
                    }
                }
            }
            first_send = 0;
            resendat += CLIENT_POLL_DELAY;
        }

        _idle();
        // next-hack ? process_udp_send_queue();
        uint16_t dest = 0;
        rc = radioGetSubPacket(&id, &packet, sizeof (packet), &dest);


        if ( (rc > 0) && (id != my_id)  )
        {

            for (i = 0; i < max; i++)
            {
                if (id == allowed_addresses[i].host)  //addfaz NAT line addition
                {
                    break;
                }
                if ((bcast) && (allowed_addresses[i].host == 0))
                    break;  /* select this slot. */
            }
             if (i == max)
                printf("%d is not an allowed player.\n", id);
             else if (rc != sizeof (packet))
                printf("Missized packet/packet fragment from %i\n", id);

            else if (packet.header != HEADER_PEER_GREETING)
                printf("Unexpected packet type from %i\n", id);

            else if (heard_from[i] == 0)
            {
                packet.id = BUILDSWAP_INTEL16(packet.id);
                heard_from[i] = packet.id;
                allowed_addresses[i].host = id;   /* bcast needs this. */
                menuEnableMultiplayerReady(1);   // at least 2 players
                remaining--;
                if (!remaining)
                    now = getticks();
                printf("Heard from id 0x%X. %d player%s to go.\n",
                        (int) packet.id,
                        remaining, remaining == 1 ? "" : "s");

                /* make sure they've heard from us at all... */
                /* !!! FIXME: Could be fatal if packet is dropped... */
                send_peer_greeting(allowed_addresses[i].host,
                                   my_id);

                if(g_bStun)
                {
                    itmp++; //addfaz router/stun addition (goto next player)
                }
                else
                {
                    tmpmax[i] = 1; //addfaz line addition
                }
            }
            if (i < max && packet.header == HEADER_PEER_GREETING)
            {
                /* make sure they've heard from us at all... */
                /* !!! FIXME: Could be fatal if packet is dropped... */
                // next-hack: moved here, so that every time we get a packet from them we greet with ours.
                // we also answer only to a broadcast packet so that we avoid sending again and again
                //if (dest == 0xFFFF)
                //    send_peer_greeting(allowed_addresses[i].host, my_id);
                memcpy( ud.user_name[i + 1], packet.name, sizeof (ud.user_name[i + 1]));
                ud.user_name[i+1][sizeof( ud.user_name[i + 1]) - 1] = 0; // force null terminator.
            }
        }
        drawbackground();
        menus();
        updateAudio();
        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);
        // TODO: fix me this crap made by me (next-hack)
        if (!remaining && (getticks() - now) > 3000)
        //if (!isInLobbyMenu())
        {
            break;
        }
    }

    if (ctrlc_pressed)
    {
        printf("Connection attempt aborted.\n");
        return(0);
    }

    /* ok, now everyone is talking to you. Sort them into player numbers... */

    heard_from[max] = my_id; /* so we sort, too... */
    allowed_addresses[max].host = my_id;
    do
    {
        remaining = 0;
        for (i = 0; i < max; i++)
        {
            if (heard_from[i] == heard_from[i+1])  /* blah. */
            {
                printf("ERROR: Two players have the same random ID!\n");
                printf("ERROR: Please restart the game to generate new IDs.\n");
                return(0);
            }

            else if (heard_from[i] > heard_from[i+1])
            {
                int tmpi;
                short tmps;

                tmps = heard_from[i];
                heard_from[i] = heard_from[i+1];
                heard_from[i+1] = tmps;

                tmpi = allowed_addresses[i].host;
                allowed_addresses[i].host = allowed_addresses[i+1].host;
                allowed_addresses[i+1].host = tmpi;

                remaining = 1;  /* yay for bubble sorting! */
            }
        }
    } while (remaining);

    /*
     * Now we're sorted. But, the local player is referred to by both his
     *  player number and player index ZERO, so bump everyone up one to
     *  their actual index and fill in local player as item zero.
     */

    memmove(&allowed_addresses[1], &allowed_addresses[0],
            sizeof (allowed_addresses) - sizeof (allowed_addresses[0]));
    allowed_addresses[0].host = my_id;

    gcom->myconnectindex = 0;
    for (i = 1; i <= gcom->numplayers; i++)
    {
        id = (allowed_addresses[i].host);


        if (id == my_id)
        {
                gcom->myconnectindex = i;
        }

        printf("%i is player #%i.\n",allowed_addresses[i].host, i);
    }
    assert(gcom->myconnectindex);

    printf("Everyone ready! We are player #%i\n", gcom->myconnectindex);

    /*
     * Ok, we should have specific IPs and ports for all players, and
     *  therefore shouldn't broadcast anymore. Disable permission to do so,
     *  just in case, so we aren't flooding the LAN with broadcasted packets.
     */
    set_socket_broadcast(0);
    return(1);
}


#endif
#if WIN32
static int parse_ip(const char *str, int *ip)
{
        #if NOT_YET_IMPLEMENTED || 1

    int ip1, ip2, ip3, ip4;

    if (stricmp(str, "any") == 0)
    {
        *ip = 0;
        return(1);
    }

    if (sscanf(str, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4) != 4)
    {
        printf("\"%s\" is not a valid IP address.\n", str);
        return(0);
    }

    /* we _should_ check that 0 <= ip? <= 255, but it'll fail later anyhow. */

    *ip = ( ((ip1 & 0xFF) << 24) |
            ((ip2 & 0xFF) << 16) |
            ((ip3 & 0xFF) <<  8) |
            ((ip4 & 0xFF)      ) );
#endif
    return(1);
}

static int parse_interface(char *str, int *ip, short *udpport)
{
        #if NOT_YET_IMPLEMENTED || 1

    char *ptr = strchr(str, ':');
    if (ptr) /* portnum specified? */
        *ptr = '\0';

    if (!parse_ip(str, ip))
        return(0);

    *udpport = BUILD_DEFAULT_UDP_PORT;
    if (ptr != NULL)  /* port specified? */
    {
        ptr++;
        if (stricmp(ptr, "any") == 0)
            *udpport = 0;
        else
            *udpport = (short) atoi(ptr);
    }
#endif
    return(1);
}
static int initialize_sockets(void)
{

    int rc;
    WSADATA data;
    printf("initializing WinSock...\n");
    rc = WSAStartup(0x0101, &data);
    if (rc != 0)
    {
        printf("WinSock failed to initialize! [err==%d].\n", rc);
        return(0);
    }
    else
    {
        printf("WinSock initialized.\n");
        printf("  - Caller uses version %d.%d, highest supported is %d.%d.\n",
                data.wVersion >> 8, data.wVersion & 0xFF,
                data.wHighVersion >> 8, data.wHighVersion & 0xFF);
        printf("  - Implementation description: [%s].\n", data.szDescription);
        printf("  - System status: [%s].\n", data.szSystemStatus);
        printf("  - Max sockets: %d.\n", data.iMaxSockets);
        printf("  - Max UDP datagram size: %d.\n", data.iMaxUdpDg);
    }
    return(1);
}
static void deinitialize_sockets(void)
{
    WSACleanup();
}
static int m_ip;
//static uint8_t m_bcast;
static int parse_udp_config(const char *cfgfile, gcomtype *gcom)
{
        #if NOT_YET_IMPLEMENTED || 1

    char *tok;
    char *ptr;
    int ip = 0;  /* interface */
    int bcast = 0;
#if READ_FROM_FILE
    char* buf = read_whole_file(cfgfile);  /* we must free this. */
    if (buf == NULL)
        return(0);

#else
    char buf[] = {"interface 127.0.0.1\r\nmode peer\r\nallow 127.0.0.1"};
#endif

    ptr = buf;
    while ((tok = get_token(&ptr)) != NULL)
    {
        int bogus = 1;

        if (stricmp(tok, "interface") == 0)
        {
            if ( (tok = get_token(&ptr)) &&
                 (parse_interface(tok, &ip, &udpport)) )
            {
                bogus = 0;
            }
            printf("Interface %s:%d chosen.\n",
                    static_ipstring(ip), (int) udpport);
        }

        else if (stricmp(tok, "mode") == 0)
        {
            if ((tok = get_token(&ptr)) != NULL)
            {
                bogus = 0;
                if (stricmp(tok, "server") == 0)
                    udpmode = udpmode_server;
                else if (stricmp(tok, "client") == 0)
                    udpmode = udpmode_client;
                else if (stricmp(tok, "peer") == 0)
                    udpmode = udpmode_peer;
                else
                    bogus = 1;

                if (!bogus)
                    printf("You want to be in [%s] mode\n", tok);
            }
        }

        else if (stricmp(tok, "broadcast") == 0)
        {
            if ((tok = get_token(&ptr)) != NULL)
            {
                bcast = atoi(tok);
                if (bcast > MAXPLAYERS - 1)
                {
                    printf("WARNING: Too many broadcast players.\n");
                    bcast = MAXPLAYERS - 1;
                }

                bogus = 0;
            }
        }

        else if (stricmp(tok, "allow") == 0)
        {
            int host;
            short port=BUILD_DEFAULT_UDP_PORT;
            if ((tok = get_token(&ptr)) != NULL)
            {
                if (gcom->numplayers >= MAXPLAYERS - 1)
                    printf("WARNING: Too many allowed IP addresses.\n");

                else if (parse_interface(tok, &host, &port))
                {
                    allowed_addresses[gcom->numplayers].host = host;
                    allowed_addresses[gcom->numplayers].port = port;
                    gcom->numplayers++;
                    bogus = 0;
                }
            }
        }

        if (bogus)
            printf("bogus token! [%s]\n", tok);
    }
    m_ip = ip;
#if READ_FROM_FILE
    free(buf);
#endif
#if ORIGINAL_MULTIPLAYER | 1
    if (open_udp_socket(ip, udpport))
    {
#if TEST_MULTIPLAYER
        allowed_addresses[gcom->numplayers -  1].port++;    // next port.
#endif
        gcom->numplayers++;  /* that's you. */
        if (udpmode == udpmode_server)
            return(wait_for_other_players(gcom, ip));
        else if (udpmode == udpmode_client)
            return(connect_to_server(gcom, ip));
        else if (udpmode == udpmode_peer)
            return(connect_to_everyone(gcom, ip, bcast));

        printf("wtf?!");  /* Should be handled by a udpmode above... */
        assert(0);
    }
    else
    {
        #if TEST_MULTIPLAYER
        // probably we need to use port +1
        udpport++;
    if (open_udp_socket(ip, udpport))
    {
        gcom->numplayers++;  /* that's you. */
        if (udpmode == udpmode_server)
            return(wait_for_other_players(gcom, ip));
        else if (udpmode == udpmode_client)
            return(connect_to_server(gcom, ip));
        else if (udpmode == udpmode_peer)
            return(connect_to_everyone(gcom, ip, bcast));

        printf("wtf?!");  /* Should be handled by a udpmode above... */
        assert(0);
    }
        #endif
    }
#else
    m_bcast = bcast;
    if (open_udp_socket(ip, udpport))
    {
#if TEST_MULTIPLAYER
        allowed_addresses[gcom->numplayers -  1].port++;    // next port.
#endif
        gcom->numplayers++;  /* that's you. */
        return 1;// (connect_to_everyone(gcom, ip, bcast));
    }
    else
    {
        #if TEST_MULTIPLAYER
        // probably we need to use port +1
        udpport++;
        if (open_udp_socket(ip, udpport))
        {
            gcom->numplayers++;  /* that's you. */
            return 1;
        }
        #endif
    }
#endif
#endif
    return(0);
}
#endif

#if WIN32
static gcomtype *init_network_transport(char **ARGV, int argpos)
{
    gcomtype *retval;

    printf("UDP NETWORK TRANSPORT INITIALIZING...\n");

    ctrlc_pressed = 0;

    if (!initialize_sockets())
        return(NULL);

    retval = &gcomstruct;
//    if (retval != NULL)
    {
        int rc;
        char *cfgfile = ARGV[argpos];
        void (*oldsigint)(int);

        memset(retval, '\0', sizeof (gcomtype));
        memset(allowed_addresses, '\0', sizeof (allowed_addresses));
        udpsocket = -1;
        udpport = BUILD_DEFAULT_UDP_PORT;
        udpmode = udpmode_peer;

        oldsigint = signal(SIGINT, siginthandler);
        rc = parse_udp_config(cfgfile, retval);
#if !ORIGINAL_MULTIPLAYER & 0
        if (rc)
            rc = connect_to_everyone(retval, m_ip, m_bcast);
#endif
        signal(SIGINT, oldsigint);

        if (!rc)
        {
            deinit_network_transport(NULL);
            return(NULL);
        }
        retval->gametype = 3;  /* gametype: 1-serial,2-modem,3-net */
    }
    return(retval);
}
#else
static gcomtype *init_network_transport(void)
{
    gcomtype *retval;
    printf("15.4 NETWORK TRANSPORT INITIALIZING...\n");
    ctrlc_pressed = 0;
    retval = &gcomstruct;
    int rc;
    memset(retval, '\0', sizeof (gcomtype));
    memset(allowed_addresses, '\0', sizeof (allowed_addresses));

    uint16_t my_id;
    srand(totalclock);
    //
    while (my_id == 0 || my_id == 0xFFFF)  /* player number is based on id, low to high. */
       my_id = (unsigned short) rand();
#define DEFAULT_PAN_ID 0x4443
    radioSetClientId(my_id, DEFAULT_PAN_ID);
    retval->numplayers = 2;
    rc = connect_to_everyone(retval, my_id);

    if (!rc)
    {
        deinit_network_transport(NULL);
        return(NULL);
    }
    retval->gametype = 3;  /* gametype: 1-serial,2-modem,3-net */
    return(retval);
}

#endif


static void deinit_network_transport(gcomtype *gcom)
{
        #if  WIN32

    printf("UDP NETWORK TRANSPORT DEINITIALIZING...\n");

/*    if (gcom != NULL)
    {
        printf("  ...freeing gcom structure...\n");
        free(gcom);
    }*/

    if (udpsocket != -1)
    {
        printf("  ...closing socket...\n");
        set_socket_blockmode(1);  /* block while socket drains. */
        shutdown(udpsocket, SOCKET_SHUTDOWN_BOTH);
        socketclose(udpsocket);
        udpsocket = -1;
    }

    deinitialize_sockets();

    printf("UDP net deinitialized successfully.\n");
    #endif
}

#if WIN32
static void callcommit(void)
{
    int ip, i, rc;
    short port;
    if (udpsocket == -1)
        return;

    process_udp_send_queue();

    switch (gcom->command)
    {
        case COMMIT_CMD_GET:
            rc = get_udp_packet(&ip, &port, gcom->buffer, sizeof(gcom->buffer));
            if (rc > 0)
            {
                gcom->numbytes = rc;  /* size of new packet. */
                for (i = 1; i <= gcom->numplayers; i++)
                {
                    if ( (allowed_addresses[i].host == ip) &&
                         (allowed_addresses[i].port == port) )
                    {
                        gcom->other = i;
                        return;
                    }
                }
                /* if you manage to hit this, it'll report no packet avail. */
            }

            gcom->numbytes = 0;
            gcom->other = -1;  /* no packet available. */
            break;

        case COMMIT_CMD_SEND:
            if ((gcom->other < 0) || (gcom->other > gcom->numplayers))
            {
                printf("NET TRANSPORT ERROR: send to player out of range\n");
                return;
            }

            ip = allowed_addresses[gcom->other].host;
            if (ip == 0)  /* dropped player? */
                return;

            port = allowed_addresses[gcom->other].port;

            if (!send_udp_packet(ip, port, gcom->buffer, gcom->numbytes))
            {
                printf("NET TRANSPORT ERROR: send failed to %s:%d\n",
                        static_ipstring(ip), (int) port);
            }
            break;

        case COMMIT_CMD_SENDTOALL:
            /* skip player zero, 'cause that's a duplicate of local IP. */
            for (i = 1; i <= gcom->numplayers; i++)
            {
                ip = allowed_addresses[i].host;
                if (ip == 0)  /* dropped player? */
                    continue;

                port = allowed_addresses[i].port;

                if (!send_udp_packet(ip, port, gcom->buffer, gcom->numbytes))
                {
                    printf("NET TRANSPORT ERROR: send failed to %s:%d\n",
                            static_ipstring(ip), (int) port);
                }
            }
            break;

        case COMMIT_CMD_SENDTOALLOTHERS:
            /* skip player zero, 'cause that's a duplicate of local IP. */
            for (i = 1; i <= gcom->numplayers; i++)
            {
                if (i == gcom->myconnectindex)  /* local player. */
                    continue;

                ip = allowed_addresses[i].host;
                if (ip == 0)  /* dropped player? */
                    continue;

                port = allowed_addresses[i].port;

                if (!send_udp_packet(ip, port, gcom->buffer, gcom->numbytes))
                {
                    printf("NET TRANSPORT ERROR: send failed to %s:%d\n",
                            static_ipstring(ip), (int) port);
                }
            }
            break;


        case COMMIT_CMD_SCORE:
            // FIX_00008: minor protocol error after frags (NET TRANSPORT ERROR: Unknown command 5)

            // Leave it on, since the sendscore() function will trigger that case.
            // This will avoid to fall in the default case on each frag and flood
            // the DOS console with NET TRANSPORT ERROR: Unknown command error messages.
            // sendscore() seems to be an obsolete function, and was probably used
            // in an old netcode architecture that 3drealms forgot to remove.
            // We leave sendscore() in the code anyway since it doesnt seem to break
            // anything. It's not needed to fix what's not broken...
            gcom->other = -1;
            break;

        default:
            printf("NET TRANSPORT ERROR: Unknown command %d\n", gcom->command);
            gcom->other = -1;  /* oh well. */
            break;
    }
}
#else
static void callcommit(void)
{
    uint16_t id;
    int i, rc;
    short port;

    // next-hack: ? process_udp_send_queue();

    switch (gcom->command)
    {
        case COMMIT_CMD_GET:
            rc = radioGetSubPacket(&id,  gcom->buffer, sizeof(gcom->buffer), NULL);
            if (rc > 0)
            {
                gcom->numbytes = rc;  /* size of new packet. */
                for (i = 1; i <= gcom->numplayers; i++)
                {
                    if ( (allowed_addresses[i].host == id) )
                    {
                        gcom->other = i;
                        return;
                    }
                }
                /* if you manage to hit this, it'll report no packet avail. */
            }

            gcom->numbytes = 0;
            gcom->other = -1;  /* no packet available. */
            break;

        case COMMIT_CMD_SEND:
            if ((gcom->other < 0) || (gcom->other > gcom->numplayers))
            {
                printf("NET TRANSPORT ERROR: send to player out of range\n");
                return;
            }

            id = allowed_addresses[gcom->other].host;
            if (id == 0)  /* dropped player? */
                return;

            if (!radioEnqueuePacket(id, gcom->buffer, gcom->numbytes))
            {
                printf("NET TRANSPORT ERROR: send failed to %d\n", id);
            }
            break;

        case COMMIT_CMD_SENDTOALL:
            /* skip player zero, 'cause that's a duplicate of local IP. */
            for (i = 1; i <= gcom->numplayers; i++)
            {
                id = allowed_addresses[i].host;
                if (id == 0)  /* dropped player? */
                    continue;

                if (!radioEnqueuePacket(id, gcom->buffer, gcom->numbytes))
                {
                    printf("NET TRANSPORT ERROR: send failed to %d\n", id);
                }
            }
            break;

        case COMMIT_CMD_SENDTOALLOTHERS:
            /* skip player zero, 'cause that's a duplicate of local IP. */
            for (i = 1; i <= gcom->numplayers; i++)
            {
                if (i == gcom->myconnectindex)  /* local player. */
                    continue;

                id = allowed_addresses[i].host;
                if (id == 0)  /* dropped player? */
                    continue;

                if (!radioEnqueuePacket(id, gcom->buffer, gcom->numbytes))
                {
                    printf("NET TRANSPORT ERROR: send failed to %d\n", id);
                }
            }
            break;


        case COMMIT_CMD_SCORE:
            // FIX_00008: minor protocol error after frags (NET TRANSPORT ERROR: Unknown command 5)

            // Leave it on, since the sendscore() function will trigger that case.
            // This will avoid to fall in the default case on each frag and flood
            // the DOS console with NET TRANSPORT ERROR: Unknown command error messages.
            // sendscore() seems to be an obsolete function, and was probably used
            // in an old netcode architecture that 3drealms forgot to remove.
            // We leave sendscore() in the code anyway since it doesnt seem to break
            // anything. It's not needed to fix what's not broken...
            gcom->other = -1;
            break;

        default:
            printf("NET TRANSPORT ERROR: Unknown command %d\n", gcom->command);
            gcom->other = -1;  /* oh well. */
            break;
    }
}
#endif
#else
#include "mmulti.h"
#include <stdint.h>
#include "radio.h"
#include "build.h"
#include "duke3d.h"
#include "printf.h"
#include "engine.h"
#include "display.h"
#define DEFAULT_PAN_ID 0x4443
#define CLIENT_POLL_DELAY_TICKS                 500
#define HEADER_PEER_GREETING                    245
typedef struct
{
#if 0
    unsigned char dummy1;   /* so these don't confuse game after load. */
    unsigned char dummy2;   /* so these don't confuse game after load. */
    unsigned char dummy3;   /* so these don't confuse game after load. */
#endif
    unsigned char header;   /* always HEADER_PEER_GREETING (245). */
    unsigned short id;
    uint8_t name[MAX_NAME_LEN];
} PacketPeerGreeting;

short numplayers = 0, myconnectindex = 0;
short connecthead, connectpoint2[MAXPLAYERS];
char syncstate = 0;

static void send_peer_greeting(uint16_t myid)
{
    PacketPeerGreeting packet;
    memset(&packet, '\0', sizeof (packet));
    packet.header = HEADER_PEER_GREETING;
    packet.id = (myid);
    memcpy (packet.name, myname, sizeof(packet.name));
    packet.name[sizeof (packet.name) - 1] = 0;   // force null terminator
    radioSendAsynchPacket((uint8_t *)&packet, sizeof (packet));
}


static int connect_to_everyone(uint16_t my_id, int maxplayers)
{
    PacketPeerGreeting packet;
    int i;
    int rc;
    uint16_t id;
    uint16_t heard_from[MAXPLAYERS];

    unsigned int resendat;
    int max;
    int remaining;
    int numPlayersFound = 1;
    printf("peer-to-peer init...\n");

    memset(heard_from, '\0', sizeof (heard_from));
    //
    printf("(This client's ID for this round is 0x%X.)\n\n", my_id);
    // copy my name
    strncpy (ud.user_name[0], myname, sizeof(ud.user_name[0]));
    //
    resendat = getticks();
    remaining = max = maxplayers - 1;

    printf("Waiting for %d player%s...\n", remaining, remaining==1 ? "":"s");
    if (remaining == 0)
    {
        printf("Hmmm...don't have time to play with myself.\n");
        return(0);
    }
    uint32_t now= getticks();
    while (1)
    {
        if (resendat <= getticks())
        {
            send_peer_greeting(my_id);
            resendat += CLIENT_POLL_DELAY_TICKS;
        }
        rc = radioGetAsynchPacket((uint8_t*)&packet, sizeof (packet));
        if ( (rc > 0) && (packet.id != my_id)  )
        {
            // get short address
            id = packet.id;
            for (i = 0; i < max; i++)
            {
                if (id == getAllowedAddress(i))  //addfaz NAT line addition
                {
                    break;
                }
                if ((getAllowedAddress(i) == 0))
                    break;  /* select this slot. */
            }
             if (i == max)
                printf("%d is not an allowed player.\n", id);
             else if (rc != sizeof (packet))
                printf("wrong packet size from %i\n", id);

            else if (packet.header != HEADER_PEER_GREETING)
                printf("Unexpected packet type from %i\n", id);

            else if (heard_from[i] == 0)
            {
                packet.id = packet.id;
                heard_from[i] = packet.id;
                setAllowedAddress(i,  id);   /* bcast needs this. */
                if (remaining)
                {
                    remaining--;
                    numPlayersFound++;
                }
                else
                    now = getticks();
                menuEnableMultiplayerReady(true);   // at least 2 players
                printf("Heard from id 0x%X. %d player%s to go.\n",
                        (int) packet.id,
                        remaining, remaining == 1 ? "" : "s");

                /* make sure they've heard from us at all... */
                /* !!! FIXME: Could be fatal if packet is dropped... */
                send_peer_greeting(my_id);

            }
            if (i < max && packet.header == HEADER_PEER_GREETING)
            {
                /* make sure they've heard from us at all... */
                /* !!! FIXME: Could be fatal if packet is dropped... */
                // next-hack: moved here, so that every time we get a packet from them we greet with ours.
                // we also answer only to a broadcast packet so that we avoid sending again and again
                //if (dest == 0xFFFF)
                //    send_peer_greeting(allowed_addresses[i].host, my_id);
                memcpy( ud.user_name[i + 1], packet.name, sizeof (ud.user_name[i + 1]));
                ud.user_name[i+1][sizeof( ud.user_name[i + 1]) - 1] = 0; // force null terminator.
            }
        }
        drawbackground();
        menus();
        updateAudio();
        nextpage(SCREEN_HEIGHT);
        waitForUpdate(-1);
        // TODO: fix me this crap made by me (next-hack)
        if (!remaining && (getticks() - now) > 500)
        //if (!isInLobbyMenu())
        {
            break;
        }
        if (!isInLobbyMenu() && remaining) // we have pressed ready
        {
            now = getticks();
            max = numPlayersFound - 1;
            numplayers = numPlayersFound;
            remaining = 0;
            radioSetNumberOfPeers(numPlayersFound);
        }
    }
// todo fixme: handle if we exit from menu.
    /* ok, now everyone is talking to you. Sort them into player numbers... */

    heard_from[max] = my_id; /* so we sort, too... */
    for (int i = max; i > 0; i--)
    {
        strncpy (ud.user_name[i - 1], ud.user_name[i], sizeof(ud.user_name[0]));
    }
    strncpy (ud.user_name[max], myname, sizeof(ud.user_name[0]));

    setAllowedAddress(max,  my_id);
    do
    {
        remaining = 0;
        for (i = 0; i < max; i++)
        {
            char tmpName[MAX_NAME_LEN];
            if (heard_from[i] == heard_from[i+1])  /* blah. */
            {
                printf("ERROR: Two players have the same random ID!\n");
                printf("ERROR: Please restart the game to generate new IDs.\n");
                return(0);
            }
            else if (heard_from[i] > heard_from[i+1])
            {
                int tmpi;
                short tmps;

                tmps = heard_from[i];
                heard_from[i] = heard_from[i+1];
                heard_from[i+1] = tmps;

                // next-hack: argh, pardon me! But we are just doing this horrible thing few times.
                strncpy (tmpName, ud.user_name[i], sizeof(tmpName));
                strncpy (ud.user_name[i], ud.user_name[i + 1], sizeof(ud.user_name[i]));
                strncpy (ud.user_name[i + 1], tmpName, sizeof(ud.user_name[i + 1]));


                tmpi = getAllowedAddress(i);
                setAllowedAddress(i, getAllowedAddress(i + 1));
                setAllowedAddress(i + 1, tmpi);

                remaining = 1;  /* yay for bubble sorting! */
            }
        }
    } while (remaining);

    /*
     * Now we're sorted. But, the local player is referred to by both his
     *  player number and player index ZERO, so bump everyone up one to
     *  their actual index and fill in local player as item zero.
     */
    for (int i = MAXPLAYERS; i > 0; i--)
    {
        setAllowedAddress(i, getAllowedAddress(i - 1));
    }
    setAllowedAddress(0,  my_id);

    myconnectindex = 0;


    for (i = 1; i <= numplayers; i++)
    {
        id = getAllowedAddress(i);


        if (id == my_id)
        {
                myconnectindex = i;
                radioSetConnectionIndex(myconnectindex);
        }

        printf("%i is player #%i.\n",getAllowedAddress(i), i);
    }

    printf("Everyone ready! We are player #%i\n", myconnectindex);

    /*
     * Ok, we should have specific IPs and ports for all players, and
     *  therefore shouldn't broadcast anymore. Disable permission to do so,
     *  just in case, so we aren't flooding the LAN with broadcasted packets.
     */
    return(myconnectindex);
}
#if DEBUG_BROADCAST_PACKET
void broadcastPacket2(uint8_t * packet, int length, int notme, char *file, int line, char *function)
{
    printf("File %s Line %d Function %s\r\n", file, line, function);
#else
void broadcastPacket(uint8_t * packet, int length, int notme)
{

#endif
    (void) notme;
    // so simple...
    radioEnqueueSubPacketForTransmission(packet, length);
}
static uint32_t id_seed = 1;

// Function to set the seed for the random number generator
void id_srand (uint32_t new_seed)
{
    id_seed = new_seed;
}

// Function to generate a pseudo-random number
int id_rand(void) {
    // Constants for the LCG algorithm
    const uint32_t a = 1664525;
    const uint32_t c = 1013904223;
    const uint32_t m = 0xFFFFFFFF; // 2^32

    // Update the seed using the LCG formula
    id_seed = (a * id_seed + c) & m;

    // Return the generated random number
    return (int)(id_seed >> 16) & 0x7FFF; // Return a 15-bit random number
}
static void init_network_transport(void)
{
    printf("15.4 NETWORK TRANSPORT INITIALIZING...\n");
    resetAllowedAddresses();
//    sampletimer();
    uint16_t my_id = 0;
    id_srand(totalclock);
    //
    while (my_id == 0 || my_id == 0xFFFF)  /* player number is based on id, low to high. */
       my_id = (unsigned short) id_rand();
    radioSetClientId(my_id, DEFAULT_PAN_ID);
    numplayers = 4;

    radioSetNumberOfPeers(numplayers);

    connect_to_everyone(my_id, numplayers);
    playerswhenstarted = numplayers;
    //
}
void uninitmultiplayers(void)
{
    // do nothing
}

void sendlogon(void)
{
    // do nothing
}

void sendlogoff(void)
{
    uint8_t tempbuf[2];

    tempbuf[0] = 255;
    tempbuf[1] = myconnectindex;
    broadcastPacket(tempbuf, 2, true);

}
#if WIN32
static void deinit_network_transport(void)
{

}
#endif
void flushpackets(void)
{
    // do nothing
}
int32_t getoutputcirclesize(void)
{
    return(0);
}
void genericmultifunction(int32_t other, const uint8_t *bufptr, int32_t messleng, int32_t command)
{
    if (numplayers < 2) return;
    // do nothing
    UNUSED(other);
    UNUSED(bufptr);
    UNUSED(messleng);
    UNUSED(command);

}
void initmultiplayers()
{
    int oldmyconnectindex = myconnectindex;
    init_network_transport();
    myconnectindex--;
    // next-hack: workaround when gettind a different connection index.
    if (myconnectindex != oldmyconnectindex)
        memcpy(&ps[myconnectindex], &ps[oldmyconnectindex], sizeof(ps[0]));

    connecthead = 0;
    for(int i = 0; i < numplayers-1;i++)
    {
        connectpoint2[i] = i+1;
    }
    connectpoint2[numplayers-1] = -1;
#if 0
    for(i=0;i<numplayers;i++)
    {
        lastsendtime[i] = totalclock;
    }
#endif
}
short getpacket (short *other, uint8_t *bufptr)
{
    // next-hack: instead of that crappy getpacket, we prefer this one
    int length;
    uint8_t clientId;
    length = radioGetSubPacket(&clientId, bufptr, 128);
    *other = clientId - 1;
    return length;
}
#endif
