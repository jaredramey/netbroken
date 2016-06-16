#pragma once

#pragma comment( lib, "wsock32.lib" ) // windows top-level wrapper for winsock - 'WSA' functions
#pragma comment( lib, "ws2_32.lib"  ) // traditional socket libraries

#include <WinSock2.h>   // for wsock32.lib
#include <WS2tcpip.h>   // for ws2_32.lib
#include <cstdio>
#include <cstring>

#define NSFW_MAKELONG(a,b,c,d) (a << 24 | b << 16 | c << 8 | d)
#define NSFW_SPLITLONG(a) { (char)((a & 0xFF000000) >> 24), (char)((a & 0x00FF0000) >> 16), (char)((a & 0x0000FF00) >> 8), (char)(a & 0x000000FF)  }

namespace nsfw
{
    // necessary prerequisites for working with windows networking
    bool initNetworking() { WSAData data; return WSAStartup(MAKEWORD(2, 2), &data) == 0; }
    bool termNetworking() {               return WSACleanup()                      == 0; }

    //convert string of format: "127.1.1.0:50000" to sockaddr_in
    sockaddr_in stoa(const char *src)
    {        
        sockaddr_in dst;        
        dst.sin_family = AF_INET;  
        unsigned char *qtr = (unsigned char*)&dst.sin_addr.S_un.S_addr;
        sscanf_s(src,"%hhu.%hhu.%hhu.%hhu:%hu",qtr,qtr+1,qtr+2,qtr+3,&dst.sin_port);
                
        dst.sin_port = htons(dst.sin_port);
        return dst;
    }

    class Address
    {
        // host format
        unsigned long  addr;
        unsigned short port; 
    public:
        Address() : addr(0), port(0) {}
        Address(char a, char b, char c, char d, unsigned short p)
            : port{p}, addr(NSFW_MAKELONG(a,b,c,d)) {}

        Address(const char *ip) : Address(stoa(ip)){}
        Address(const sockaddr_in &addr)
            : port{ntohs(addr.sin_port)}, addr{ntohl(addr.sin_addr.S_un.S_addr)} {}
    
        sockaddr_in toN() const
        {
            sockaddr_in dst;
            dst.sin_family = AF_INET;
            dst.sin_addr.S_un.S_addr = htonl(addr);
            dst.sin_port = htons(port);
            return dst;
        }
        void toString(char *dst, size_t dlen) const
        {
            char blah[4] = NSFW_SPLITLONG(addr);
            sprintf_s(dst,dlen,"%hhu.%hhu.%hhu.%hhu:%hu",blah[3],blah[2],blah[1],blah[0],port);
        }                
    };

    struct Packet
    {
        unsigned long long MS;
        unsigned long ID;
        char data[1500];
    };

    class Socket
    {
        int handle = 0;
    public:
        ~Socket() { close(); }
        
        bool isOpen() const { return handle > 0; }
        
        bool open(unsigned short port = 0)
        {            
            sockaddr_in addr;
            addr.sin_family           = AF_INET;
            addr.sin_addr.S_un.S_addr = INADDR_ANY;
            addr.sin_port             = htons(port);            
            DWORD param = 1;

            handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
            bind(handle, (sockaddr*)&addr, sizeof(sockaddr_in));
            ioctlsocket(handle, FIONBIO, &param);
            return true;
        }
        bool close()
        {
            if (!isOpen())
            closesocket(handle);
            handle = 0;
            return true;
        }

        // Packet, address
        bool sendRaw(const char *data, int dlen, const sockaddr_in &addr)
        {
            return sendto(handle, data, dlen, 0, (sockaddr*)&addr, sizeof(sockaddr_in)) == dlen;
        }
        bool recvRaw(char *data, int dlen, sockaddr_in &addr)
        {
            int in_len = sizeof(sockaddr_in);
            return recvfrom(handle, data, dlen, 0, (sockaddr*)& addr, &in_len) == dlen;
        }
 

        bool send(const Packet &pack, const Address &addr)
        {
            return sendto(handle, (char*)&pack, sizeof(Packet), 0, (sockaddr*)&(addr.toN()), sizeof(sockaddr_in)) == sizeof(Packet);
        }
        bool recv(Packet &pack, Address &addr)
        {
            int in_len = sizeof(sockaddr_in);
            sockaddr_in t;
            int bytes = recvfrom(handle, (char*)&pack, sizeof(Packet), 0, (sockaddr*)&t, &in_len);
            addr = Address(t);
            return bytes == sizeof(Packet);
        }
 
   };
}

//sendto(handle, out_buffer, 40, 0, (sockaddr*)&out_addr, sizeof(sockaddr_in));   // failed to send if retval != packet_size
//recvfrom(handle, in_buffer, 40, 0, (sockaddr*)& in_addr, &in_len);             // no data if retval <= 0