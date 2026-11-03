/****************************************************************************
 * app/timesetd/timesetd_main.c
 *
 * File-IPC system time setter daemon for contest2026_090 / MaruHome.
 *
 * Watches:
 *   /data/tmp/settime.req   (create by quickapp or nsh)
 * Payload (one of):
 *   1795123456                 unix seconds
 *   2026-11-03 17:30:00        local time string
 * Then:
 *   settimeofday()
 *   write /data/tmp/settime.ack  "OK" | "FAIL" | "BAD"
 *   unlink request file
 *
 * /data is a writable mount on this board; do not use /etc/data.
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

#define REQ_PATH "/data/tmp/settime.req"
#define ACK_PATH "/data/tmp/settime.ack"
#define DIR_PATH "/data/tmp"
#define POLL_US  500000  /* 500ms */

static void write_ack(const char *msg)
{
  int fd = open(ACK_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      return;
    }

  dprintf(fd, "%s\n", msg);
  close(fd);
}

static int parse_payload(const char *buf, struct timeval *tv)
{
  char *end = NULL;
  long long sec;

  /* Format 1: pure unix seconds */
  sec = strtoll(buf, &end, 10);
  if (end != NULL && *end == '\0' && sec > 1000000000LL)
    {
      tv->tv_sec = (time_t)sec;
      tv->tv_usec = 0;
      return 0;
    }

  /* Format 2: YYYY-MM-DD HH:MM:SS (local) */
  {
    struct tm tm;
    time_t t;

    memset(&tm, 0, sizeof(tm));
    if (strptime(buf, "%Y-%m-%d %H:%M:%S", &tm) != NULL)
      {
        t = mktime(&tm);
        if (t > 0)
          {
            tv->tv_sec = t;
            tv->tv_usec = 0;
            return 0;
          }
      }
  }

  return -1;
}

int main(int argc, char *argv[])
{
  printf("[timesetd] start, watch %s\n", REQ_PATH);

  mkdir(DIR_PATH, 0777);

  for (; ; )
    {
      int fd;
      char buf[64];
      ssize_t n;

      fd = open(REQ_PATH, O_RDONLY);
      if (fd < 0)
        {
          usleep(POLL_US);
          continue;
        }

      memset(buf, 0, sizeof(buf));
      n = read(fd, buf, sizeof(buf) - 1);
      close(fd);

      if (n <= 0)
        {
          usleep(POLL_US);
          continue;
        }

      /* consume request first to avoid re-entry */
      unlink(REQ_PATH);

      {
        struct timeval tv;

        if (parse_payload(buf, &tv) == 0)
          {
            if (settimeofday(&tv, NULL) == 0)
              {
                printf("[timesetd] set time -> %lld\n",
                       (long long)tv.tv_sec);
                write_ack("OK");
              }
            else
              {
                printf("[timesetd] settimeofday errno=%d\n", errno);
                write_ack("FAIL");
              }
          }
        else
          {
            printf("[timesetd] bad payload: %s\n", buf);
            write_ack("BAD");
          }
      }

      usleep(POLL_US);
    }

  return 0;
}
