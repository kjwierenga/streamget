

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
	
GIT_REF = $(shell git rev-parse --verify `git symbolic-ref -q HEAD`)

git-ref.h: FORCE
	echo $(GIT_REF) > git-ref.h
	
FORCE:
	
CLEANFILES = $(BUILT_SOURCES)

streamget_LDADD = @LIBCURL@

