TESTS = evbsc.t evvector.t
check_PROGRAMS = $(TESTS)

srcdir = $(top_builddir)/src
AM_CFLAGS = -I$(srcdir) 

evbsc_t_SOURCES = check_evbsc.c evbsc.h
evbsc_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
evbsc_t_LDADD   = @CHECK_LIBS@ $(srcdir)/*.o

evvector_t_SOURCES = check_evvector.c evvector.h
evvector_t_CFLAGS  = @CHECK_CFLAGS@ $(AM_CFLAGS)
evvector_t_LDADD   = @CHECK_LIBS@ $(srcdir)/*.o
