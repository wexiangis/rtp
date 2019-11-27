
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "rtp.h"

#define RTP_IP "127.0.0.1"//"172.16.23.217"//"127.0.0.1"
#define RTP_PORT 9832

int main(int argc, char* argv[])
{
    int fd;
    int ret;
    SocketStruct *ss;
    uint8_t aacBuff[2048];
    AacHeader AacHeader;
    RtpPacket rtpPacket;
    char wsdp = 0;

    if(argc != 2)
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

    ss = rtp_socket(RTP_IP, RTP_PORT, 1);
    if(!ss)
    {
        printf("failed to create udp socket\n");
        close(fd);
        return -1;
    }

    rtp_header(&rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_AAC, 1, 0, 0, 0x32411);

    while(1)
    {
        printf("--------------------------------\n");

        ret = read(fd, aacBuff, 7);
        if(ret <= 0)
        {
            lseek(fd, 0, SEEK_SET);
            continue;
        }

        if(aac_parseHeader(aacBuff, &AacHeader, 1) < 0)
        {
            printf("parse err\n");
            lseek(fd, 0, SEEK_SET);
            continue;
        }

        if(!wsdp)
            rtp_create_sdp("./test.sdp", 
                RTP_IP, RTP_PORT, 
                AacHeader.channelCfg, 
                aac_freq[AacHeader.samplingFreqIndex],
                RTP_PAYLOAD_TYPE_AAC);

        ret = read(fd, rtpPacket.payload, AacHeader.aacFrameLength-7);
        if(ret <= 0)
        {
            lseek(fd, 0, SEEK_SET);
            continue;
        }

        rtp_send(ss, &rtpPacket, AacHeader.aacFrameLength-7);
        
        rtpPacket.rtpHeader.timestamp += (AacHeader.adtsBufferFullness+1)/2;

        // usleep(23000);
        usleep(
            (AacHeader.adtsBufferFullness+1)/2
            *1000*1000
            /aac_freq[AacHeader.samplingFreqIndex]);
    }

    close(fd);
    close(ss->fd);
    free(ss);

    return 0;
}
