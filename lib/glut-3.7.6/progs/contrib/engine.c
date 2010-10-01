
/* hanoi solver for hanoi2 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

static int num_disks[3] =
{0, 0, 0};

static void
move_disk(int from, int to, int fd)
{
  char buf[3];

  assert(from != to);
  num_disks[from]--;
  num_disks[to]++;
#ifdef TEST_ENGINE
  printf("%d --> %d\n", from, to);
#else
  buf[0] = 'M';
  buf[1] = (char) from;
  buf[2] = (char) to;
  if (3 != write(fd, buf, 3)) {
    perror("can't write");
    exit(1);
  }
#endif
}

static void
move_disks(int from, int to, int n, int fd)
{
  static int other_table[9] =
  {-1, 2, 1, 2, -1, 0, 1, 0, -1};
  int other;

  assert(from != to);
  other = other_table[from * 3 + to];
  assert(other != -1);
  if (n == 1) {
    move_disk(from, to, fd);
  } else {
    move_disks(from, other, n - 1, fd);
    move_disk(from, to, fd);
    move_disks(other, to, n - 1, fd);
  }
}

void
engine(int *args)
{
  num_disks[0] = args[0];
  for (;;) {
    move_disks(0, 2, args[0], args[1]);
    move_disks(2, 0, args[0], args[1]);
  }
}

#ifdef TEST_ENGINE
int
main(int argc, char *argv[])
{
  int engine_args[2];

  if (argc > 1) {
    engine_args[0] = atoi(argv[1]);
  }
  engine_args[1] = 1;
  engine(n, engine_args);
  return 0;             /* ANSI C requires main to return int. */
}
#endif
