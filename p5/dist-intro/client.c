#include <stdio.h>
#include "mfs.h"

#define BUFFER_SIZE (1000)

// client code
int main(int argc, char *argv[]) {
    int rc = MFS_Init("localhost", 10000);
    if(rc < 0){
        printf("Failed at Init");
        exit(1);
    }
    // struct sockaddr_in addrSnd, addrRcv;

    // int sd = UDP_Open(20000);
    // int rc = UDP_FillSockAddr(&addrSnd, "localhost", 10000);

    // char message[BUFFER_SIZE];
    // sprintf(message, "hello world");

    char* name = "testFile";

    for (int i = 0; i < 10; i++) {
        int rc;
        if (i % 2 == 0) {
            rc = MFS_Creat(0, MFS_DIRECTORY, name);
            if(rc < 0){
                printf("Failed at Creat");
                exit(1);
            }
        } else {
            rc = MFS_Lookup(0, name);
            if(rc < 0){
                printf("Failed at Creat");
                exit(1);
            }
        }
        printf("%d\n", rc);
    //     printf("client:: wait for reply...\n");
    // rc = UDP_Read(sd, &addrRcv, message, BUFFER_SIZE);
    // printf("client:: got reply [size:%d contents:(%s)\n", rc, message);
    }

    // int 

    

    

    // printf("%d\n", rc);

    // printf("client:: send message [%s]\n", message);
    // rc = UDP_Write(sd, &addrSnd, message, BUFFER_SIZE);
    // if (rc < 0) {
	// printf("client:: failed to send\n");
	// exit(1);
    // }

    
    return 0;
}

