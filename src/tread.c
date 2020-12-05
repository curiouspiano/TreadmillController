#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#include <pthread.h>
#include <wiringPi.h>

#include <pigpio.h>

//define our "globals" for configuration purposes.
//port to bind to
#define PORT 8003
#define MILLISECOND 9995

struct ListenData {
        int sockfd;
        int* speedVal;
};

void* async_listen(void* arg){
        int sockfd = ((struct ListenData*)arg)->sockfd;
        int*speedVal = ((struct ListenData*)arg)->speedVal;
                int err;
                printf("Starting to listen on %d\n", sockfd);
                char buff[20] = {0};
                while(*speedVal != -1) {
                        err = read(sockfd, buff, sizeof(buff));
                        if(err < 0)
                                perror("Read error:");
                        //printf("Raw Buff: {%s}  ", buff);
                        sscanf(buff, "%c", speedVal);
                        //printf("Read {%c} {%d}\n", *speedVal, *speedVal);
                        bzero(buff, 20);
                }
}

int pulse(int msOn, int msTotal) {
        int count = 0;
        //printf("pulsing {%d} of {%d}\n", msOn, msTotal);
        if(msOn > 0)
                system("echo 1 > /sys/class/gpio/gpio17/value");

        while(count < msTotal){
                if(count >= msOn)
                {
                        system("echo 0 > /sys/class/gpio/gpio17/value");
                }
                count = count + 1;
                usleep(MILLISECOND);
        }
        return 0;
}

int keepRunning = 1;

void intHandler(int sig){
        printf("keepRunning: %d\n", keepRunning);
        keepRunning = 0;
        printf("Caught Interrupt\n");
}

const int PWM_pin = 1;

int main(int argc, char const *argv[]) {

    //system("echo out > /sys/class/gpio/gpio17/direction");

    if (wiringPiSetup () == -1)
            exit (1) ;

    pinMode (PWM_pin, PWM_OUTPUT) ; /* set PWM pin as output */
    
    //Configure the clock speed of the PWM to 20hz
    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(3840);
    pwmSetRange(250);
    //Times in terms of Ms
    int time_on = 100;
    int pulse_width = 102;

    struct sigaction act;
    act.sa_handler = intHandler;

    int cont = 1;

    sigaction(SIGINT, &act, NULL);

    printf("Signal catcher initiated\n");
    // SERVER SETUP
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "Hello from server";


    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                       (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    struct ListenData *listenData = (struct ListenData *)malloc(sizeof(struct ListenData));
    listenData->sockfd = new_socket;
    listenData->speedVal = malloc(sizeof(int));
    *listenData->speedVal=0;
    pthread_t listenThread;
    printf("starting thread\n");

    //create a thread that constantly listens for the new speed to send to the treadmill.
    pthread_create(&listenThread, NULL, &async_listen, (void *)listenData);
    printf("post start thread\n");

    printf("Thread created, %d, %d\n", listenData->sockfd, listenData->speedVal);
    int newSpeed;

    //Continuously loop, updating the speed of the PWM signal based on whatever value is on our async struct.
    while(*listenData->speedVal != -1 && keepRunning == 1){
                newSpeed = *listenData->speedVal;
                newSpeed = (newSpeed * 5)/2;
                if(newSpeed > 212)
                        newSpeed = 212;
                //printf("Writing %d", newSpeed);
                pwmWrite (PWM_pin, newSpeed);
                //usleep(1000000);
    }

    printf("Broke out of read loop\n");
    //system("echo 0 > /sys/class/gpio/gpio17/value");
    pwmWrite(PWM_pin, 0);
    close(new_socket);
}
