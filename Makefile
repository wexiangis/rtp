
all:
	gcc -o sendAAC sendAAC.c rtp.c
	gcc -o recvAAC recvAAC.c rtp.c
	gcc -o sendPCM sendPCM.c rtp.c
	gcc -o recvPCM recvPCM.c rtp.c

clean:
	rm sendAAC recvAAC sendPCM recvPCM
