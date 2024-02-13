#include <assert.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Escape Sequences
#define ES_ESCAPE "\x1b"
#define ES_BACKSPACE "\x7f"
#define ES_DELETE "\x1b\x5b\x33\x7e"

typedef struct {
	char *items;
	size_t count;
	size_t capacity;
} Data;

typedef struct {
	size_t begin;
	size_t end;
} Line;

typedef struct {
	Line *items;
	size_t count;
	size_t capacity;
} Lines;

#define ITEMS_INIT_CAPACITY (10 * 1024)

#define da_append(da, item)                                               \
  do {                                                                    \
    if ((da)->count >= (da)->capacity) {                                  \
      (da)->capacity =                                                    \
          (da)->capacity == 0 ? ITEMS_INIT_CAPACITY : (da)->capacity * 2; \
      (da)->items =                                                       \
          realloc((da)->items, (da)->capacity * sizeof(*(da)->items));    \
      assert((da)->items != NULL && "Buy more RAM lol");                    \
    }                                                                     \
    (da)->items[(da)->count++] = (item);                                  \
  } while (0)

#define da_reserve(da, desired_capacity)                               \
  do {                                                                 \
    if ((da)->capacity < desired_capacity) {                           \
      (da)->capacity = desired_capacity;                               \
      (da)->items =                                                    \
          realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
      assert((da)->items != NULL && "Buy more RAM lol");                 \
    }                                                                  \
  } while (0)

typedef struct {
	Data data;
	Lines lines;
	size_t cursor;
} wed_Editor;

void wed_free_buffers(wed_Editor *e) {
	free(e->data.items);
	free(e->lines.items);
	e->data.items = NULL;
	e->lines.items = NULL;
}

void wed_load_file(wed_Editor *e, char* file_path) {
	struct stat statbuf;
	if (stat(file_path, &statbuf) < 0) {
		perror("Error: Couldnt Get File Stats");
		exit(1);
	}
	size_t file_size = statbuf.st_size;
	da_reserve(&e->data, file_size);
	e->data.count = file_size;
	int fd = open(file_path, O_RDONLY);
	read(fd, e->data.items, e->data.count);
	close(fd);
}

enum Mode { NORMAL, INSERT };
int main(int argc, char **argv) {
	if (argc <= 1) {
		perror("Error: Not enough arguments");
		exit(1);
	}

	char *file_path = argv[1];
	wed_Editor editor = {0};
	wed_load_file(&editor, file_path);
	// init screen and sets up screen
	initscr();

	// print to screen
	printw("%s", editor.data.items);

	// refreshes the screen
	refresh();

	enum Mode mode = NORMAL;
	int isRunning = 1;
	while (isRunning) {
		// pause the screen output
		char input = getch();
		if (mode == NORMAL) {
			if (input == 'q') {
				isRunning = 0;
			}
		}
	}

	// deallocates memory and ends ncurses
	endwin();
	return 0;
}
