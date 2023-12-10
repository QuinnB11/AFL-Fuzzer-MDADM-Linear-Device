#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>
#include <assert.h>

#include "jbod.h"
#include "mdadm.h"
#include "util.h"
#include "tester.h"

#define TESTER_ARGUMENTS "hw:s:"
#define USAGE                                               \
  "USAGE: test [-h] [-w workload-file] [-s cache_size]\n"                   \
  "\n"                                                      \
  "where:\n"                                                \
  "    -h - help mode (display this message)\n"             \
  "\n"                                                      \


/* Utility functions. */
char *stringify(const uint8_t *buf, int length) {
  char *p = (char *)malloc(length * 6);
  for (int i = 0, n = 0; i < length; ++i) {
    if (i && i % 16 == 0)
      n += sprintf(p + n, "\n");
    n += sprintf(p + n, "0x%02x ", buf[i]);
  }
  return p;
}

int run_workload(char *workload, int cache_size);

int main(int argc, char *argv[])
{
  int cache_size;
  char *workload = NULL;

  workload = argv[1];
  cache_size = 1024;
  

  run_workload(workload, cache_size);
  return 0;

    
}

int equals(const char *s1, const char *s2) {
  return strncmp(s1, s2, strlen(s2)) == 0;
}

int run_workload(char *workload, int cache_size) {
  char line[256], cmd[32];
  uint8_t buf[MAX_IO_SIZE];
  uint32_t addr, len, ch;
  int rc;

  memset(buf, 0, MAX_IO_SIZE);

  FILE *f = fopen(workload, "r");
  if (!f)
    err(1, "Cannot open workload file %s", workload);

  
  rc = cache_create(cache_size);
  

  int line_num = 0;
  while (fgets(line, 256, f)) {
    ++line_num;
    line[strlen(line)-1] = '\0';
    if (equals(line, "MOUNT")) {
      rc = mdadm_mount();
    } else if (equals(line, "UNMOUNT")) {
      rc = mdadm_unmount();
    } else if (equals(line, "WRITE_PERMIT")) {
      rc = mdadm_write_permission();
    } else if (equals(line, "WRITE_PERMIT_REVOKE")) {
      rc = mdadm_revoke_write_permission();
    } else if (equals(line, "SIGNALL")) {
      for (int i = 0; i < JBOD_NUM_DISKS; ++i)
        for (int j = 0; j < JBOD_NUM_BLOCKS_PER_DISK; ++j)
          jbod_sign_block(i, j);
    } else {
      if (sscanf(line, "%7s %7u %4u %3u", cmd, &addr, &len, &ch) != 4)
        errx(1, "Failed to parse command: [%s\n], aborting.", line);
      if (equals(cmd, "READ")) {
        rc = mdadm_read(addr, len, buf);
      } else if (equals(cmd, "WRITE")) {
        memset(buf, ch, len);
        rc = mdadm_write(addr, len, buf);
      } else {
        errx(1, "Unknown command [%s] on line %d, aborting.", line, line_num);
      }
    }

    if (rc == -1)
      errx(1, "tester failed when processing command [%s] on line %d", line, line_num);
  }
  fclose(f);

  if (cache_size)
    cache_destroy();

  jbod_print_cost();
  cache_print_hit_rate();

  return 0;
}
