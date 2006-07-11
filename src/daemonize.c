#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <daemonize.h>
 
int daemonize()
{
  int err = -1;
  pid_t pid;
  /*int i;*/
  int fd = -1;
     
  pid = fork();
  if (pid == -1) {
    fprintf(stderr, "fork() failed: %s", strerror(errno));
    goto e;
  }
  if (pid != 0) {
    /* TODO: Hmmm... what about atexit callbacks? */
    exit(0);
  }
     
  /* Get rid of controlling terminal. */
  setsid();
     
  /* http://www.unixguide.net/unix/programming/1.7.shtml says, that it's
   * good idea to fork again... so be it! */
  pid = fork();
  if (pid == -1) {
    fprintf(stderr, "fork() failed: %s", strerror(errno));
    goto e;
  }
  if (pid != 0) {
    /* TODO: Hmmm... what about atexit callbacks? */
    exit(0);
  }
     
  /* It's not good idea to stay in old working directory. Don't care 
   * about errors though... */

  /* It's cumbersome to run from */
#if 0
  chdir("/");
#endif
     
  /* We dont do umask(0) here, as suggested by
   * http://www.unixguide.net/unix/programming/1.7.shtml */

  /* Don't want to close all files! because output file would be closed too */
#if 0
  /* Close all files except stdin, stdout and stderr. */
  for (i = getdtablesize(); i > 2; --i) {
    for (;;) {
      if (!close(i)) {
	break;
      }
      if (errno != EINTR) {
	break;
      }
    }
  }
#endif

#if 0     
  /* Open /dev/null for redirections. */
  for (;;) {
    fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
      break;
    }
    if (errno != EINTR) {
      fprintf(stderr, "open(\"%s\") failed: %s", "/dev/null", strerror(errno));
      goto e;
    }
  }

  /* Redirect stdin, stderr, but not stdout. */
  for (i = 0; i < 3; ++i) {
    for (;;) {
      if (dup2(fd, i) != -1) {
	break;
      }
      if (errno != EINTR) {
	fprintf(stderr, "dup2() failed for %d: %s", i, strerror(errno));
	goto e;
      }
    }
  }
#endif
  err = 0;
     
 e:
     
  if (fd != -1) {
    for (;;) {
      if (!close(fd)) {
	break;
      }
      if (errno != EINTR) {
	fprintf(stderr, "close() failed for /dev/null: %s", strerror(errno));
	err = -1;
	break;
      }
    }
  }
     
  return err;
}
