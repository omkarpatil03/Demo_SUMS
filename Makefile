
TARGET = main

CC = g++
CFLAGS = -Wall
DYNAMIC = -lpthread -lmysqlcppconn

CSRCS = $(TARGET).cpp vehicle_control_unit.cpp battery_management_system.cpp motor_control_unit.cpp tel_control_unit.cpp trng.cpp
COBJS = $(CSRCS:.cpp=.o)

$(TARGET): $(COBJS)
	$(CC) $^ -o $@ $(CFLAGS) $(DYNAMIC)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f *.o $(TARGET)

.PHONY: run clean
