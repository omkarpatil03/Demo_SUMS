#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

#include "vehicle_control_unit.hpp"

#define     SYSTEM_FAULT_TH     10

int mcu_speed_th_reached= 0;
int bms_current_th_reached= 0;

int bms_over_current_counter= 0;
int mcu_over_speed_counter= 0;


//VCU CAN receiver thread
void vcu_can_receiver_task(void){
    int mySocket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    //create CAN socket
    mySocket= socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(mySocket < 0){
        perror("VCU Socket error");
        return;
    }

    //preaction before binding
    strcpy(ifr.ifr_name, "vcan0");
    ioctl(mySocket, SIOCGIFINDEX, &ifr);

    addr.can_family= AF_CAN;
    addr.can_ifindex= ifr.ifr_ifindex;

    //bind the socket
    if( bind(mySocket, (struct sockaddr *) &addr, sizeof(addr)) < 0 ){
        perror("VCU Bind error");
        close(mySocket);
        return;
    }

    std::cout<<"[Receiver] VCU listing on vcan0..."<<std::endl;

    //continuos read for CAN message
    while(1){
        //read socket
        int nbytes= read(mySocket, &frame, sizeof(frame));
        
        if(nbytes < 0){
            perror("VCU Read error");
            break;
        }

        std::cout<<"VCU [Receiver] Got CAN frame: ID=0x"
                 <<std::hex<<frame.can_id
                 <<"DLC="<<std::dec<<int(frame.can_dlc)
                 <<"Data=";

        for(int i= 0; i < frame.can_dlc; i++){
            std::cout<<std::hex<<int(frame.data[i])<<" ";
        }         

        //mcu data
        if(frame.can_id == 0x201){
            if(frame.data[6] >= SYSTEM_FAULT_TH){
                mcu_speed_th_reached= 1;
            }

            mcu_over_speed_counter= frame.data[6];

        }else if(frame.can_id == 0xB1){
            if(frame.data[6] >= SYSTEM_FAULT_TH){
                bms_current_th_reached= 1;
            }

            bms_over_current_counter= frame.data[6];
        }

        std::cout<<std::dec<<std::endl;
    }

    close(mySocket);
}

//VCU CAN message sender thread
void vcu_can_sender_task(void){
    int mySocket;
    struct sockaddr_can addr;
    struct ifreq ifr;

    //open CAN socket
    mySocket= socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(mySocket < 0){
        perror("VCU socket error");
        return;
    }

    //find interface indx for vcan0
    strcpy(ifr.ifr_name, "vcan0");
    if(ioctl(mySocket, SIOCGIFINDEX, &ifr) < 0){
        perror("VCU ioctl error");
        close(mySocket);
        return;
    }

    addr.can_family= AF_CAN;
    addr.can_ifindex= ifr.ifr_ifindex;

    //bind to vcan0
    if(bind(mySocket, (struct sockaddr *)&addr, sizeof(addr)) < 0 ){
        perror("VCU bind error");
        close(mySocket);
        return;
    }

    std::cout<<"[Sender] VCU started on vcaon0 ..."<<std::endl;

    struct can_frame frame;
    frame.can_id= 0x10a;
    frame.can_dlc= 8;

    while(1){

        //for(int i= 0; i<8; i++){
        //    frame.data[i]= i*3;
        //}

        frame.data[0]= 0x01;
        
        frame.data[1]= 0x0;
        frame.data[2]= 0x0;
        frame.data[3]= 0x0;

        frame.data[4]= bms_over_current_counter;        
        frame.data[5]= mcu_over_speed_counter;
        frame.data[6]= bms_current_th_reached;
        frame.data[7]= mcu_speed_th_reached;



        int nbytes= write(mySocket, &frame, sizeof(struct can_frame));

        if(nbytes < 0){
            perror("VCU write error");
            break;
        }

        //std::cout<<"[Sender] VCU sent frame #"<<std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));

    }

    close(mySocket);
}

