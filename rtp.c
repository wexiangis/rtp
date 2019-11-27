/*
 * 作者：_JT_
 * 博客：https://blog.csdn.net/weixin_42462202
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>

#include "rtp.h"

void rtp_header(RtpPacket* rtpPacket, uint8_t csrcLen, uint8_t extension,
                    uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker,
                   uint16_t seq, uint32_t timestamp, uint32_t ssrc)
{
    rtpPacket->rtpHeader.csrcLen = csrcLen;
    rtpPacket->rtpHeader.extension = extension;
    rtpPacket->rtpHeader.padding = padding;
    rtpPacket->rtpHeader.version = version;
    rtpPacket->rtpHeader.payloadType =  payloadType;
    rtpPacket->rtpHeader.marker = marker;
    rtpPacket->rtpHeader.seq = seq;
    rtpPacket->rtpHeader.timestamp = timestamp;
    rtpPacket->rtpHeader.ssrc = ssrc;
}

int rtp_send(SocketStruct *ss, RtpPacket* rtpPacket, uint32_t dataSize)
{
    int ret;

    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);

    rtpPacket->rtpHeader.len1 = 0x00;
    rtpPacket->rtpHeader.len2 = 0x10;
    rtpPacket->rtpHeader.len3 = (dataSize>>5)&0xFF;
    rtpPacket->rtpHeader.len4 = (dataSize&0x1F)<<3;

    ret = sendto(
        ss->fd, 
        (void*)rtpPacket, 
        dataSize+RTP_HEADER_SIZE, 
        MSG_DONTWAIT,
        (struct sockaddr*)&ss->addr,
        ss->addrSize);

    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

    rtpPacket->rtpHeader.seq++;

    return ret;
}

int rtp_recv(SocketStruct *ss, RtpPacket* rtpPacket, uint32_t *dataSize)
{
    int ret;

    ret = recvfrom(
        ss->fd, 
        (void*)rtpPacket, 
        sizeof(RtpPacket), 
        0,
        (struct sockaddr*)&ss->addr,
        (socklen_t *)(&ss->addrSize));

    if(ret > 0 && dataSize)
        *dataSize = (rtpPacket->rtpHeader.len3<<5)|(rtpPacket->rtpHeader.len4>>3);

    return ret;
}

SocketStruct* rtp_socket(uint8_t *ip, uint16_t port, uint8_t isServer)
{
    int fd;
    int ret;
    SocketStruct *ss;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        fprintf(stderr, "socket err\n");
        return -1;
    }

    //非阻塞设置
    ret = fcntl(fd , F_GETFL , 0);
    fcntl(fd , F_SETFL , ret | O_NONBLOCK);

    ss = (SocketStruct*)calloc(1, sizeof(SocketStruct));
    ss->fd = fd;
    ss->addr.sin_family = AF_INET;
    ss->addr.sin_port = htons(port);
    ss->addr.sin_addr.s_addr = inet_addr(ip);
    ss->addrSize = sizeof(ss->addr);

    if(!isServer)
    {
        ret = bind(fd, &ss->addr, ss->addrSize);
        if(fd < 0)
        {
            fprintf(stderr, "bind err\n");
            free(ss);
            return NULL;
        }
    }

    return ss;
}


void rtp_create_sdp(uint8_t *file, uint8_t *ip, uint16_t port, uint16_t chn, uint16_t freq, uint16_t type)
{
    char buff[1024] = {0};
    char typeName[64] = {0};
    // char demo[] = 
    //     "m=audio 9832 RTP/AVP 97\n"
    //     "a=rtpmap:97 mpeg4-generic/44100/2\n"
    //     "a=fmtp:97 sizeLength=13;mode=AAC-hbr;config=1210;\n"
    //     "c=IN IP4 127.0.0.1";
    char demo[] = 
        "m=audio %d RTP/AVP %d\n"
        "a=rtpmap:%d %s/%d/%d\n"
        "a=fmtp:%d sizeLength=13;config=%d;\n"
        "c=IN IP4 %s";
    uint16_t config = 1410, _freq = 8;
    int fd;

    if(type == RTP_PAYLOAD_TYPE_AAC)
        strcpy(typeName, "mpeg4-generic");
    else if(type == RTP_PAYLOAD_TYPE_PCMA)
        strcpy(typeName, "pcma");
    else
        strcpy(typeName, "mpeg4-generic");

    if(freq == 96000) _freq = 0;
    else if(freq == 88200) _freq = 1;
    else if(freq == 64000) _freq = 2;
    else if(freq == 48000) _freq = 3;
    else if(freq == 44100) _freq = 4;
    else if(freq == 32000) _freq = 5;
    else if(freq == 24000) _freq = 6;
    else if(freq == 22050) _freq = 7;
    else if(freq == 16000) _freq = 8;
    else if(freq == 12000) _freq = 9;
    else if(freq == 11025) _freq = 10;
    else if(freq == 8000) _freq = 11;
    else if(freq == 7350) _freq = 12;
    config = 0x1; config <<= 5;
    config |= _freq; config <<= 4;
    config |= chn; config <<= 3;
    config = ((config>>12)&0xF)*1000 + ((config>>8)&0xF)*100 + ((config>>4)&0xF)*10 + ((config>>0)&0xF);
    snprintf(buff, sizeof(buff), demo, port, type, type, typeName, freq, chn, type, config, ip);
    remove(file);
    if((fd = open(file, O_WRONLY|O_CREAT, 0666)) > 0)
    {
        write(fd, buff, strlen(buff));
        close(fd);
    }
}

//----------------- AAC -------------------

int aac_freq[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350};

int aac_header(uint8_t* in, uint8_t chn, uint16_t freq, uint16_t codeRate, uint16_t datLen)
{
    datLen += 7;
    
    //byte 1
    *in++ = 0xFF;//syncword[8]
    //byte 2
    *in++ = 0xF1;//syncword[4],id[1],layer[2],protectionAbsent[1]
    //byte 3
    *in = (0x1<<6);//profile[2]
    if(freq == 96000) *in |= (0x9<<2);//samplingFreqIndex[4]
    else if(freq == 88200) *in |= (0x1<<2);
    else if(freq == 64000) *in |= (0x2<<2);
    else if(freq == 48000) *in |= (0x3<<2);
    else if(freq == 44100) *in |= (0x4<<2);
    else if(freq == 32000) *in |= (0x5<<2);
    else if(freq == 24000) *in |= (0x6<<2);
    else if(freq == 22050) *in |= (0x7<<2);
    else if(freq == 16000) *in |= (0x8<<2);
    else if(freq == 12000) *in |= (0x9<<2);
    else if(freq == 11025) *in |= (0xa<<2);
    else if(freq == 8000) *in |= (0xb<<2);
    else if(freq == 7350) *in |= (0xc<<2);
    else *in |= (0x8<<2);
    *in |= (0x0<<1);//privateBit[1]
    *in++ |= (chn>>2);//channelCfg[1]
    //byte 4
    *in = ((chn&0x3)<<6);//channelCfg[2]
    *in |= (0x0<<5);//originalCopy[1]
    *in |= (0x0<<4);//home[1]
    *in |= (0x0<<3);//copyrightIdentificationBit[1]
    *in |= (0x0<<2);//copyrightIdentificationStart[1]
    *in++ |= (datLen>>11);//aacFrameLength[2]
    //byte 5
    *in++ = ((datLen>>3)&0xFF);//aacFrameLength[8]
    //byte 6
    *in = ((datLen&0x7)<<5);//aacFrameLength[3]
    *in++ |= (codeRate>>6);//adtsBufferFullness[5]
    //byte 7
    *in = ((codeRate&0x3F)<<2);//adtsBufferFullness[6]
    *in |= (0x0&0x3);//numberOfRawDataBlockInFrame[2]
    //
    return 7+datLen;
}

int aac_parseHeader(uint8_t* in, AacHeader* res, uint8_t show)
{
    static int frame_number = 0;
    memset(res,0,sizeof(*res));

    if ((in[0] == 0xFF)&&((in[1] & 0xF0) == 0xF0))
    {
        res->id = ((unsigned int) in[1] & 0x08) >> 3;
        res->layer = ((unsigned int) in[1] & 0x06) >> 1;
        res->protectionAbsent = (unsigned int) in[1] & 0x01;
        res->profile = ((unsigned int) in[2] & 0xc0) >> 6;
        res->samplingFreqIndex = ((unsigned int) in[2] & 0x3c) >> 2;
        res->privateBit = ((unsigned int) in[2] & 0x02) >> 1;
        res->channelCfg = ((((unsigned int) in[2] & 0x01) << 2) | (((unsigned int) in[3] & 0xc0) >> 6));
        res->originalCopy = ((unsigned int) in[3] & 0x20) >> 5;
        res->home = ((unsigned int) in[3] & 0x10) >> 4;
        res->copyrightIdentificationBit = ((unsigned int) in[3] & 0x08) >> 3;
        res->copyrightIdentificationStart = (unsigned int) in[3] & 0x04 >> 2;
        res->aacFrameLength = (((((unsigned int) in[3]) & 0x03) << 11) |
                                (((unsigned int)in[4] & 0xFF) << 3) |
                                    ((unsigned int)in[5] & 0xE0) >> 5) ;
        res->adtsBufferFullness = (((unsigned int) in[5] & 0x1f) << 6 |
                                        ((unsigned int) in[6] & 0xfc) >> 2);
        res->numberOfRawDataBlockInFrame = ((unsigned int) in[6] & 0x03);

        if(show)
        {
            printf("adts:id  %d\n", res->id);
            printf( "adts:layer  %d\n", res->layer);
            printf( "adts:protection_absent  %d\n", res->protectionAbsent);
            printf( "adts:profile  %d\n", res->profile);
            printf( "adts:sf_index  %dHz\n", aac_freq[res->samplingFreqIndex]);
            printf( "adts:pritvate_bit  %d\n", res->privateBit);
            printf( "adts:channel_configuration  %d\n", res->channelCfg);
            printf( "adts:original  %d\n", res->originalCopy);
            printf( "adts:home  %d\n", res->home);
            printf( "adts:copyright_identification_bit  %d\n", res->copyrightIdentificationBit);
            printf( "adts:copyright_identification_start  %d\n", res->copyrightIdentificationStart);
            printf( "adts:aac_frame_length  %d\n", res->aacFrameLength);
            printf( "adts:adts_buffer_fullness  %d\n", res->adtsBufferFullness);
            printf( "adts:no_raw_data_blocks_in_frame  %d\n", res->numberOfRawDataBlockInFrame);
        }

        return 0;
    }
    else
    {
        printf("failed to parse adts header\n");
        return -1;
    }
}