/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stddef.h>

#define SOCSNDBUFSIZE 4096    /* Size of socket send buffer */
#define SOCRCVBUFSIZE 4096   /* Size of socket receive buffer */
#define SOCK_NAME_SIZE 30
// Variables
int sock;
char socSendBuffer[SOCSNDBUFSIZE];
char socRcvBuffer[SOCRCVBUFSIZE];
pthread_t receiveThread;

void * receive_data(void * threadData) {
  /*unused param*/
  threadData = NULL;
  int bytesRcvd,i;
  char closeAckCmd[] = {'A','_','C','l','o','s','e','\n'};

  while(1){
    if ((bytesRcvd = recv(sock, socRcvBuffer, SOCRCVBUFSIZE - 1, 0)) <= 0){
      printf("recv() failed or connection closed prematurely");
      break;
    }

    // Exit the loop if socket server close ack is received
    if(bytesRcvd == 8){
      for(i=0;i<8;i++){
        if(closeAckCmd[i] != socRcvBuffer[i]){
          break;
        }
      }

      if(i == 8){
        break;
      }
    }

    for (i=0; i < bytesRcvd; i++){
      printf("%c", socRcvBuffer[i]);
    }
  }
  pthread_exit(NULL);
  return NULL;
}

void send_data(){
  int i;
  char closeCmd[] = {'C','l','o','s','e'};

  while(1){
    if (fgets(socSendBuffer, SOCSNDBUFSIZE , stdin) != NULL) {
      if (send(sock, socSendBuffer, (strlen(socSendBuffer) - 1), 0) != (strlen(socSendBuffer)-1)){
        printf("Socket Send Failed\n");
        close(sock);
        return;
      }

      // Exit the loop if socket close is sent to server
      if((strlen(socSendBuffer)-1) == 5){
        for(i=0;i<5;i++){
          if(closeCmd[i] != socSendBuffer[i]){
            break;
          }
        }

        if(i == 5){
          break;
        }
      }
    }
  }
}

int main(int argc, char *argv[])
{
  int size;
  char localSocketName[SOCK_NAME_SIZE] = {0};
  struct sockaddr_un servAddr;

  if((sock = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0){
    printf("Socket Failed err num %d msg: %s\n", errno, strerror(errno));
    return 0;
  }else{
    printf("Socket Success\n");
  }

  if (argc == 2 && strlen(argv[1]) < SOCK_NAME_SIZE) {
    memcpy(localSocketName,argv[1],strlen(argv[1]));
  } else {
    printf("Socket name is not entered/invalid \n");
    printf("Enter Local Socket Name: ");
    if(fgets(localSocketName, sizeof(localSocketName) , stdin) == NULL){
      printf("fgets failed\n");
      return 0;
    }
  }

  printf("Local Socket name is %s\n", localSocketName);
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sun_family = AF_LOCAL;

  strlcpy(servAddr.sun_path + 1, localSocketName, strlen(localSocketName));
  size = sizeof(servAddr) - sizeof(servAddr.sun_path) + strlen(servAddr.sun_path+1) + 1;
  if(connect(sock, (struct sockaddr *) &servAddr, size) < 0){
    printf("Socket Failed err num %d msg: %s\n", errno, strerror(errno));
    return 0;
  } else{
    printf("Socket Connected\n");
  }

  // Create new thread to receive messages
  pthread_create(&receiveThread, NULL, receive_data, NULL);

  // Send user input
  send_data();

  pthread_join(receiveThread, NULL);
  close(sock);
  return 0;
}
