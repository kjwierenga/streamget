

bin_PROGRAMS = \
	streamget

streamget_SOURCES = \
	lock.h \
	lock.c \
	daemonize.h \
	daemonize.c \
	url_fopen.h \
	url_fopen.c \
	main.c

BUILT_SOURCES = \
	git-ref.h

git-ref.h: main.c
	/bin/echo -n '#define GIT_REF ' > git-ref.h && cat ../.git/FETCH_HEAD | cut -f1 >> git-ref.h
	
CLEANFILES = $(BUILT_SOURCES)

streamget_LDADD = @LIBCURL@

