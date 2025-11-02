/*
 * (C) Copyright 2025 Chris January
 */

#include <errno.h>
#include <fcntl.h>
#include <nextp8.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define LOAD_ADDRESS 0x20000
#define BLOCK_SIZE 4096
#define FILENAME "0:/nextp8/nextp8.bin"

#define PROGRESS_BAR_WIDTH (_SCREEN_WIDTH * 3 / 4)
#define PROGRESS_BAR_HEIGHT 16
#define PROGRESS_BAR_LEFT ((_SCREEN_WIDTH - PROGRESS_BAR_WIDTH) / 2)
#define PROGRESS_BAR_TOP ((_SCREEN_HEIGHT -  PROGRESS_BAR_HEIGHT) / 2)
#define LOGO_WIDTH 24
#define LOGO_HEIGHT 5
#define LOGO_LEFT ((_SCREEN_WIDTH - LOGO_WIDTH) / 2)
#define LOGO_TOP (PROGRESS_BAR_TOP - LOGO_HEIGHT - 2)

uint8_t logo[LOGO_HEIGHT][LOGO_WIDTH / 2] = {
    {0x88, 0x00, 0xaa, 0xa0, 0xb0, 0xb0, 0xcc, 0xc0, 0x77, 0x70, 0x77, 0x70},
    {0x80, 0x80, 0xa0, 0x00, 0xb0, 0xb0, 0x0c, 0x00, 0x70, 0x70, 0x70, 0x70},
    {0x80, 0x80, 0xaa, 0x00, 0x0b, 0x00, 0x0c, 0x00, 0x77, 0x70, 0x77, 0x70},
    {0x80, 0x80, 0xa0, 0x00, 0xb0, 0xb0, 0x0c, 0x00, 0x70, 0x00, 0x70, 0x70},
    {0x80, 0x80, 0xaa, 0xa0, 0xb0, 0xb0, 0x0c, 0x00, 0x70, 0x00, 0x77, 0x70}
};

static void draw_logo(int x, int y)
{
    for (int i=0;i<LOGO_HEIGHT;++i) {
        for (int j=0;j<LOGO_WIDTH/2;++j) {
            uint8_t c = logo[i][j];
            uint8_t left = (c >> 4) & 0xf;
            uint8_t right = c & 0xf;
            // transparency
            if (left == 0) left = 1;
            if (right == 0) right = 1;
            c = (right << 4) | left;
            *(uint8_t *)(_BACK_BUFFER_BASE + (y+i) * 64 + (x >> 1) + j) = c;
        }
    }
}

static void draw_rect(int x, int y, int w, int h)
{
  for (int x1=x;x1<x+w;x1+=2) {
      *(uint8_t *)(_BACK_BUFFER_BASE + y*64 + (x1 >> 1)) = 0x77;
      *(uint8_t *)(_BACK_BUFFER_BASE + (y+h-1)*64 + (x1 >> 1)) = 0x77;
  }
  for (int y1=y+1;y1<y+h-1;y1++) {
      *(uint8_t *)(_BACK_BUFFER_BASE + y1*64 + (x >> 1)) = 0x07;
      *(uint8_t *)(_BACK_BUFFER_BASE + y1*64 + ((x + w - 1) >> 1)) = 0x70;
  }
}

static void fill_rect(int x, int y, int w, int h)
{
    for (int x1=x;x1<x+w;x1+=2) {
        for (int y1=y;y1<y+h;y1++) {
            *(uint8_t *)(_BACK_BUFFER_BASE + y1*64 + (x1 >> 1)) = 0x77;
        }
    }
}

static void report_error(void)
{
  if (errno == ENOENT)
    _fatal_error(FILENAME ": file not found");
  else if (errno == EIO)
    _fatal_error("IO error");
  else
    _fatal_error("unknown error");
}

int main(void)
{
    char *load_ptr = (char *)LOAD_ADDRESS;
    _set_postcode(10);
    _clear_screen(_DARK_BLUE);
    draw_logo(LOGO_LEFT, LOGO_TOP);
    draw_rect(PROGRESS_BAR_LEFT,
              PROGRESS_BAR_TOP,
              PROGRESS_BAR_WIDTH,
              PROGRESS_BAR_HEIGHT);
    _flip();
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        report_error();
        return 1;
    }
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1) {
        report_error();
        return 1;
    }
    lseek(fd, 0, SEEK_SET);
    size_t total_read = 0;
    int last_progress = 0;
    for (;;) {
        ssize_t bytes_read = read(fd, load_ptr, BLOCK_SIZE);
        if (bytes_read == -1) {
            report_error();
            return 1;
        }
        if (bytes_read == 0)
            break;
        load_ptr += bytes_read;
        total_read += bytes_read;
        int progress = (total_read * (PROGRESS_BAR_WIDTH - 4)) / size;
        if (progress > last_progress) {
            _copy_front_to_back();
            fill_rect(PROGRESS_BAR_LEFT+2+last_progress,
                      PROGRESS_BAR_TOP + 2,
                      progress - last_progress,
                      PROGRESS_BAR_HEIGHT - 4);
            _flip();
            last_progress = progress;
        }
    }
    if (*(uint32_t *)LOAD_ADDRESS == 0) {
        _fatal_error("invalid file contents");
        return 1;
    }
    _clear_screen(_DARK_BLUE);
    _flip();
    __asm__("jmp (%0)"
            :
            : "a" (LOAD_ADDRESS));
    return 1;
}
