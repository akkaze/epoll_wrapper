CC = g++ -fPIC -g
LDFLAGS = -lm -lpthread

# set DEBUG options
ifdef DEBUG
CFLAGS = -Wall -Wextra -ggdb -pg -std=c++11 -lpthread
else
CFLAGS = -Wall -O2 -std=c++11 -lpthread 
endif

#name all the object files
OBJS = poll.o 
LIB = libpoll.a
all : bin
debug :
	make all DEBUG=1

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $^
$(LIB) : $(OBJS)
	ar rcs $(LIB) $^
server : server.o $(LIB) 
	$(CC) $(LDFLAGS) $< -o $@ -L./ -lpoll 
client : client.o $(LIB)
	$(CC) $(LDFLAGS) $< -o $@ -L./ -lpoll 
bin : server client

clean :
	rm -rf $(OBJS) server.o client.o server client libpoll.a



