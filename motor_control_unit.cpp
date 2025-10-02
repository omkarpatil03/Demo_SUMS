#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

#include "motor_control_unit.hpp"
#include "trng.hpp"

#define     MCU_SPEED_TH    80

int mcu_speed= 0;
int true_mcu_speed= 0;
int mcu_current= 0;
int mcu_speed_th_counter= 0;



//MCU CAN receiver thread
void mcu_can_receiver_task(void){
    int mySocket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    //create CAN socket
    mySocket= socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(mySocket < 0){
        perror("MCU Socket error");
        return;
    }

    //preaction before binding
    strcpy(ifr.ifr_name, "vcan0");
    ioctl(mySocket, SIOCGIFINDEX, &ifr);

    addr.can_family= AF_CAN;
    addr.can_ifindex= ifr.ifr_ifindex;

    //bind the socket
    if( bind(mySocket, (struct sockaddr *) &addr, sizeof(addr)) < 0 ){
        perror("MCU Bind error");
        close(mySocket);
        return;
    }

    std::cout<<"[Receiver] MCU listing on vcan0..."<<std::endl;

    //continuos read for CAN message
    while(1){
        //read socket
        int nbytes= read(mySocket, &frame, sizeof(frame));
        
        if(nbytes < 0){
            perror("MCU Read error");
            break;
        }

        /*
        std::cout<<"MCU [Receiver] Got CAN frame: ID=0x"
                 <<std::hex<<frame.can_id
                 <<"DLC="<<std::dec<<int(frame.can_dlc)
                 <<"Data=";

        for(int i= 0; i < frame.can_dlc; i++){
            std::cout<<std::hex<<int(frame.data[i])<<" ";
        }
        */             

        std::cout<<std::dec<<std::endl;
    }

    close(mySocket);
}

//MCU CAN message sender thread
void mcu_can_sender_task(void){
    int mySocket;
    struct sockaddr_can addr;
    struct ifreq ifr;

    //open CAN socket
    mySocket= socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(mySocket < 0){
        perror("MCU socket error");
        return;
    }

    //find interface indx for vcan0
    strcpy(ifr.ifr_name, "vcan0");
    if(ioctl(mySocket, SIOCGIFINDEX, &ifr) < 0){
        perror("MCU ioctl error");
        close(mySocket);
        return;
    }

    addr.can_family= AF_CAN;
    addr.can_ifindex= ifr.ifr_ifindex;

    //bind to vcan0
    if(bind(mySocket, (struct sockaddr *)&addr, sizeof(addr)) < 0 ){
        perror("MCU bind error");
        close(mySocket);
        return;
    }

    std::cout<<"[Sender] MCU started on vcaon0 ..."<<std::endl;

    struct can_frame frame;
    frame.can_id= 0x201;
    frame.can_dlc= 8;

    while(1){

        
        mcu_speed= getTrueRandomNumber();

        if(mcu_speed > 60){
            frame.data[0]= mcu_speed;
            true_mcu_speed= mcu_speed;
        }

        //mcu speed byte reserved
        frame.data[1]= 0x0;

        //mcu dc voltage
        frame.data[2]= 0x1A;
        frame.data[3]= 0x01;


        //mcu dc current
        mcu_current= (true_mcu_speed * 3.56);

        frame.data[4]= mcu_current;
        frame.data[5]= mcu_current >> 8;

        //speed th counter
        if(true_mcu_speed > MCU_SPEED_TH){
            frame.data[6]= mcu_speed_th_counter++;
        }

        //mcu reserved
        
        frame.data[7]= 0x0;

        int nbytes= write(mySocket, &frame, sizeof(struct can_frame));

        if(nbytes < 0){
            perror("MCU write error");
            break;
        }

        //std::cout<<"[Sender] MCU sent frame #"<<std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    }

    close(mySocket);
}
