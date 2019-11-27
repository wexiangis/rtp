
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "rtp.h"

#define RTP_IP "127.0.0.1"
#define RTP_PORT 9832

int main(int argc, char* argv[])
{
    int fd;
    int ret;
    SocketStruct *ss;
    RtpPacket rtpPacket;
    char wsdp = 0;

    int readSize = 1024;
    int chn = 2, freq = 16000;
    int timestamp = 1000000*1024/(16000*2*2);
    int seekStart = 0;

    if(argc != 4)
    {
        printf("Usage: %s <dest ip>\n", argv[0]);
        return -1;
    }

    fd = open(argv[1], O_RDONLY);
    if(fd < 0)
    {
        printf("failed to open %s\n", argv[1]);
        return -1;
    }

    if(strstr(argv[1], ".wav"))
        seekStart = 44;
    chn = atoi(argv[2]);
    freq = atoi(argv[3]);
    timestamp = 1000000*1024/(freq*chn*2);

    ss = rtp_socket(RTP_IP, RTP_PORT, 1);
    if(!ss)
    {
        printf("failed to create udp socket\n");
        close(fd);
        return -1;
    }

    rtp_header(&rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_PCMA, 1, 0, 0, 0x32411);

    while(1)
    {
        printf("--------------------------------\n");

        if(!wsdp)
            rtp_create_sdp("./test.sdp", RTP_IP, RTP_PORT, chn, freq, RTP_PAYLOAD_TYPE_PCMA);

        ret = read(fd, rtpPacket.payload, readSize);
        if(ret <= 0)
        {
            lseek(fd, seekStart, SEEK_SET);
            continue;
        }

        ret = rtp_send(ss, &rtpPacket, readSize);
        if(ret > 0)
            printf("send: %d, %d\n", ret, rtpPacket.rtpHeader.seq);
        
        rtpPacket.rtpHeader.timestamp += timestamp;

        usleep(timestamp);
    }

    close(fd);
    close(ss->fd);
    free(ss);

    return 0;
}
