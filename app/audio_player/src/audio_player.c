/****************************************************************************
 * audio_player.c - Lightweight audio player daemon
 *
 * Monitors /data/play_trigger. When triggered, plays WAV via
 * /dev/audio/pcm0 (AUDCODEC DAC -> PA42 Class-D PA -> speaker).
 * Falls back to generating a test tone if WAV file is invalid.
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <math.h>

#include <nuttx/audio/audio.h>

#define TRIGGER_FILE  "/data/play_trigger"
#define STATUS_FILE   "/data/play_status"
#define DEFAULT_WAV   "/data/cicada.wav"
#define AUDIO_DEVICE  "/dev/audio/pcm0"

/* Minimal WAV header (16-bit PCM) */

struct wav_header_s
{
  uint32_t riff_id;
  uint32_t riff_size;
  uint32_t wave_id;
  uint32_t fmt_id;
  uint32_t fmt_size;
  uint16_t audio_format;
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
  uint32_t data_id;
  uint32_t data_size;
};

static volatile int g_running = 1;

static void sig_handler(int sig)
{
  (void)sig;
  g_running = 0;
}

static void write_status(const char *status)
{
  int fd = open(STATUS_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd >= 0)
    {
      write(fd, status, strlen(status));
      close(fd);
    }
}

/* Configure audio device via NuttX audio ioctls */

static int audio_configure(int fd, int sample_rate, int channels, int bps)
{
  struct audio_caps_s caps;
  int ret;

  memset(&caps, 0, sizeof(caps));
  caps.ac_len = sizeof(caps);
  caps.ac_type = AUDIO_TYPE_OUTPUT;
  caps.ac_subtype = AUDIO_FMT_PCM;
  caps.ac_controls.hw[0] = sample_rate;
  caps.ac_controls.b[2] = bps;
  caps.ac_channels = channels;

  printf("[audio_player] Configuring: %dHz %dch %dbit\n", sample_rate, channels, bps);

  ret = ioctl(fd, AUDIOIOC_CONFIGURE, (unsigned long)&caps);
  printf("[audio_player] AUDIOIOC_CONFIGURE returned: %d (errno=%d)\n", ret, errno);
  if (ret < 0)
    {
      return -errno;
    }

  ret = ioctl(fd, AUDIOIOC_START, 0);
  printf("[audio_player] AUDIOIOC_START returned: %d (errno=%d)\n", ret, errno);
  if (ret < 0)
    {
      return -errno;
    }

  return 0;
}

/* Generate a simple sine wave tone (440Hz) */

static int play_tone(int duration_ms)
{
  int audio_fd;
  int16_t buf[512];
  int sample_rate = 8000;
  int total_samples = sample_rate * duration_ms / 1000;
  int i;

  printf("[audio_player] Playing test tone: 440Hz, %dms\n", duration_ms);

  audio_fd = open(AUDIO_DEVICE, O_WRONLY);
  if (audio_fd < 0)
    {
      printf("[audio_player] Cannot open %s: %d\n", AUDIO_DEVICE, errno);
      return -errno;
    }

  /* Configure audio device */

  audio_configure(audio_fd, sample_rate, 1, 16);

  for (i = 0; i < total_samples && g_running; i += 512)
    {
      int chunk = total_samples - i;
      if (chunk > 512)
        chunk = 512;

      int j;
      for (j = 0; j < chunk; j++)
        {
          double t = (double)(i + j) / (double)sample_rate;
          buf[j] = (int16_t)(16000.0 * sin(2.0 * 3.14159 * 440.0 * t));
        }

      write(audio_fd, buf, chunk * sizeof(int16_t));
      usleep(chunk * 1000000 / sample_rate);
    }

  ioctl(audio_fd, AUDIOIOC_STOP, 0);
  close(audio_fd);
  printf("[audio_player] Tone complete\n");
  return 0;
}

/* Play WAV file */

static int play_wav(const char *path)
{
  int fd;
  int audio_fd;
  struct wav_header_s hdr;
  ssize_t n;
  uint8_t buf[4096];

  printf("[audio_player] Playing %s\n", path);

  fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      printf("[audio_player] Cannot open %s: %d, playing tone instead\n",
             path, errno);
      return play_tone(2000);
    }

  n = read(fd, &hdr, sizeof(hdr));
  if (n != sizeof(hdr) || hdr.riff_id != 0x46464952)
    {
      printf("[audio_player] Invalid WAV header (read %zd bytes), playing tone\n", n);
      close(fd);
      return play_tone(2000);
    }

  printf("[audio_player] WAV: %ldHz %d-bit %dch, data=%lu bytes\n",
         (long)hdr.sample_rate, hdr.bits_per_sample,
         hdr.num_channels, (unsigned long)hdr.data_size);

  if (hdr.data_size < 100)
    {
      printf("[audio_player] WAV data too small (%lu bytes), playing tone\n",
             (unsigned long)hdr.data_size);
      close(fd);
      return play_tone(2000);
    }

  audio_fd = open(AUDIO_DEVICE, O_WRONLY);
  if (audio_fd < 0)
    {
      printf("[audio_player] Cannot open %s: %d\n", AUDIO_DEVICE, errno);
      close(fd);
      return -errno;
    }

  /* Configure audio device */

  audio_configure(audio_fd, hdr.sample_rate, hdr.num_channels, hdr.bits_per_sample);

  write_status("playing");
  uint32_t remaining = hdr.data_size;

  while (remaining > 0 && g_running)
    {
      size_t to_read = remaining > sizeof(buf) ? sizeof(buf) : remaining;
      n = read(fd, buf, to_read);
      if (n <= 0)
        break;

      ssize_t written = write(audio_fd, buf, n);
      if (written < 0)
        {
          printf("[audio_player] Audio write error: %d\n", errno);
          break;
        }

      usleep((n * 1000000) / hdr.byte_rate);
      remaining -= n;
    }

  ioctl(audio_fd, AUDIOIOC_STOP, 0);
  close(audio_fd);
  close(fd);
  write_status("idle");

  printf("[audio_player] Playback complete\n");
  return 0;
}

static int check_trigger(void)
{
  int fd = open(TRIGGER_FILE, O_RDONLY);
  if (fd < 0)
    return 0;

  char buf[256];
  int n = read(fd, buf, sizeof(buf) - 1);
  close(fd);

  if (n > 0)
    {
      buf[n] = '\0';
      unlink(TRIGGER_FILE);
      return 1;
    }

  return 0;
}

int main(int argc, char *argv[])
{
  const char *wav_path = DEFAULT_WAV;

  if (argc > 1)
    wav_path = argv[1];

  printf("[audio_player] Starting, WAV: %s\n", wav_path);
  write_status("idle");

  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  mkdir("/data", 0755);

  while (g_running)
    {
      if (check_trigger())
        {
          play_wav(wav_path);
        }

      usleep(500000);
    }

  printf("[audio_player] Exiting\n");
  return 0;
}
