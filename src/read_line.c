/*  
 *  This file © Michael Kerrisk, 2010; originally released under LGPLv3.
 *
 *  Modifications © 2013 N. K. Tilton; modifications released under
 *  LGPLv3.
 *
 */

#include "vichess.h"

/* 
 *  Read characters from 'fd' until a certain delimiting character is
 *  encountered. 
 *  
 *  If the delimiting character is not encountered in the first (n - 1)
 *  bytes, then the excess characters are discarded. The returned string
 *  placed in 'buf' is null-terminated and includes the newline
 *  character if it was read in the first (n - 1) bytes. The function
 *  return value is the number of bytes placed in buffer (which includes
 *  the newline character if encountered, but excludes the terminating
 *  null byte). 
 * 
 * (returns NULed string)
 *
 */

ssize_t read_line(int fd, void *buffer, size_t n)
{
  ssize_t   n_read; // # of bytes fetched by last read()
  size_t    total;  // bytes read so far
  char      *buf;
  char      ch;

  if (n <= 0 || buffer == NULL)
  {
    errno = EINVAL;
    return -1;
  }

  // No pointer arithmetic on (void *) ...
  buf   = buffer;
  total = 0;

  while (true) 
  {
    n_read = read(fd, &ch, 1); // read a single character

    if (n_read == -1)
    {
      if (errno == EINTR) { continue;   } // interrupted; restart read()
      else                { return -1;  } // other error
    } 
    else if (n_read == 0) // EOF
    {
      if (total == 0)   { return 0; }   // nothing read
      else              { break;    }   // something read, append '\0'
    } 
    else // n_read == 1
    {
      if (n_read < (n - 1)) // discard > (n - 1) bytes
      {
        total++;
        *buf++ = ch;
      }
      if (ch == '\r') { break; }
    }
  }
  *buf = '\0';

  return total;
}
