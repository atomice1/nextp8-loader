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
#include "timestamp_macros.h"
#include "version_macros.h"

#define HW_API_VERSION 0
#define API_VERSION    0
#define MAJOR_VERSION  0
#define MINOR_VERSION  1
#define PATCH_VERSION  0

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

static const uint32_t loader_version = _MAKE_VERSION(API_VERSION, MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);
static const uint32_t loader_timestamp = _TIMESTAMP;

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

static uintptr_t memtop = 0;

static void detect_memtop(void)
{
    for (unsigned i=1;i<8;++i) {
        for (unsigned j=1;j<=i;++j) {
            uint16_t *p = ((uint16_t *) ((uintptr_t)j << 20)) - 1;
            uint16_t pat = 0x55555555 ^ j;
            *p = 0;
            *p = pat;
        }
        for (unsigned j=1;j<=i;++j) {
            uint16_t *p = ((uint16_t *) ((uintptr_t)j << 20)) - 1;
            uint16_t pat = 0x55555555 ^ j;
            if (*p != pat)
                return;
            *p = 0;
        }
        memtop = (uintptr_t) i << 20;
    }
}

int main(void)
{
    _set_postcode(10);
    uint32_t hw_timestamp = *(uint32_t *)_BUILD_TIMESTAMP_HI;
    uint32_t hw_version = *(uint32_t *)_HW_VERSION_HI;
    if (_EXTRACT_API(hw_version) != HW_API_VERSION)
        _fatal_error("Incompatible hardware version");
    _set_postcode(11);
    detect_memtop();
    _set_postcode(12);
    _clear_screen(_DARK_BLUE);
    draw_logo(LOGO_LEFT, LOGO_TOP);
    draw_rect(PROGRESS_BAR_LEFT,
              PROGRESS_BAR_TOP,
              PROGRESS_BAR_WIDTH,
              PROGRESS_BAR_HEIGHT);
    char ram_string[10];
    char hw_version_string[50];
    char bsp_version_string[50];
    char loader_version_string[50];
    unsigned ram_mb = memtop >> 20;
    strcpy(ram_string, "RAM 0 MB");
    ram_string[4] = ram_mb + '0';
    _format_version(hw_version_string, sizeof(hw_version_string), "HW", hw_version, hw_timestamp);
    _format_version(bsp_version_string, sizeof(bsp_version_string), "BSP", _bsp_version, _bsp_timestamp);
    _format_version(loader_version_string, sizeof(loader_version_string), "Loader", loader_version, loader_timestamp);
    _display_string(0, _SCREEN_HEIGHT - _FONT_LINE_HEIGHT * 4, ram_string);
    _display_string(0, _SCREEN_HEIGHT - _FONT_LINE_HEIGHT * 3, hw_version_string);
    _display_string(0, _SCREEN_HEIGHT - _FONT_LINE_HEIGHT * 2, bsp_version_string);
    _display_string(0, _SCREEN_HEIGHT - _FONT_LINE_HEIGHT * 1, loader_version_string);
    _set_postcode(13);
    _flip();
    _set_postcode(14);

    /* Copy the config data to the new location before we write over it. */
    char *new_stack = (char *) memtop - 512;
    struct _config_data *new_config_data = (struct _config_data *) new_stack;
    memcpy((char *) new_config_data, (char *) _CONFIG_BASE_ROM, 256);

    _set_postcode(15);
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        report_error();
        return 1;
    }
    _set_postcode(16);
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1) {
        report_error();
        return 1;
    }
    lseek(fd, 0, SEEK_SET);
    char *load_ptr = (char *)LOAD_ADDRESS;
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
    _set_postcode(17);
    close(fd);

    if (*(uint32_t *)LOAD_ADDRESS == 0) {
        _fatal_error("invalid file contents");
        return 1;
    }

    _clear_screen(_DARK_BLUE);
    _set_postcode(18);
    _flip();
    _set_postcode(19);

    struct _loader_data *loader_data = (struct _loader_data *) (new_stack + 256);
    loader_data->magic = LOADER_MAGIC;
    loader_data->loader_version = loader_version;
    loader_data->loader_timestamp = loader_timestamp;
    loader_data->entry_point = LOAD_ADDRESS;

    _set_postcode(20);
    __asm__("move.l %0,%%sp\n\t"
            "jmp (%1)"
            :
            : "r" (new_stack), "a" (LOAD_ADDRESS));
    return 1;
}