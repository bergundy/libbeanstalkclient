TESTS = evbsc.t
check_PROGRAMS = $(TESTS)
ALL_LIBS = -lev -lbeanstalkclient -lsockutils -lioqueue

srcdir = $(top_builddir)/src
AM_CFLAGS = -I$(srcdir) 

commands_t_SOURCES = check_commands.c evbsc.h
commands_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
commands_t_LDADD   = @CHECK_LIBS@ $(srcdir)/evbsc.o
