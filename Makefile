CC=gcc
CFLAGS=-Wall -Werror -g
SRCDIR=src
TESTDIR=tests
TEST_FLAGS=-I$(SRCDIR) -lcheck
EV_LIBS=-lv
OBJECTS=evbsc.o evbuffer.o evvector.o

all:     main

check: $(TESTDIR)/evbuffer.t $(TESTDIR)/evvector.t $(TESTDIR)/evbsc.t
	$(TESTDIR)/evbuffer.t  
	$(TESTDIR)/evvector.t  
	$(TESTDIR)/evbsc.t  

$(TESTDIR)/evbsc.t: $(TESTDIR)/check_evbsc.c evbsc.o
	$(CC) $(TEST_FLAGS) -o $(TESTDIR)/evbsc.t $(TESTDIR)/check_evbsc.c $(OBJECTS) $(EV_LIBS)

$(TESTDIR)/evvector.t: $(TESTDIR)/check_evvector.c evvector.o
	$(CC) $(CFLAGS) $(TEST_FLAGS) -o $(TESTDIR)/evvector.t $(TESTDIR)/check_evvector.c evvector.o

$(TESTDIR)/evbuffer.t: $(TESTDIR)/check_evbuffer.c evbuffer.o
	$(CC) $(CFLAGS) $(TEST_FLAGS) -o $(TESTDIR)/evbuffer.t $(TESTDIR)/check_evbuffer.c evbuffer.o

libevbeanstalkclient.so: $(OBJECTS)
	$(CC) $(CFLAGS) -o libevbeanstalkclient.so $(OBJECTS) $(EV_LIBS)

evvector.o:    $(SRCDIR)/evvector.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/evvector.c

evbuffer.o:    $(SRCDIR)/evbuffer.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/evbuffer.c

evbsc.o:    $(SRCDIR)/evbsc.c
	$(CC) $(CFLAGS) -c $(SRCDIR)/evbsc.c

clean:
	rm -f *.so *.o tests/*.t
