#include <iostream>
#include <stdexcept>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>

#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

#include "tel_control_unit.hpp"

#define     TEL_DATA_UPDATE         (3)    //sec

using namespace std;

int Battery_SoC= 0;
float Battery_Vtg= 0;
float Battery_Curr= 0;
int BMS_OverCurr_Count= 0;
float MCU_Curr= 0;
int MCU_Speed= 0;
int MCU_OverSpeed_Count= 0;

extern int tel_timer_counter;

//TEL CAN receiver thread
void tel_can_receiver_task(void){
    int mySocket;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame;

    //create CAN socket
    mySocket= socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(mySocket < 0){
        perror("TEL Socket error");
        return;
    }

    //preaction before binding
    strcpy(ifr.ifr_name, "vcan0");
    ioctl(mySocket, SIOCGIFINDEX, &ifr);

    addr.can_family= AF_CAN;
    addr.can_ifindex= ifr.ifr_ifindex;

    //bind the socket
    if( bind(mySocket, (struct sockaddr *) &addr, sizeof(addr)) < 0 ){
        perror("TEL Bind error");
        close(mySocket);
        return;
    }

    std::cout<<"[Receiver] TEL listing on vcan0..."<<std::endl;

    //continuos read for CAN message
    while(1){
        //read socket
        int nbytes= read(mySocket, &frame, sizeof(frame));
        
        if(nbytes < 0){
            perror("TEL Read error");
            break;
        }

        //vcu
        if(frame.can_id == 0x10a){
            //do nothing
            BMS_OverCurr_Count= frame.data[4];
            MCU_OverSpeed_Count= frame.data[5];
            memset(&frame, 0, sizeof(frame));
        }
        //bms
        else if(frame.can_id == 0xb1){
            Battery_SoC= frame.data[5];

            Battery_Vtg= (frame.data[1] << 8) | frame.data[0];
            Battery_Vtg= Battery_Vtg * 0.1;

            Battery_Curr= (frame.data[3] << 8) | frame.data[2];
            Battery_Curr= Battery_Curr * 0.1;

            

            memset(&frame, 0, sizeof(frame));
        }
        //mcu
        else if(frame.can_id == 0x201){
            MCU_Curr= (frame.data[5] << 8) | frame.data[4];
            MCU_Curr= MCU_Curr * 0.1;

            MCU_Speed= frame.data[0];

            

            memset(&frame, 0, sizeof(frame));
        }

        if(tel_timer_counter >= TEL_DATA_UPDATE){
            //add data into table
            connectsql_addintotable();
            tel_timer_counter= 0;
        }

        /*
        std::cout<<"TEL [Receiver] Got CAN frame: ID=0x"
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

//TEL CAN message sender thread
void tel_can_sender_task(void){
    int mySocket;
    struct sockaddr_can addr;
    struct ifreq ifr;

    //open CAN socket
    mySocket= socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(mySocket < 0){
        perror("TEL socket error");
        return;
    }

    //find interface indx for vcan0
    strcpy(ifr.ifr_name, "vcan0");
    if(ioctl(mySocket, SIOCGIFINDEX, &ifr) < 0){
        perror("TEL ioctl error");
        close(mySocket);
        return;
    }

    addr.can_family= AF_CAN;
    addr.can_ifindex= ifr.ifr_ifindex;

    //bind to vcan0
    if(bind(mySocket, (struct sockaddr *)&addr, sizeof(addr)) < 0 ){
        perror("TEL bind error");
        close(mySocket);
        return;
    }


    std::cout<<"[Sender] TEL started on vcaon0 ..."<<std::endl;

    struct can_frame frame;
    frame.can_id= 0x508;
    frame.can_dlc= 8;

    while(1){

        //for(int i= 0; i<8; i++){
        //    frame.data[i]= i*3;
        //}

        //mobilization
        frame.data[0]= 0x0;

        frame.data[1]= 0x0;

        frame.data[2]= 0x0;
        frame.data[3]= 0x0;
        frame.data[4]= 0x0;
        frame.data[5]= 0x0;

        //faults
        frame.data[6]= 0x03;

        frame.data[7]= 0x0;

        int nbytes= write(mySocket, &frame, sizeof(struct can_frame));

        if(nbytes < 0){
            perror("TEL write error");
            break;
        }

        //std::cout<<"[Sender] TEL sent frame #"<<std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(3));

    }

    close(mySocket);
}



bool connectsql_addintotable(void){

    try{
        //1. create driver and connection
        sql::Driver *driver;
        sql::Connection *conn;
        sql::PreparedStatement *pstmt;

        driver= get_driver_instance();

        conn= driver->connect("tcp://127.0.0.1:3306", "root", "root");

        //2. connect to the database

        conn->setSchema("demo_sums");

        //3. insert data

        pstmt= conn->prepareStatement("INSERT INTO Vehicle_Data(Battery_SoC, Battery_Vtg, Battery_Curr, BMS_OverCurr_Count, MCU_Curr, MCU_Speed, MCU_OverSpeed_Count) VALUES (?, ?, ?, ?, ?, ?, ?)");

        //pstmt->setInt(1, 10);
        //pstmt->setString(2, "OMKAR");
        //pstmt->setInt(3, 24);
        pstmt->setInt(1, Battery_SoC);
        pstmt->setDouble(2, Battery_Vtg);
        pstmt->setDouble(3, Battery_Curr);
        pstmt->setInt(4, BMS_OverCurr_Count);

        pstmt->setDouble(5, MCU_Curr);
        pstmt->setInt(6, MCU_Speed);
        pstmt->setInt(7, MCU_OverSpeed_Count);

        pstmt->execute();

        cout<<"Data inserted successfully!"<<endl;

        delete pstmt;
        delete conn;

        return  true;
    }
    catch(sql::SQLException &e){
        cerr<<"Error:"<<e.what()<<endl;
    }

    return false;
}