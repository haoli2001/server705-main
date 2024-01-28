TARGET  := OUT
SOURCE  := main.cpp mpi_manage.cpp sim_lib.cpp socketFunctions.cpp resource.cpp
OBJS	:= $(SOURCE: %.cpp=%.o)
#LDFLAGS := -L /home/xianyun/XD_YLSim/code/usr_lib/
LDFLAGS := -L /home/hao/Documents/KunPeng/2023.11.7/
LIBS    := -lstdc++ -lpthread

CC  := mpicxx

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LIBS) 

$(OBJS):$(SOURCE)
	$(CC) -g -c $(OBJS) $(SOURCE)

clean:
	rm -rf $(TARGET) *.o
run:
	./run.sh  $(TARGET)
