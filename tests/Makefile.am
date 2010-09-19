TESTS = bsc.t ivector.t commands.t responses.t stats.t ioqueue.t
check_PROGRAMS = $(TESTS)

srcdir = $(top_builddir)/src
AM_CFLAGS = -I$(srcdir) 

bsc_t_SOURCES = check_bsc.c beanstalkclient.h
bsc_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
bsc_t_LDADD   = @CHECK_LIBS@ $(srcdir)/*.o

ivector_t_SOURCES = check_ivector.c ivector.h
ivector_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
ivector_t_LDADD   = @CHECK_LIBS@ $(srcdir)/*.o

commands_t_SOURCES = check_commands.c beanstalkproto.h
commands_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
commands_t_LDADD   =  @CHECK_LIBS@ $(srcdir)/beanstalkproto.o

responses_t_SOURCES = check_responses.c beanstalkproto.h
responses_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
responses_t_LDADD   =  @CHECK_LIBS@ $(srcdir)/beanstalkproto.o

stats_t_SOURCES = check_stats.c beanstalkproto.h
stats_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
stats_t_LDADD   =  @CHECK_LIBS@ $(srcdir)/beanstalkproto.o

ioqueue_t_SOURCES = check_ioqueue.c ioqueue.h
ioqueue_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
ioqueue_t_LDADD   = @CHECK_LIBS@ $(srcdir)/ioqueue.o
