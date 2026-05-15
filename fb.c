/*
 * fb.c — write pixels directly to the Linux framebuffer.
 *
 * This program does the smallest possible thing: it fills the screen
 * with a color, one pixel at a time, with no graphics library at all.
 *
 * The whole program is about 40 lines of real code. Every line is
 * explained in the comments. If you understand this file, you understand
 * what the bottom of the Linux graphics stack actually does.
 *
 * Build:  gcc -o fb fb.c
 * Run:    sudo ./fb
 *
 * Run it from a TTY (Ctrl+Alt+F3), not inside a desktop session,
 * or you won't see the result.
 */

#include <stdio.h>      /* printf, perror */
#include <fcntl.h>      /* open */
#include <unistd.h>     /* close */
#include <sys/mman.h>   /* mmap, munmap */
#include <sys/ioctl.h>  /* ioctl */
#include <linux/fb.h>   /* fb_var_screeninfo, FBIOGET_VSCREENINFO */

int main(void) {
    /* Step 1: open the framebuffer device.
     *
     * /dev/fb0 is a "character device" the kernel exposes. To us it
     * looks like a file. To the hardware, it is a window into the
     * memory that the display will read from. */
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    /* Step 2: ask the kernel how big the screen is and how each pixel
     * is laid out in memory.
     *
     * ioctl() is the generic "ask the driver a question" system call.
     * FBIOGET_VSCREENINFO fills in a struct with width, height, and
     * bits-per-pixel. We need these to know where each pixel lives. */
    struct fb_var_screeninfo vi;
    ioctl(fd, FBIOGET_VSCREENINFO, &vi);

    int width  = vi.xres;
    int height = vi.yres;
    int bpp    = vi.bits_per_pixel / 8;   /* bytes per pixel, usually 4 */
    long size  = (long)width * height * bpp;

    printf("screen: %d x %d, %d bytes per pixel, %ld bytes total\n",
           width, height, bpp, size);

    /* Step 3: map the framebuffer into our process's memory.
     *
     * After this call, the pointer `fb` IS the screen. Writing a byte
     * to fb[0] writes to the top-left pixel. There is no API, no
     * function call — the memory we're writing to is the same memory
     * the display hardware reads from sixty times a second. */
    unsigned char *fb = mmap(NULL, size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);

    /* Step 4: paint every pixel.
     *
     * We walk through the screen row by row, pixel by pixel, and
     * write four bytes per pixel: blue, green, red, and an unused
     * byte. (Most modern Linux framebuffers store pixels in B-G-R-X
     * order, which is just a hardware convention.)
     *
     * The color we write here is teal: R=29, G=158, B=117. */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            long offset = (y * width + x) * bpp;
            fb[offset + 0] = 117;   /* blue  */
            fb[offset + 1] = 158;   /* green */
            fb[offset + 2] = 29;    /* red   */
            fb[offset + 3] = 0;     /* unused */
        }
    }

    /* Step 5: clean up.
     *
     * Unmap the memory and close the file. The pixels we wrote stay
     * on the screen — the display hardware keeps scanning them out
     * until something else overwrites the framebuffer. */
    munmap(fb, size);
    close(fd);
    return 0;
}
