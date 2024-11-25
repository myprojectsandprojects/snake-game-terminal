const std = @import("std");

pub fn main() !void {
    // std.debug.print("Hello Zig!\n", .{});
    //
    // const output = "\x1b[31mHello again!!!\x1b[0m\n";
    // const r = std.os.linux.write(1, output, output.len);
    // std.debug.print("r = {}\n", .{r});
    //
    // const output2 = "\x1b[s\x1b[1;10HHello one last time!\x1b[u";
    // _ = std.os.linux.write(1, output2, output2.len);

    const TIOCGWINSZ = 21523;
    var windowSize: std.os.linux.winsize = undefined;
    _ = std.os.linux.ioctl(1, TIOCGWINSZ, @intFromPtr(&windowSize));
    std.debug.print("rows: {}, columns: {}\n", .{ windowSize.ws_row, windowSize.ws_col });

    var t: std.os.linux.termios = undefined;
    var backup: std.os.linux.termios = undefined;

    const r = std.os.linux.tcgetattr(std.os.linux.STDIN_FILENO, &t); //£ return type?
    if (r != 0) {
        std.debug.print("error\n", .{});
        return;
    }

    backup = t;

    // turn off the display of characters as the user is typing
    t.lflag.ICANON = false;
    t.lflag.ECHO = false;
    _ = std.os.linux.tcsetattr(std.os.linux.STDIN_FILENO, std.os.linux.TCSA.FLUSH, &t);

    defer {
        _ = std.os.linux.tcsetattr(std.os.linux.STDIN_FILENO, std.os.linux.TCSA.FLUSH, &backup);
        std.debug.print("defer!\n", .{});
    }

    // snake:
    var snakeX: i32 = 3;
    var snakeY: i32 = 3;

    var dx: i32 = 1;
    var dy: i32 = 0;

    // food
    const foodX: i32 = 10;
    const foodY: i32 = 10;

    while (true) {
        var pollData = std.os.linux.pollfd{ .fd = std.os.linux.STDIN_FILENO, .events = std.os.linux.POLL.IN, .revents = 0 };
        const pollResult = std.os.linux.poll(@ptrCast(&pollData), 1, 1);
        std.debug.print("poll result: {}, revents: {}\n", .{ pollResult, pollData.revents });

        if (pollData.revents == std.os.linux.POLL.IN) {
            var input: [16]u8 = undefined;
            const bytesRead = std.os.linux.read(std.os.linux.STDIN_FILENO, &input, input.len);
            // std.debug.print("input: {s}\n", .{input[0..bytesRead]});

            if (input[bytesRead - 1] == 'w') {
                dx = 0;
                dy = -1;
            } else if (input[bytesRead - 1] == 's') {
                dx = 0;
                dy = 1;
            } else if (input[bytesRead - 1] == 'a') {
                dx = -1;
                dy = 0;
            } else if (input[bytesRead - 1] == 'd') {
                dx = 1;
                dy = 0;
            }
        }

        snakeX += dx;
        snakeY += dy;

        // const t1 = std.time.microTimestamp();

        for (1..windowSize.ws_row + 1) |row| {
            for (1..windowSize.ws_col + 1) |column| {
                const a = std.heap.page_allocator;
                const escapeSequence = try std.fmt.allocPrint(a, "\x1b[{};{}H", .{ row, column });
                _ = std.os.linux.write(1, escapeSequence.ptr, escapeSequence.len);
                a.free(escapeSequence);

                if (row == snakeY and column == snakeX) {
                    _ = std.os.linux.write(1, "x", 1);
                } else if (row == foodY and column == foodX) {
                    _ = std.os.linux.write(1, "o", 1);
                } else {
                    _ = std.os.linux.write(1, " ", 1);
                }
            }
        }

        if (snakeX == foodX and snakeY == foodY) {
            break;
        }

        // const t2 = std.time.microTimestamp();
        // const d = t2 - t1;
        // const ms = @as(f64, @floatFromInt(d)) / 1000;
        // std.debug.print("{d} ms\n", .{ms});

        // const sleepDuration = std.os.linux.timespec{ .tv_sec = 1, .tv_nsec = 0 };
        const sleepDuration = std.os.linux.timespec{ .tv_sec = 0, .tv_nsec = 100 * 1000 * 1000 };
        _ = std.os.linux.nanosleep(&sleepDuration, null);
    }
}
