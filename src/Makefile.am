LIBBEANSTALKCLIENT_VERSION = 1:0:0

lib_LTLIBRARIES     = libbeanstalkclient.la
libbeanstalkclient_la_SOURCES = beanstalkclient.c ivector.c cbq.c beanstalkproto.c ioqueue.c sockutils.c
include_HEADERS     = beanstalkclient.h ioqueue.h arrayqueue.h
libbeanstalkclient_la_LDFLAGS = -version-info $(LIBBEANSTALKCLIENT_VERSION)
