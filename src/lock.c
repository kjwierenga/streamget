#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "lock.h"

int lockfd(int fd)
{
  struct flock lock_cmd = {F_WRLCK, 0, SEEK_SET, 0};
  return (fcntl(fd, F_SETLK, &lock_cmd) >= 0); /* return 0 on success, 1 otherwise */
}

int unlockfd(int fd)
{
  struct flock lock_cmd = {F_UNLCK, 0, SEEK_SET, 0};
  return (fcntl(fd, F_SETLK, &lock_cmd) >= 0); /* return 0 on success, 1 otherwise */
}
