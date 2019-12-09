CC = gcc
FLAGS = -Wall
TARGETS = mystation station-manager bus comptroller
OBJS = mystation.o station-manager.o bus.o comptroller.o

all: $(TARGETS)

mystation:mystation.o
	$(CC) $(FLAGS) -o mystation mystation.o

station-manager:station-manager.o
	$(CC) $(FLAGS) -o station-manager station-manager.o

bus:bus.o
	$(CC) $(FLAGS) -o bus bus.o

comptroller:comptroller.o
	$(CC) $(FLAGS) -o comptroller comptroller.o

mystation.o:./src/mystation.c
	$(CC) $(FLAGS) -o mystation.o -c ./src/mystation.c

station-manager.o:./src/station-manager.c
	$(CC) $(FLAGS) -o station-manager.o -c ./src/station-manager.c

bus.o:./src/bus.c
	$(CC) $(FLAGS) -o bus.o -c ./src/bus.c

comptroller.o:./src/comptroller.c
	$(CC) $(FLAGS) -o comptroller.o -c ./src/comptroller.c

.PHONY : clean

clean:
	rm -f $(TARGETS) $(OBJS)