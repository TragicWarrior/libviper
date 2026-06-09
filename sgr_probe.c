/* sgr_probe.c -- THROWAWAY spike, delete before any parser code lands.
 *
 * Purpose: settle the one load-bearing assumption behind self-parsing
 * SGR mouse sequences -- with mousemask(0), does ncurses hand the raw
 * \033[<...M bytes to getch(), or does it still claim them as KEY_MOUSE?
 *
 * Build ON THE TEST BOX (it must run in the same SSH session where the
 * mouse misbehaves, so the terminfo / ncurses build match production):
 *
 *     gcc -o sgr_probe sgr_probe.c -lncursesw
 *     ./sgr_probe
 *
 * Then move / click / drag / wheel the mouse.  Press 'q' to quit.
 *
 * The live display draws on the ncurses screen (wiped on exit), so every
 * line is ALSO appended to /tmp/sgr_probe.log -- after you quit, run
 *     cat /tmp/sgr_probe.log
 * and paste that back.
 *
 * Reading the output:
 *   - If moving the mouse prints a burst like
 *         27   <[>   <<>   <3> <5> <;> ... <M>
 *     then ncurses is passing the SGR sequence through as raw bytes.
 *     ASSUMPTION HOLDS -> the parser approach is viable, proceed.
 *   - If it prints
 *         KEY_MOUSE  <-- ncurses CLAIMED it
 *     then ncurses still owns the sequence under mousemask(0).
 *     ASSUMPTION FAILS -> stop and rethink before building the parser.
 *
 * This probe uses a blocking getch() for clean byte-by-byte output.
 * The real library runs nodelay(TRUE); that difference does not affect
 * *whether* ncurses claims the sequence, which is all we are testing.
 */

#include <locale.h>
#include <stdio.h>

#include <ncursesw/curses.h>

#define SGR_PROBE_LOG   "/tmp/sgr_probe.log"

int
main(void)
{
    int     c;
    FILE    *log;

    setlocale(LC_CTYPE, "");

    /* capture file -- the on-screen ncurses output is wiped on endwin() */
    log = fopen(SGR_PROBE_LOG, "w");
    if(log != NULL)
        fprintf(log, "# sgr_probe: raw getch() codes under mousemask(0)\n");

    initscr();
    keypad(stdscr, TRUE);
    noecho();
    raw();

    /* the assumption under test: tell ncurses we want no mouse events */
    mousemask(0, NULL);

    /* replicate the library's terminal setup: any-event tracking (1003h)
       + SGR encoding (1006h), written straight to the tty the way
       vk_kmio_init does via _vk_kmio_write. */
    fputs("\033[?1003h\033[?1006h", stdout);
    fflush(stdout);

    printw("SGR raw-byte probe (mousemask(0)).  Move / click / drag / "
           "wheel the mouse.\r\n");
    printw("Raw getch() codes print below; printable bytes shown in <>. "
           " 'q' quits.\r\n");
    printw("Expect a burst starting  27 <[> <<>  per mouse event if the "
           "bytes flow.\r\n");
    printw("A 'KEY_MOUSE' line means ncurses claimed the sequence "
           "(assumption fails).\r\n");
    printw("Captured to %s -- 'cat' it after quitting.\r\n\r\n",
           SGR_PROBE_LOG);
    refresh();

    for(;;)
    {
        c = getch();

        if(c == ERR) continue;
        if(c == 'q') break;

        if(c == KEY_MOUSE)
        {
            printw("KEY_MOUSE  <-- ncurses CLAIMED it (assumption fails)\r\n");
            if(log != NULL) fprintf(log, "KEY_MOUSE\n");
        }
        else if(c >= 32 && c < 127)
        {
            printw("%4d  <%c>\r\n", c, c);
            if(log != NULL) fprintf(log, "%4d  <%c>\n", c, c);
        }
        else
        {
            printw("%4d\r\n", c);
            if(log != NULL) fprintf(log, "%4d\n", c);
        }

        refresh();
        if(log != NULL) fflush(log);    /* survive a Ctrl-C mid-session */
    }

    /* disable in reverse order, like vk_kmio_shutdown */
    fputs("\033[?1006l\033[?1003l", stdout);
    fflush(stdout);

    endwin();
    if(log != NULL) fclose(log);
    return 0;
}
