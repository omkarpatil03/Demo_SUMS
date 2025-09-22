
TARGET= motor_control_unit

CSTCS= $(TARGET).cpp 
COBJS= $(CSRCS:.cpp=.o)

CC= g++
CFFLAGS= -Wall
DYNAMIC= -lpthread

$(TARGET): $(COBJS)
	$(CC) $^ -o $@ $(CFFLAGS) $(DYNAMIC)

$(COBJS):%.o: %.cpp
	$(CC) $(CFFLAGS) -c $<

run:
	./$(TARGET)

clean:
	del *.o

.phony: run clean
