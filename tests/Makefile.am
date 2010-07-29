TESTS = bsc.t ivector.t
check_PROGRAMS = $(TESTS)

srcdir = $(top_builddir)/src
AM_CFLAGS = -I$(srcdir) 

bsc_t_SOURCES = check_bsc.c beanstalkclient.h
bsc_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
bsc_t_LDADD   = @CHECK_LIBS@ -lbeanstalkproto $(srcdir)/*.o

ivector_t_SOURCES = check_ivector.c ivector.h
ivector_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
ivector_t_LDADD   = @CHECK_LIBS@ $(srcdir)/*.o
