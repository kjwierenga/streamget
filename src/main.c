/*****************************************************************************
 *
 * The streamget program is designed to receive streaming mp3 data from an
 * URL address in a robust manner. When the stream is disconnected (eof
 * condition on the data transfer from the URL), then streamget attempts
 * to reconnect. The transfer stops after a specified time.
 * This program is used to record streaming mp3 casts from an icecast server.
 * The program is usually started from cron at a specific time and continues
 * to record the stream for the specified time.
 *
 * Copyright (c) 2006 AUDIOserver.nl
 * Author: K.J. Wierenga <k.j.wierenga@audioserver.nl>
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This code requires libcurl 7.9.7 or later.
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <url_fopen.h>
#include <daemonize.h>

/* VERBOSE macro */
#define VERBOSE1(stream, format, arg1) \
do { if (g_options.verbose > 0) { fprintf(stream, (format), (arg1)); } } while (0)
#define VERBOSE2(stream, format, arg1, arg2) \
do { if (g_options.verbose > 0) { fprintf(stream, (format), (arg1), (arg2)); } } while (0)

/* local definitions */
#define DEFAULT_TIME_LIMIT        (4 * 3600) /* (sec) four hours */
#define DEFAULT_CONNECT_TIMEOUT   (20)       /* (sec) twenty seconds */
#define DEFAULT_CONNECT_PERIOD    (-1)       /* (sec) -1 means inifinite */
#define DEFAULT_RECONNECT_TIMEOUT (1)        /* (sec) 1 second */
#define DEFAULT_RECONNECT_PERIOD  (-1)       /* (sec) -1 means infinite */
#define DEFAULT_RECONNECT_BACKOFF (0)        /* (sec) add 0 seconds to timeout at each attempt */

/* local function */
static void      sg_usage(FILE* ostream);
static void      sg_alrm(int);
static int       sg_sleep(time_t seconds);
static int       sg_set_alarm(int timeout);
static int       sg_mainloop(void);

/* local typedefs */
typedef struct {

  /* URL from which to read stream */
  char* url;

  /* name of the output file */
  char* filename;

  /* (sec) Time-limit in seconds */
  int time_limit;

  /* (sec) Time between initial connects if stream not yet available. */
  int connect_timeout;

  /* (sec) How long to try initial succesful initial connect */
  int connect_period;
  
  /* (sec) Time between reconnects if stream drops. */
  int reconnect_timeout;

  /* (sec) How long to try reconnecting after dropped connection. */
  int reconnect_period;

  /* (sec) How many seconds to add to reconnect_timeout after each failed attempt.
   * This is used to prevent excessive reconnection attempts which may overload
   * the server.
   */
  int reconnect_backoff;
  
  /* show prgress yes/no */
  int progress;

  /* verbose yes/no */
  int verbose;

  /* daemonize */
  int daemonize;

} StreamgetOptions;

/* global variables */

/* global variable to hold options */
static StreamgetOptions g_options = {
  0, /* no URL specified */
  0, /* no FILENAME specified */
  DEFAULT_TIME_LIMIT,
  DEFAULT_CONNECT_TIMEOUT,
  DEFAULT_CONNECT_PERIOD,
  DEFAULT_RECONNECT_TIMEOUT,
  DEFAULT_RECONNECT_PERIOD,
  DEFAULT_RECONNECT_BACKOFF,
  0, /* don't show progress */
  0, /* don't be verbose */
  0, /* do not daemonize */
};

int parse_options(int argc, char** argv, StreamgetOptions* options)
{
  int retval = 1;
  int c;

  opterr=0;
  while (1) {
    int option_index = 0;

    static struct option long_options[] = {
      { "url",               required_argument, 0, 'u' },
      { "output",            required_argument, 0, 'o' },
      { "time-limit",        required_argument, 0, 'l' },
      { "connect-timeout",   required_argument, 0, 'c' },
      { "connect-period",    required_argument, 0, 't' },
      { "reconnect-timeout", required_argument, 0, 'r' },
      { "reconnect-period",  required_argument, 0, 'e' },
      { "reconnect-backoff", required_argument, 0, 'b' },
      { "progress",          no_argument,       0, 'p' },
      { "daemonize",         no_argument,       0, 'd' },
      { "verbose",           no_argument,       0, 'v' },
      { "help",              no_argument,       0, 'h' },
      { 0, 0, 0, 0 },
    };

    c = getopt_long(argc, argv, ":u:o:l:c:t:r:e:b:pdvh",
		    long_options, &option_index);
    if (c == -1) break;

    switch (c) {
    case 'u':
      options->url = optarg;
      break;

    case 'o':
      options->filename = optarg;
      break;

    case 'l':
      options->time_limit = atoi(optarg);
      break;

    case 'c':
      options->connect_timeout = atoi(optarg);
      break;

    case 't':
      options->connect_period = atoi(optarg);
      break;
      
    case 'r':
      options->reconnect_timeout = atoi(optarg);
      break;

    case 'e':
      options->reconnect_period = atoi(optarg);
      break;

    case 'b':
      options->reconnect_backoff = atoi(optarg);
      break;

    case 'p':
      options->progress = 1;
      break;

    case 'd':
      options->daemonize = 1;
      break;

    case 'v':
      /* set verbose level */
      options->verbose++;
      break;

    case 'h':
      sg_usage(stdout);
      exit(EXIT_SUCCESS);
      break;

    case ':':
      fprintf(stderr, "Error: missing value for option '%s'\n\n", argv[optind-1]);
      retval=0;
      break;

    default:
      fprintf(stderr, "Error: getopt returned unrecognised character code 0%o\n\n", c);
      retval=0;
      break;
    }
  }
   
  if (optind < argc) {
    retval=0;
    fprintf(stderr, "Error: unrecognised arguments: ");
    while (optind < argc) {
      fprintf (stderr, "%s", argv[optind++]);
    }
    fprintf(stderr, "\n\n");
  }

  return retval;
}

void sg_usage(FILE* ostream)
{
  fprintf(ostream, "streamget \n\
    --url              |-u =URL       # URL to get\n\
    --output           |-o =FILENAME  # file to append output to\n\
   [--time-limit       |-l =4*3600]   # in secs, total running time of program, -1=infinte)\n\
   [--connect-timeout  |-c =20]       # in secs, time between initial connect attempts)\n\
   [--connect-period   |-t =-1]       # in secs, total period to try to connect, -1=infinte)\n\
   [--reconnect-timeout|-r =1]        # in secs, time between reconnect attempts)\n\
   [--reconnect-retries|-e =-1]       # in secs, total period to try to connect, -1=infinte)\n\
   [--reconnect-backoff|-b =0]        # in secs, # seconds to add to reconnect-timeout after each attempt)\n\
   [--progress         | -p]          # show progress meter\n\
   [--daemonize        | -d]          # start the process in the background\n\
   [--verbose          | -v]          # increase verbosity level e.g. -v, -vv, -vvv, etc.\n\
   [--help]                           # this help text\n\
");
}

/*
 * SIGALRM handler.
 */
static void sg_alrm(int signo)
{
  if (g_options.verbose > 1) {
    fprintf(stderr, "\nSet recording time of %d seconds expired.\n",
	    g_options.time_limit);
  }
  exit(EXIT_SUCCESS);
}

/*
 * Set alarm to go off after timeout seconds.
 * Install SIGALRM handler.
 */
static int sg_set_alarm(int timeout)
{
  if (SIG_ERR == signal(SIGALRM, sg_alrm)) {
    return 1;
  }
  alarm(timeout);
  return 0;
}

/*
 * Sleep for the specified number of seconds.
 * This function does not interfere with signals.
 */
int sg_sleep(time_t seconds)
{
  int ret = 0;
  struct timespec req;
  struct timespec rem;

  req.tv_sec  =  seconds;
  req.tv_nsec = 0;
  memset(&rem, 0, sizeof(rem));

  do {
    ret = nanosleep(&req, &rem);
    memcpy(&req, &rem, sizeof(struct timespec));
  } while (ret < 0 && EINTR == errno);

  return ret;
}

int sg_mainloop(void)
{
  int retval       = 0; /* assume success */
  URL_FILE *handle = NULL;
  FILE *outf       = NULL;
  int nread        = 0;
  int nwritten     = 0; /* total written bytes written to file */
  int nwritten_now = 0; /* bytes written in one iteration of the loop */
  char buffer[8 * 1024];

  /* open output file */
  outf=fopen(g_options.filename, "a");
  if(!outf) {
    fprintf(stderr, "Error: couldn't open output file '%s'\n%s\n",
	    g_options.filename, strerror(errno));
    retval = 2;
    goto exit;
  }

  int* timeout = &g_options.connect_timeout;
  int* period  = &g_options.connect_period;
  int connect_count = (*period> 0 ? *period / *timeout : -1);

  /* try forever or until (re)connect periods or the time limit expire */
  while (1) {

    /* open URL */
    if (handle) url_fclose(handle);
    handle = url_fopen(g_options.url, "r");

    if (handle) {

      /* set options */
      if (g_options.verbose > 1) {
	url_setverbose(handle, g_options.verbose);
      }

      /* first read */
      nread = url_fread(buffer, 1, sizeof(buffer), handle);

      /* be verbose if data was read */
      if (nread) url_setprogress(handle, g_options.progress);

      /* set alarm for time limit if any data was written */
      if (nread && 0 == nwritten) {
	VERBOSE1(stderr, "Stream active, starting recording which will last %d seconds\n",
		 g_options.time_limit);
	if (sg_set_alarm(g_options.time_limit) < 0) {
	  retval = 3;
	  goto exit;
	}
      }

      while (nread) {
	if (nread != (nwritten_now = fwrite(buffer, 1, nread, outf))) {
	  fprintf(stderr, "Error writing to file '%s': %s.\n",
		  g_options.filename, strerror(errno));
	  retval = 4;
	  goto exit;
	}
	nwritten += nwritten_now;
	nread = url_fread(buffer, 1, sizeof(buffer), handle);
      }
    }

    /*
     * Use reconnect timeout and period once we've written something to file
     * this is no longer concerning the initial connect.
     */
    if (nwritten > 0) {
      timeout = &g_options.reconnect_timeout;
      period = &g_options.reconnect_period;
      connect_count = (*period > 0 ? *period / *timeout : -1);
    }

    /*    if (!handle || url_feof(handle)) { */
    if (connect_count < 0 || --connect_count > 0) {
      if (nwritten <= 0) {
	VERBOSE1(stderr, "Stream not active. Next connect attempt in %d seconds.\n",
		 *timeout);
      } else {
	VERBOSE1(stderr, "Lost connection. Reconnecting in %d seconds.\n", *timeout);
      }
      sg_sleep(*timeout);
    } else {

      if (nwritten <= 0) {
	VERBOSE2(stderr, "Stream not active after connect period of %d seconds expired.\n"
		 "Failed to open URL '%s'\n", *period, g_options.url);
      } else {
	VERBOSE2(stderr, "(Re)connect-period of %d seconds expired.\n"
		 "Failed to open URL '%s'\n", *period, g_options.url);
      }

      break;
    }
    /*}*/
  }

  /* close output file */
  fclose(outf);
  outf = NULL;

 exit:
  if (handle) url_fclose(handle);
  if (outf)   fclose(outf);
  return retval;
}
/*
 * Main program 
 * output to two test files (note the fgets method will corrupt binary files if
 * they contain 0 chars */
int main(int argc, char *argv[])
{
  if (!parse_options(argc, argv, &g_options)) {
    sg_usage(stderr);
    exit(EXIT_FAILURE);
  }

  if (!g_options.url) {
    fprintf(stderr, "Error: no URL specified.\n\n");
    sg_usage(stderr);
    exit(EXIT_FAILURE);
  }
  if (!g_options.filename) {
    fprintf(stderr, "Error: no output file specified.\n\n");
    sg_usage(stderr);
    exit(EXIT_FAILURE);
  }

  if (g_options.daemonize) { 
    daemonize(g_options.verbose);
  }

  /* we got the parameters, get going... */
  return sg_mainloop();
}
