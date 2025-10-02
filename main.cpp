#include <iostream>
#include <thread>

#include <csignal>
#include <sys/time.h>

#include <unistd.h>


#include "vehicle_control_unit.hpp"
#include "battery_management_system.hpp"
#include "motor_control_unit.hpp"
#include "tel_control_unit.hpp"

#include "trng.hpp"


struct itimerval timer;
int tel_timer_counter= 0;

//ISR handler for timer ISR
void timer_handler(int signum){
    tel_timer_counter++;
    //std::cout << "timer fired! Count= " << tel_timer_counter << std::endl;
}

void Timer0_Init(void){
    //Install signal handler for SIGALRM -> signal ipc
    struct sigaction sa;
    sa.sa_handler= timer_handler;
    sa.sa_flags= SA_RESTART;
    sigisemptyset(&sa.sa_mask);

    if(sigaction(SIGALRM, &sa, nullptr) == -1){
        perror("sigaction");
        return;
    }

    //create timer
    
    timer.it_value.tv_sec= 1;           //first firing after 1sec
    timer.it_value.tv_usec= 0;
    timer.it_interval.tv_sec= 1;        //periodicity 1sec
    timer.it_interval.tv_usec= 0;

    
}

void start_timer0(void){
    if(setitimer(ITIMER_REAL, &timer, nullptr) == -1){
        perror("setitimer");
        return;
    }
}

int main(void){

    


    /*
    while(1){
        try{
            int number= getTrueRandomNumber();
            std::cout << "Random number:" << number << std::endl;
            sleep(1);
        }
        catch(const std::exception &e){
            std::cerr << e.what() << std::endl;

            return 1;
        }
    }
    */

    Timer0_Init();
    start_timer0();


    std::thread receiver_bms(bms_can_receiver_task);
    std::thread sender_bms(bms_can_sender_task);

    std::thread receiver_mcu(mcu_can_receiver_task);
    std::thread sender_mcu(mcu_can_sender_task);

    std::thread receiver_vcu(vcu_can_receiver_task);
    std::thread sender_vcu(vcu_can_sender_task);

    std::thread receiver_tel(tel_can_receiver_task);
    std::thread sender_tel(tel_can_sender_task);

    receiver_vcu.join();
    sender_vcu.join();

    receiver_bms.join();
    sender_bms.join();

    receiver_mcu.join();
    sender_mcu.join();

    receiver_tel.join();
    sender_tel.join();



    return 0;
}