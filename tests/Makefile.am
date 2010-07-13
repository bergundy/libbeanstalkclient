TESTS = evbsc.t
check_PROGRAMS = $(TESTS)
ALL_LIBS = -lev -lbeanstalkclient -lsockutils -lioqueue

srcdir = $(top_builddir)/src
AM_CFLAGS = -I$(srcdir) 

evbsc_t_SOURCES = check_evbsc.c evbsc.h
evbsc_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
evbsc_t_LDADD   = @CHECK_LIBS@ $(srcdir)/*.o
