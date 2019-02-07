CC=g++
CFLAGS+= -g -std=c++11 
POST_LIBS= -lm -lcrypto -lpthread

OBJ_DIR=../
TEST_DEPS=Value.h
TEST_OBJS= paxos_id.o paxos_timer.o  paxos_instance.o paxos_msg.o UDP_comms.o util.o node.o

INCLUDES_RAW=
#driver.o acceptor.o proposer.o learner.o comms.o
DEBUG=-g

DEPS=util.h $(TEST_DEPS)
OBJS= $(addprefix $(OBJ_DIR), $(TEST_OBJS))
INCLUDES = $(addprefix -I, $(INCLUDES_RAW))
.cpp.o: %.cpp $(TEST_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

#%.o: %.cpp $(DEPS)
#	$(CC) $(CFLAGS) -c -o $@ $<  $(POST_LIBS)

#test: test.o $(OBJS)
#	$(CC) -o $@ $^ $(CFLAGS)
	
#test_server: test_server.o $(OBJS)
#	$(CC) -o $@ $^ $(CFLAGS)
	
#test_client : test_client.o $(OBJS)
#	$(CC) -o $@ $^ $(CFLAGS)

#test_daemon: test_paxos_daemon.o $(OBJS)
#	$(CC) -o $@ $^ $(CFLAGS)

all: test_paxos_client test_paxos_server 

test_paxos_client: test_paxos_client.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(POST_LIBS)

test_paxos_server: test_paxos_server.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(POST_LIBS)

clean: 
	rm -f *.o test_paxos_client test_paxos_server  test.exe test_client.exe test_server.exe test_paxos_daemon.exe test_paxos_client.exe $(OBJS)
