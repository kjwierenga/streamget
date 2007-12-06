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
#include <ctype.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <url_fopen.h>
#include <daemonize.h>
#include "svnrev.h"
#include "config.h"

#define MAXTIMESTR 32

#define GETTIMESTR \
time_t _now_ = time(0); char _timestr_[MAXTIMESTR]; \
(void)strftime(_timestr_, MAXTIMESTR, "%b %d %H:%M:%S ", localtime(&_now_));

/* VERBOSE macro */
#define LOGINFO1(stream, format, arg1)							\
do {											\
GETTIMESTR										\
if (g_options.verbose > 0) { fprintf(stream, "%s " format, _timestr_, (arg1)); }	\
} while (0)

#define LOGINFO2(stream, format, arg1, arg2)							\
GETTIMESTR											\
do {												\
if (g_options.verbose > 0) { fprintf(stream, "%s " format, _timestr_, (arg1), (arg2)); }	\
} while (0)

#define LOGINFO3(stream, format, arg1, arg2, arg3)							\
GETTIMESTR												\
do {													\
if (g_options.verbose > 0) { fprintf(stream, "%s " format, _timestr_, (arg1), (arg2), (arg3)); }	\
} while (0)

#define LOGINFO4(stream, format, arg1, arg2, arg3, arg4)							\
do {														\
GETTIMESTR													\
if (g_options.verbose > 0) { fprintf(stream, "%s " format, _timestr_, (arg1), (arg2), (arg3), (arg4)); }	\
} while (0)

/* local definitions */
#define SVNID       "$Id$"
#define SVNDATE     "$Date$"
#define SVNREVISION "$Revision$";

#define BUFFERSIZE                (8 * 1024) /* read/write in these chunks */
#define DEFAULT_TIME_LIMIT        (4 * 3600) /* (sec) four hours */
#define DEFAULT_CONNECT_TIMEOUT   (20)       /* (sec) twenty seconds */
#define DEFAULT_CONNECT_PERIOD    (-1)       /* (sec) -1 means inifinite */
#define DEFAULT_RECONNECT_TIMEOUT (1)        /* (sec) 1 second */
#define DEFAULT_RECONNECT_PERIOD  (-1)       /* (sec) -1 means infinite */

/* local typedefs */
typedef struct {

  /* URL from which to read stream */
  char* url;

  /* name of the output file */
  char* output;

  /* name and handle of the logfile */
  char* logname;
  FILE* log;

  /* (sec) Time-limit in seconds */
  int time_limit;

  /* boolean Time from connect, if set to 1 start the time-limit
   * timer when first succesfully retrieved and written data from
   * the stream. Default is to start the time-limit timer when the
   * program starts.
   */
  int time_from_connect;

  /* (sec) Time between initial connects if stream not yet available. */
  int connect_timeout;

  /* (sec) How long to try initial succesful initial connect */
  int connect_period;

  /* countdown */
  int connect_countdown;
  
  /* (sec) Time between reconnects if stream drops. */
  int reconnect_timeout;

  /* (sec) How long to try reconnecting after dropped connection. */
  int reconnect_period;

  /* reconnect countdown */
  int reconnect_countdown;

  /* show prgress yes/no */
  int progress;

  /* verbose yes/no */
  int verbose;

  /* daemonize */
  int daemonize;

} StreamgetOptions;

/* local function */
static void      sg_usage(FILE* ostream);
static void      sg_alrm(int);
static int       sg_sleep(time_t seconds);
static int       sg_set_alarm(int timeout);
static void      sg_reset_countdown(StreamgetOptions* options);
static int       sg_open_logfile(StreamgetOptions* options);
static int       sg_parse_options(int argc, char** argv, StreamgetOptions* options);
static int       sg_mainloop(void);

/* global variables */
static char* g_useragent = "Streamget/" VERSION " (Rev " SVN_REVSTR ")";

/* global variable to hold options */
static StreamgetOptions g_options = {
  NULL, /* no URL specified */
  NULL, /* no output FILENAME specified */
  NULL, /* no logname set */
  NULL, /* no logging */
  DEFAULT_TIME_LIMIT,
  0, /* start time-limit timer when program starts */
  DEFAULT_CONNECT_TIMEOUT,
  DEFAULT_CONNECT_PERIOD,
  0,
  DEFAULT_RECONNECT_TIMEOUT,
  DEFAULT_RECONNECT_PERIOD,
  0,
  0, /* don't show progress */
  0, /* don't be verbose */
  0, /* do not daemonize */
};

void print_options(StreamgetOptions* options)
{
  if (!options) return;

  LOGINFO1(stdout, "url                : %s\n", options->url);
  LOGINFO1(stdout, "output             : %s\n", options->output);
  LOGINFO1(stdout, "log                : %s\n", options->logname ? options->logname : "<not set>");
  LOGINFO1(stdout, "time-limit         : %d seconds\n", options->time_limit);
  LOGINFO1(stdout, "time-from-connect  : %s\n", options->time_from_connect ? "yes" : "no");
  LOGINFO1(stdout, "connect-timeout    : %d seconds\n", options->connect_timeout);
  LOGINFO1(stdout, "connect-period     : %d seconds\n", options->connect_period);
  LOGINFO1(stdout, "connect-countdown  : %d seconds\n", options->connect_countdown);
  LOGINFO1(stdout, "reconnect-timeout  : %d seconds\n", options->reconnect_timeout);
  LOGINFO1(stdout, "reconnect-period   : %d seconds\n", options->reconnect_period);
  LOGINFO1(stdout, "reconnect-countdown: %d seconds\n", options->reconnect_countdown);
  LOGINFO1(stdout, "progress           : %s\n", options->progress ? "yes" : "no");
  LOGINFO1(stdout, "verbose            : %d (level)\n", options->verbose);
  LOGINFO1(stdout, "daemonize          : %s\n", options->daemonize ? "yes" : "no");
}

static void sg_reset_countdown(StreamgetOptions* options)
{
  if (!options) return;

  options->connect_countdown = (options->connect_period > 0
				? options->connect_period / options->connect_timeout
				: -1);
  options->reconnect_countdown = (options->reconnect_period > 0 
				  ? options->reconnect_period / options->reconnect_timeout
				  : -1);
}

static int sg_open_logfile(StreamgetOptions* options)
{
  int i = -1;

  if (!options || !options->logname) return 0;

  options->log = fopen(options->logname, "a");
  if (!options->log) {
    fprintf(stderr, "Error: couldn't open log file '%s'\n%s\n",
	    options->logname, strerror(errno));

    return -2;
  }

  /* set unbuffered mode */
  (void)setvbuf(options->log, NULL, _IOLBF, 0);
  (void)setvbuf(stdout, NULL, _IOLBF, 0);
  (void)setvbuf(stderr, NULL, _IOLBF, 0);
  
  /* redirect stdin, stdout and stderr to logfile */
  for (i = 0; i < 3; ++i) {
    for (;;) {
      if (dup2(fileno(options->log), i) != -1) {
	break;
      }
      if (errno != EINTR) {
	fprintf(stderr, "dup2() failed for %d: %s", i, strerror(errno));
	return -4;
      }
    }
  }

  return 0;
}

static int sg_parse_options(int argc, char** argv, StreamgetOptions* options)
{
  int retval = 1;
  int c;

  if (!options) return 0;

  opterr=0;
  while (1) {
    int option_index = 0;

    static struct option long_options[] = {
      { "url",               required_argument, 0, 'u' },
      { "output",            required_argument, 0, 'o' },
      { "log",               required_argument, 0, 'l' },
      { "time-limit",        required_argument, 0, 's' },
      { "time-from-connect", no_argument,       0, 'x' },
      { "connect-timeout",   required_argument, 0, 'c' },
      { "connect-period",    required_argument, 0, 't' },
      { "reconnect-timeout", required_argument, 0, 'r' },
      { "reconnect-period",  required_argument, 0, 'e' },
      { "progress",          no_argument,       0, 'p' },
      { "daemonize",         no_argument,       0, 'd' },
      { "verbose",           no_argument,       0, 'v' },
      { "help",              no_argument,       0, 'h' },
      { "version",           no_argument,       0, 'V' },
      { 0, 0, 0, 0 },
    };

    c = getopt_long(argc, argv, "u:o:l:s:xc:t:r:e:pdvhV",
		    long_options, &option_index);
    if (c == -1) break;

    switch (c) {
    case 'u':
      options->url = optarg;
      break;

    case 'o':
      options->output = optarg;
      break;

    case 'l':
      options->verbose++; /* raise the verbosity level */
      options->logname = optarg;
      break;

    case 's':
      options->time_limit = abs(atoi(optarg));
      break;

    case 'x':
      options->time_from_connect = 1;
      break;

    case 'c':
      options->connect_timeout = abs(atoi(optarg));
      if (options->connect_timeout <= 0) {
	fprintf(stderr, "Error: invalid value for 'connect-timeout': %d\n", options->connect_timeout);
	retval=0;
      }
      break;

    case 't':
      options->connect_period = abs(atoi(optarg));
      if (options->connect_period <= 0) {
	fprintf(stderr, "Error: invalid value for 'connect-period': %d\n", options->connect_period);
	retval=0;
      }
      break;
      
    case 'r':
      options->reconnect_timeout = abs(atoi(optarg));
      if (options->reconnect_timeout <= 0) {
	fprintf(stderr, "Error: invalid value for 'reconnect-timeout': %d\n", options->reconnect_timeout);
	retval=0;
      }
      break;

    case 'e':
      options->reconnect_period = abs(atoi(optarg));
      if (options->reconnect_period <= 0) {
	fprintf(stderr, "Error: invalid value for 'reconnect-period': %d\n", options->reconnect_period);
	retval=0;
      }
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

    case 'V':
      fprintf(stdout, "streamget " VERSION " (Rev " SVN_REVSTR ")\n");
      exit(EXIT_SUCCESS);
      break;

    case ':':
      fprintf(stderr, "Error: missing value for option '%s'\n", argv[optind-1]);
      retval=0;
      break;

    default:
      fprintf(stderr, "Error: unknown option '%s'\n", argv[optind-1]);
      /*getopt returned unrecognised character code 0%o\n", c);*/
      retval=0;
      break;
    }
  }

  /* open log output */
  if (sg_open_logfile(options) < 0) {
    return 0;
  }

  /* reset countdown values */
  sg_reset_countdown(options);
   
  if (optind < argc) {
    retval=0;
    fprintf(stderr, "Error: unrecognised arguments: ");
    while (optind < argc) {
      fprintf (stderr, "%s", argv[optind++]);
    }
    fprintf(stderr, "\n");
  }

  return retval;
}

void sg_usage(FILE* ostream)
{
  fprintf(ostream, "\nstreamget " VERSION " (Rev " SVN_REVSTR ")\n\
    --url              |-u URL       # URL to get\n\
    --output           |-o FILENAME  # file to append output to\n\
   [--log              |-l FILENAME] # output logging to this file, raise verbosity level by 1\n\
   [--time-limit       |-s 4*3600]   # in secs, limit recording time, -1=infinte)\n\
   [--time-from-connect|-x]          # start the time-limit timer when first connected\n\
                                        default is to start timer when the program starts\n\
   [--connect-timeout  |-c 20]       # in secs, time between initial connect attempts)\n\
   [--connect-period   |-t 600]      # in secs, total period to try to connect, default is infinte)\n\
   [--reconnect-timeout|-r 1]        # in secs, time between reconnect attempts)\n\
   [--reconnect-retries|-e 600]      # in secs, total period to try to connect, default is infinte)\n\
   [--progress         | -p]         # show progress meter\n\
   [--daemonize        | -d]         # start the process in the background\n\
   [--verbose          | -v]         # increase verbosity level by 1,2, etc e.g. -v, -vv, -vvv, etc.\n\
   [--help             | -h]         # this help text\n\
   [--version          | -V]         # print version of the program\n\
");
}

/*
 * SIGALRM handler.
 */
static void sg_alrm(int signo)
{
  if (g_options.verbose > 0) {
    time_t now = time(0);

    /* \n omitted intentionally, provided by ctime() */
    LOGINFO2(stdout, "Time limit of %d seconds expired at %s",
	     g_options.time_limit, ctime(&now));
    fsync(fileno(stdout));
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
  return alarm(timeout);
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
  char buffer[BUFFERSIZE];
  
  /* defined valid states */
  enum { IDLE, CONNECTING, CONNECTED, RECONNECTING, RECONNECTED, DONE };
  int state = IDLE;

  /* Start time-limit timer, if required */
  if (!g_options.time_from_connect) {
    time_t now = time(0) + g_options.time_limit;

    /* \n omitted intentionally, provided by ctime() */
    LOGINFO2(stdout, "Time limit set to %d seconds, expires at %s",
	     g_options.time_limit, ctime(&now));
    (void)sg_set_alarm(g_options.time_limit);
  }

  int* countdown = &g_options.connect_countdown;

  /* try forever or until (re)connect periods or the time limit expire */
  while (1) {

    /* open URL */
    if (handle) url_fclose(handle);
    handle = url_fopen(g_options.url, "r", g_useragent);

    if (handle) {

      /* set options */
      if (g_options.verbose > 1) {
	url_setverbose(handle, g_options.verbose);
      }

      /* first read */
      nread = url_fread(buffer, 1, sizeof(buffer), handle);

      /* be verbose if data was read */
      if (nread) url_setprogress(handle, g_options.progress);

      /* set alarm for time limit when first data is written */
      if (nread > 0) {
	LOGINFO2(stdout, "Stream '%s' %s.\n", g_options.url, nwritten ? "reconnected" : "active");

	/* update state */
	if (0 == nwritten) state = CONNECTED;
	else               state = RECONNECTED;

	sg_reset_countdown(&g_options);

	if (CONNECTED == state) {

	  /*
	   * Open output file late (when first data is about to be written,
	   * to prevent creating an empty file when the source is not yet active.
	   */
	  outf = fopen(g_options.output, "a");
	  if(!outf) {
	    fprintf(stderr, "Error: couldn't open output file '%s'\n%s.\n",
		    g_options.output, strerror(errno));
	    retval = 2;
	    goto exit;
	  }

	  /* start time-limit timer if required */
	  if (g_options.time_from_connect) {
	    time_t now = time(0) + g_options.time_limit;

	    /* \n omitted intentionally, provided by ctime() */
	    LOGINFO2(stdout, "Starting time-limit timer of %d seconds, will expire at %s",
		     g_options.time_limit, ctime(&now));
	    (void)sg_set_alarm(g_options.time_limit);
	  }
	}
      }

      while (nread) {
	if (nread != (nwritten_now = fwrite(buffer, 1, nread, outf))) {
	  LOGINFO2(stdout, "Error writing to file '%s' : %s.\n",
		  g_options.output, strerror(errno));
	  fprintf(stderr, "Error writing to file '%s': %s.\n",
		  g_options.output, strerror(errno));
	  retval = 4;
	  goto exit;
	}
	nwritten += nwritten_now;
	nread = url_fread(buffer, 1, sizeof(buffer), handle);
      }
    }

    if (*countdown < 0 || --*countdown > 0) {
      if (nwritten <= 0) {
	if (CONNECTING != state) {
	  LOGINFO1(stdout, "Stream '%s' not active.\n", g_options.url);
	}
	/* update state */
	state = CONNECTING;
	sg_sleep(g_options.connect_timeout);
      } else {
	countdown = &g_options.reconnect_countdown;
	if (RECONNECTING != state) {
	  LOGINFO2(stdout, "Lost connection. countdown=%d, timeout=%d, Reconnecting...\n",
		   *countdown, g_options.reconnect_timeout);
	}
	/* update state */
	state = RECONNECTING;
	sg_sleep(g_options.reconnect_timeout);
      }

    } else {

      if (nwritten <= 0) {
	LOGINFO2(stdout, "Connect period of %d seconds expired. "
		 "Failed to open URL '%s'.\n", g_options.connect_period, g_options.url);
	state = DONE;
      } else {
	LOGINFO2(stdout, "Reconnect period of %d seconds expired. "
		 "Failed to open URL '%s'.\n", g_options.reconnect_period, g_options.url);
	state = DONE;
      }

      break; // stop recording
    }
  }

  /* close output file */
  if (outf) {
    fclose(outf);
    outf = NULL;
  }

  /* close log output */
  if (g_options.log) {
    fclose(g_options.log);
    g_options.log = NULL;
  }

 exit:
  if (handle)        url_fclose(handle);
  if (outf)          fclose(outf);
  if (g_options.log) fclose(g_options.log);
  return retval;
}
/*
 * Main program 
 * output to two test files (note the fgets method will corrupt binary files if
 * they contain 0 chars */
int main(int argc, char *argv[])
{
  if (!sg_parse_options(argc, argv, &g_options)) {
    sg_usage(stderr);
    exit(EXIT_FAILURE);
  }

  if (g_options.verbose > 1) {
    print_options(&g_options);
  }

  if (!g_options.url) {
    fprintf(stderr, "Error: no URL specified.\n");
    sg_usage(stderr);
    exit(EXIT_FAILURE);
  }
  if (!g_options.output) {
    fprintf(stderr, "Error: no output file specified.\n");
    sg_usage(stderr);
    exit(EXIT_FAILURE);
  }

  /* daemonize if requested */
  if (g_options.daemonize) daemonize();

  /* we got the parameters, get going... */
  return sg_mainloop();
}
