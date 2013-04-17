# vichess

A command line interface to the [Free Internet Chess Server
(FICS)](http://www.freechess.org) with (planned) support for vi-like
keybindings.

# Screenshot

![screenshot](https://github.com/noah/vichess/raw/master/vichess.png)
    
# Installation

    % make run

ought to be sufficient on most modern UNIX systems

# Implementation details, architecture

`vichess` is written in C, `-std=c11` to be precise.  

The design of `vichess` is my idea of simple and modular C.  There are 4
basic components:

1. Telnet reader (reads data [tells, challenges, seeks] from FICS
   server)
2. Telnet writer (writes data to FICS server)
3. Ncurses front-end (accepts user input [moves, tells, seeks, etc])
4. Command parser (parses FICS command output)

## `mqueue.h` -- POSIX message queues

The above 4 pieces communicate via two POSIX message queues, which are
intended to be an improvement on the traditional System V message queues
that have existed in UNIX systems, including Linux, since the early
days.  In my opinion, they are an improvement, if for no other reason
than that they eliminate calls to `ftok()`.

The queues effectively behave as a pair of pipes, and indeed unnamed
pipes could be substituted for the queues -- but what fun would that be?
A simplified diagram:

    vichess ----w---> MQ[ib] ----r----> FICS
    vichess <---r---- MQ[ob]  <---w----- FICS

# System requirements and library documentation

To use POSIX message queues, you must be running a newish Linux kernel
(>2.2.6), have mounted `/dev/queue`, have glibc > 2.3.4, and link using
`-lrt`.
[This](http://www.rocket.flyer.co.uk/docs/mqueue-unofficial-HOWTO.html)
is a good read about `mqueue.h`.  There is another excellent guide to
POSIX message queues freely available
[here](http://man7.org/tlpi/download/TLPI-52-POSIX_Message_Queues.pdf).
And while I'm at it, Beej's guides on
[sockets](http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html)
and [IPC, generally](http://beej.us/guide/bgipc/output/html/singlepage/bgipc.html) 
are essential.

Of course, you will need a terminal that supports Unicode, specifically
`utf-8`, and you will need to correctly configure your locale.

# FICS protocols

FICS has pretty good [doc](http://www.freechess.org/Help/AllFiles.html).  The data
exchange format is called
"[Style 12](http://www.freechess.org/Help/HelpFiles/style12.html)", and is
well-documented.

# Portability

In theory, `vichess` is portable to any POSIX system.

# Resource limits

Message queue max sizes are governed by `rlimits`; defaults on my
Archlinux 64-bit system are:

    ⚡ prlimit -q
    RESOURCE DESCRIPTION                  SOFT   HARD UNITS
    MSGQUEUE max bytes in POSIX mqueues 819200 819200 bytes

This total resource limit corresponds to the following invididual
limits:

    ⚡ for f in /proc/sys/fs/mqueue/\*; do echo "$f: $(cat $f)"; done
    /proc/sys/fs/mqueue/msg\_default: 10
    /proc/sys/fs/mqueue/msg\_max: 1000
    /proc/sys/fs/mqueue/msgsize\_default: 8192
    /proc/sys/fs/mqueue/msgsize\_max: 16384
    /proc/sys/fs/mqueue/queues\_max: 256

Note that 819200 bytes is `100*max individual message size`.  This total
is about the size of a floppy disk, so it should be plenty of space for
low-latency messaging.

# Why?

My motivation for writing this was based on a number of things.  First
was the fact that I've been playing a lot of chess on FICS lately, using
the excellent Java client, Jin.  Jin is probably the only piece of
software written in Java that I use on a regular basis, and as nice and
feature-complete as it is, it still sucks and feels incredibly bloaty.

The idea of writing a terminal FICS client that could use the unicode
chess pieces seemed like a good idea but I've been putting it off
because I knew once I started writing it, it would be a drain on my
time.  Mostly because I knew that to pull it off, I'd have to learn
**NCURSES**.

The choice of C is pragmatic.  I would have been happy to write
`vichess` in something else, and I kicked around a Python prototype.
However, I had to give up after I realized that the Python curses
library seems to be missing support for some interesting things I might
want to use, such as *soft keys*.  (Incidentally, I learned about *soft
keys* by reading Dan Gookin's excellent book, "Programmer's Guide to
nCurses".)

So I sucked it up and wrote some C.  I'm happy with the decision, as it
turns out ncurses programming is pretty straightforward.

That's the story of how `vichess` was born.  Now go forth and
checkmate some newbies in full terminal glory!

# Other thoughts

`ncurses` is nice.  Programming in it, and C more generally, feels very
no-nonsense, and very powerful -- no hand-holding, please and thank you.  

`ncurses` can kill you deader than dead (and C, too) -- one example
pitfall:

    char *p = "prompt %";
    printw(p);              // ka-BOOM!
    addstr(p);              // fine
    
Curses has several ways to send strings to the screen, including:

    printw, wprintw, mvprintw, mvwprintw, vwprintw, vw_printw, addch,
    waddch, mvaddch, mvwaddch, echochar, wechochar addstr, addnstr,
    waddstr, waddnstr, mvaddstr, mvaddnstr, mvwaddstr, mvwaddnstr ...
    
For a beginner, it's difficult to determine which of these to use.
`printf`, to which the `printw` family corresponds, is bad enough given
that it is not type safe ... but it's easy to miss that sending an
escape character ('%') to `printw` or similar will lead to all types of
undefined behavior, the same as it will in the case of `printf`.

So the simple rule is:  DO NOT use `printw` to print strings UNLESS
there is an associated format string.  If there is no formatting needed,
`addstr` is probably the correct choice.

# TODO, in no particular order

- [UCI support](http://en.wikipedia.org/wiki/Universal_Chess_Interface) support (play against computer)
- Config scripts (autotools)
- allow user to make moves using keyboard shortcuts (entry is algebraic notation, "b8-a1") / vi keybindings.  right now regular entry works (e.g., "e4", "exd") but it's too slow for blitz
- timer countdown (there are several unix options)
- gui finesse - e.g., highlight last move, show possible moves
- undocumented g1 fields n=noescape, m=?
- undocumented s12 fields
- use iv_compressmove ?
- timeseal
- does "set flip" obviate the need for the n^2 flip code I wrote?
- write some valgrind tests
- reflow on terminal resize (should work in tiling wms)
- some kind of rc file or prompt-based login logic
