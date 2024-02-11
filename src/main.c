#include "gap_buff.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc <= 1) {
    perror("Error: Not enough arguments");
    exit(1);
  }
  char *file_path = argv[1];
  FILE *f = fopen(file_path, "r+");
  if (f == NULL) {
    perror("Error: Could not open file");
    exit(1);
  }
  GapBuf *buf = new_buffer(2048 * sizeof(char));
  if (!buf) {
    perror("Couldn't allocate buffer");
    exit(1);
  }
  int c;
  while ((c = fgetc(f)) != EOF) {
    insert_character(buf, (char)c);
  }
  // init screen and sets up screen
  initscr();

  // print to screen
  char *text = extract_text(buf);
  addstr(text);

  // refreshes the screen
  refresh();

  // pause the screen output
  getch();

  // deallocates memory and ends ncurses
  endwin();
  free_buffer(buf);
  fclose(f);
  return 0;
}
