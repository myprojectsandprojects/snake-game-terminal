// gcc -Wall -Wextra -std=c99 test.c -o test -D _DEFAULT_SOURCE

#include <stdio.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <poll.h>


void clear_buffer(unsigned char *buffer, struct winsize sz, char character) {
//void clear_buffer(unsigned char *buffer, struct winsize sz) {
    for(int r = 0; r < sz.ws_row; ++r) {
        for(int c = 0; c < sz.ws_col; ++c) {
            //buffer[r][c] = '/';
            *(buffer + r * sz.ws_col + c) = character;
        }
    }
}

int main()
{
//    printf("%d", TIOCGWINSZ);
//    return 0;

    struct winsize sz;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &sz);
    printf("rows = %d, columns = %d\n", sz.ws_row, sz.ws_col);

//    int width = sz.ws_col - 1;
//    int height = sz.ws_row - 1;

    struct termios  termios_new;
    struct termios  termios_backup;

    tcgetattr(STDIN_FILENO, &termios_backup);
    termios_new = termios_backup;

    termios_new.c_lflag &= ~(ICANON);
    termios_new.c_lflag &= ~(ECHO);
    termios_new.c_cc[VMIN] = 1;
    termios_new.c_cc[VTIME] = 0;

    /*
    **  Set the change
    */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_new);

    unsigned char buffer[sz.ws_row][sz.ws_col];

    clear_buffer((unsigned char *)buffer, sz, ' ');

    int playing_field_x = 3;
    int playing_field_y = 3;
    int playing_field_width = 30;
    int playing_field_height = 10;

    int snake_x = 1;
    int snake_y = 1;
    int snake_width = 16;
    int snake_height = 2;

    int snake_velocity_x = 1;
    int snake_velocity_y = 0;

    char input[1024];

    while(1) {
        // read() blocks, so poll()?
        struct pollfd poll_data;
        poll_data.fd = STDIN_FILENO;
        poll_data.events = POLLIN;
        poll_data.revents = 0; // out

        int poll_result = poll(&poll_data, 1, 1);
        //printf("%d, %d\n", poll_result, poll_data.revents);

        if(poll_result > 0) {
            // check for user input
            int bytes_read = read(STDIN_FILENO, input, 1024);
            assert(bytes_read > 0);
            assert(bytes_read < 1024);
            input[bytes_read] = 0;
            //printf("bytes read: %s (%d bytes)\n", input, bytes_read);

            if(input[bytes_read - 1] == 'w') {
                snake_velocity_y = -1;
                snake_velocity_x = 0;
            } else if(input[bytes_read - 1] == 's') {
                snake_velocity_y = 1;
                snake_velocity_x = 0;
            } else if(input[bytes_read - 1] == 'a') {
                snake_velocity_y = 0;
                snake_velocity_x = -1;
            } else if(input[bytes_read - 1] == 'd') {
                snake_velocity_y = 0;
                snake_velocity_x = 1;
            }

//            for(int i = 0; input[i] != '\0'; ++i) {
//                if(input[i] == 'w') {
//                    velocity_y = -1;
//                    velocity_x = 0;
//                } else if(input[i] == 's') {
//                    velocity_y = 1;
//                    velocity_x = 0;
//                } else if(input[i] == 'a') {
//                    velocity_y = 0;
//                    velocity_x = -1;
//                } else if(input[i] == 'd') {
//                    velocity_y = 0;
//                    velocity_x = 1;
//                }
//            }
        } else if(poll_result < 0) {
            assert(0);
        }

        // update snake's position
        snake_x += snake_velocity_x;
        snake_y += snake_velocity_y;

        clear_buffer((unsigned char *)buffer, sz, ' ');

        // draw playing field
        {
            int x = playing_field_x;
            int y = playing_field_y - 1;
            while(x < playing_field_x + playing_field_width) {
                *((char *)buffer + y * sz.ws_col + x) = '-';

                x += 1;
            }
        }

        {
            int x = playing_field_x;
            int y = playing_field_y + playing_field_height;
            while(x < playing_field_x + playing_field_width) {
                *((char *)buffer + y * sz.ws_col + x) = '-';

                x += 1;
            }
        }

        {
            int x = playing_field_x - 1;
            int y = playing_field_y;
            while(y < playing_field_y + playing_field_height) {
                *((char *)buffer + y * sz.ws_col + x) = '|';

                y += 1;
            }
        }

        {
            int x = playing_field_x + playing_field_width;
            int y = playing_field_y;
            while(y < playing_field_y + playing_field_height) {
                *((char *)buffer + y * sz.ws_col + x) = '|';

                y += 1;
            }
        }

        *((char *)buffer + (playing_field_y - 1) * sz.ws_col + (playing_field_x - 1)) = '+';
        *((char *)buffer + (playing_field_y - 1) * sz.ws_col + (playing_field_x + playing_field_width)) = '+';
        *((char *)buffer + (playing_field_y + playing_field_height) * sz.ws_col + (playing_field_x + playing_field_width)) = '+';
        *((char *)buffer + (playing_field_y + playing_field_height) * sz.ws_col + (playing_field_x - 1)) = '+';

        // draw the snake into the buffer
        for(int y = snake_y; y < snake_y + snake_height; ++y) {
            for(int x = snake_x; x < snake_x + snake_width; ++x) {
                *((char *)buffer + y * sz.ws_col + x) = '/';
            }
        }

        // draw the buffer onto the window
        // draw into intermediary buffer first and then print the whole thing at once?
        for(int r = 0; r < sz.ws_row; ++r) {
            for(int c = 0; c < sz.ws_col; ++c) {
                fprintf(stderr, "\033[%d;%dH%c", r + 1, c + 1, buffer[r][c]);
                //printf("\033[%d;%dH%c", r + 1, c + 1, buffer[r][c]);
            }
        }

//        int millisecond = 1000;
//        usleep(100 * millisecond);
//        //sleep(1);

        struct timespec duration;
        duration.tv_sec = 0;
        duration.tv_nsec = 1000 * 1000; // 1 millisecond
        nanosleep(&duration, NULL);
    }

	return 0;
}
