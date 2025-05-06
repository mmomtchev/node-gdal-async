/**
 * @file utime.c
 *
 * @author Thomas Jahns, Deutsches Klimarechenzentrum
 *
 * @section LICENSE
 * Copyright 2024 Mathis Rosenhauer, Moritz Hanke, Joerg Behrens, Luis Kornblueh
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @section DESCRIPTION
 *
 * Simple timing command, since calling time(1) gives non-portable results.
 *
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

static int
run_cmd(int argc, char *argv[]);

int main(int argc, char **argv)
{
  struct timeval utime = { .tv_sec = 0, .tv_usec = 0 };
  int rstatus;
  int status = 0;
  if (argc > 1 && ((status = run_cmd(argc - 1, argv + 1)) >= 0))
  {
    struct rusage usage;
    if ((rstatus = getrusage(RUSAGE_CHILDREN, &usage) == -1))
    {
      perror("resource usage statistics unavailable");
    }
    else if (rstatus != 0)
    {
      fputs("an unknown error occurred\n", stderr);
      return EXIT_FAILURE;
    }
    else
    {
      utime = usage.ru_utime;
    }
  }
  else if (status)
  {
    fputs("could not fork child\n", stderr);
    return EXIT_FAILURE;
  }
  fprintf(stderr, "%f\n", (utime.tv_sec * 1000000 + utime.tv_usec)/1000000.0);
  return status;
}

static int
run_cmd(int argc, char *argv[])
{
  int status;
  pid_t child_pid;
  if (argc < 1)
    return -1;
  if ((child_pid = fork()) < 0)
    status = -1;
  else if (child_pid == 0)
  {
    /* child */
    execvp(argv[0], argv);
    _exit(127); /* execvp should not have returned */
  }
  else
  {
    while (waitpid(child_pid, &status, 0) < 0)
      if (errno != EINTR)
      {
        status = -1;
        break;
      }
  }
  return status;
}
