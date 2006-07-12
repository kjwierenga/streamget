

bin_PROGRAMS = \
	streamget

streamget_SOURCES = \
	daemonize.h \
	daemonize.c \
	url_fopen.h \
	url_fopen.c \
	main.c

BUILT_SOURCES = \
	svnrev.h

svnrev.h: $(streamget_SOURCES)
	svnrev $?

streamget_LDADD = @LIBCURL@

