#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

#include "battery_management_system.hpp"

#define     BMS_CURRENT_TH      (300)

extern int true_mcu_speed;
extern int mcu_current;
int bms_current= 0;
int bms_current_th_counter= 0;

//BMS CAN receiver thread
void bms_can_receiver_task(void){
    int mySocket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    //create CAN socket
    mySocket= socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(mySocket < 0){
        perror("BMS Socket error");
        return;
    }

    //preaction before binding
    strcpy(ifr.ifr_name, "vcan0");
    ioctl(mySocket, SIOCGIFINDEX, &ifr);

    addr.can_family= AF_CAN;
    addr.can_ifindex= ifr.ifr_ifindex;

    //bind the socket
    if( bind(mySocket, (struct sockaddr *) &addr, sizeof(addr)) < 0 ){
        perror("BMS Bind error");
        close(mySocket);
        return;
    }

    std::cout<<"[Receiver] BMS listing on vcan0..."<<std::endl;

    //continuos read for CAN message
    while(1){
        //read socket
        int nbytes= read(mySocket, &frame, sizeof(frame));
        
        if(nbytes < 0){
            perror("BMS Read error");
            break;
        }

        /*
        std::cout<<"[Receiver] BMS Got CAN frame: ID=0x"
                 <<std::hex<<frame.can_id
                 <<"DLC="<<std::dec<<int(frame.can_dlc)
                 <<"Data=";

        for(int i= 0; i < frame.can_dlc; i++){
            std::cout<<std::hex<<int(frame.data[i])<<" ";
        }         

        std::cout<<std::dec<<std::endl;
        */
    }

    close(mySocket);
}

//BMS CAN message sender thread
void bms_can_sender_task(void){
    int mySocket;
    struct sockaddr_can addr;
    struct ifreq ifr;

    //open CAN socket
    mySocket= socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(mySocket < 0){
        perror("BMS socket error");
        return;
    }

    //find interface indx for vcan0
    strcpy(ifr.ifr_name, "vcan0");
    if(ioctl(mySocket, SIOCGIFINDEX, &ifr) < 0){
        perror("BMS ioctl error");
        close(mySocket);
        return;
    }

    addr.can_family= AF_CAN;
    addr.can_ifindex= ifr.ifr_ifindex;

    //bind to vcan0
    if(bind(mySocket, (struct sockaddr *)&addr, sizeof(addr)) < 0 ){
        perror("BMS bind error");
        close(mySocket);
        return;
    }

    std::cout<<"[Sender] BMS started on vcaon0 ..."<<std::endl;

    struct can_frame frame;
    frame.can_id= 0xB1;
    frame.can_dlc= 8;

    while(1){

        //batt vtg
        frame.data[0]= 0x1A;
        frame.data[1]= 0x02;

        bms_current= (true_mcu_speed * 3.66);
        frame.data[2]= bms_current;
        frame.data[3]= bms_current >> 8;
        

        //speed th counter
        if(bms_current > BMS_CURRENT_TH){
            frame.data[6]= bms_current_th_counter++;
        }

    
        //batt SoC
        frame.data[5]= 0x52;
        
        //BMS status
        frame.data[6]= 0x02;
        
        //BMS fault status
        frame.data[7]= 0x0;

        int nbytes= write(mySocket, &frame, sizeof(struct can_frame));

        if(nbytes < 0){
            perror("BMS write error");
            break;
        }

        //std::cout<<"[Sender] BMS sent frame #"<<std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));

    }

    close(mySocket);
}

