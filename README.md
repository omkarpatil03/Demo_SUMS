# Demo_SUMS
Virtual simuation of SUMS


# SQL
>sudo mysql -u root -p

>create table Vehicle_Data(Battery_SoC int, Battery_Vtg float, Battery_Curr float, BMS_OverCurr_Count int, MCU_Curr float, MCU_Speed int, MCU_OverSpeed_Count int,Time timestamp default CURRENT_TIMESTAMP);

sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

