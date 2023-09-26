#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include "base64.h"

int fromHex(char ch) {
  if(ch >= '0' && ch <= '9')
    return (int) ch - '0';
  return (int) ch - 'A' + 10;
}

void decodeURL(char* src,char* dest) {
  while(*src != '\0') {
    if(*src == '%') {
      ++src;
      int n1 = fromHex(*src++);
      int n2 = fromHex(*src++);
      *dest++ = (char) n1*16+n2;
    } else {
      *dest++ = *src++;
    }
  }
  *dest = '\0';
}

void serveRequest(int fd) {
  // Read the request
  char buffer[1024];
	int bytesRead = read(fd,buffer,1023);
  buffer[bytesRead] = '\0';

  // Grab the method and URL
  char method[16];
  //question: what kind of URL are we grabbing here?
  //this is different than the url that will arrive in the body?
  char url[128];
  sscanf(buffer,"%s %s",method,url);


  //Handle POST -encode URL
  if (strcmp(method ,"POST") == 0){
    //question: what strstr() function returns is only a position in the buffer, not the url that we want right?
    char *urlPosition = strstr(buffer, "\r\n\r\nurl=");
    //allocate memory for the url
    size_t urlLength = strlen(urlPosition+8);
    char *userURL = (char *)malloc(urlLength + 1);
    strcpy(userURL, urlPosition+8);

    //questions: can I just reuse the buffer to save the decoded url like I did here?
    //or should I create a new space for the decoded url destination.
    decodeURL(&userURL, buffer);

    //question: since we're writing one url at a time to the txt file,
    //should I use the c standard library function instead of system calls?
    FILE* f = fopen("url.txt","wa");
    long position = ftell(f);
    //question: I'm guessing from here, I do need to allocate a memory for the decoded url because I'm not sure if I can use the buffer as a parameter
    fprintf(f, "%s",buffer);
    fprintf(f, "%s", "\n");
    fclose(f);

    char encoded[64];
    encode(position, encoded);

    int f200 = open("200Response.txt",O_RDONLY);
    int readSize = read(f200,buffer,1023);
    buffer[readSize] = '\0';
    close(f200);

    char* OKpoisition = strstr(buffer,"XXXXXX");

    int encodedLength = strlen(encoded);
    int diff = 6 - encodedLength;
    if(diff != 0){
      for(int i=0; i<diff; i++){
        encoded[encodedLength] = ' ';
        encodedLength++;
      }
      encoded[encodedLength] = '\0';
    }
    strcpy(&OKpoisition, encoded);
    //if encoded < 6 characters fill in with " "
    write(fd,buffer,readSize);
  }
  //Handle GET
  else {
    char* result = strstr(url, "/s/");
    if(result != NULL){
      int f404 = open("404Response.txt",O_RDONLY);
      int readSize = read(f404,buffer,1023);
      close(f404);
      write(fd,buffer,readSize);
    }
    else{
      char encoded[7];
      //question: not result is the position of where /s/ starts, so should I do the +3?
      strcpy(encoded, result+3);
      encoded[6] = '\0';

      //decoded position of the url in the url.txt
      long position[64] = decode(encoded);
      //question: r or rb?
      FILE* f = fopen("url.txt","rb");
      fseek(f, position, SEEK_SET);
      char desiredURL[128];
      //question: how do I know the size and length of the url that is stored in the url.txt?
      size_t bytesRead = fread(desiredURL, sizeof(), len(), f);
      desiredURL[bytesRead] = '\0';

      int f301 = open("301Response.txt",O_RDONLY);
      int readSize = read(f301,buffer,1023);
      close(f301);

      //question: not too sure if & should be there, and if +10 works?
      char* RedirectPosition = strstr(buffer,"Location: ");
      strcpy(&RedirectPosition+10, desiredURL);
      write(fd,buffer,readSize);
    }
  }
  close(fd);
}

void* workerThread(void *arg) {
  queue* q = (queue*) arg;
  while(1) {
    int fd = dequeue(q);
    serveRequest(fd);
  }
  return NULL;
}

int main() {
  // Set up the queue
  queue* q = queueCreate();

  // Set up the worker threads
  pthread_t w1,w2;
  pthread_create(&w1,NULL,workerThread,q);
  pthread_create(&w2,NULL,workerThread,q);

  // Create the socket
  int server_socket = socket(AF_INET , SOCK_STREAM , 0);
  if (server_socket == -1) {
    printf("Could not create socket.\n");
    return 1;
  }

  // Set the 'reuse address' socket option
  int on = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  //Prepare the sockaddr_in structure
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( 8888 );

  // Bind to the port we want to use
  if(bind(server_socket,(struct sockaddr *)&server , sizeof(server)) < 0) {
    printf("Bind failed\n");
    return 1;
  }
  printf("Bind done\n");

  // Mark the socket as a passive socket
  listen(server_socket , 3);

  // Accept incoming connections
  printf("Waiting for incoming connections...\n");
  while(1) {
    struct sockaddr_in client;
    int new_socket , c = sizeof(struct sockaddr_in);
    new_socket = accept(server_socket, (struct sockaddr *) &client, (socklen_t*)&c);
    if(new_socket != -1) {
      enqueue(q,new_socket);
    }
  }

  return 0;
}
