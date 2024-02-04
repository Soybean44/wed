const std = @import("std");

fn nextLine(reader: anytype, buffer: []u8) !?[]const u8 {
    var line = (try reader.readUntilDelimiterOrEof(
        buffer,
        '\n',
    )) orelse return null;
    // trim annoying windows-only carriage return character
    if (@import("builtin").os.tag == .windows) {
        return std.mem.trimRight(u8, line, "\r");
    } else {
        return line;
    }
}

fn isNumeric(num: u8) bool {
    return (48 <= num and num <= 57);
}

pub fn main() !void {
    const argv = std.os.argv;
    const argc = argv.len;
    if (argc <= 1) {
        std.debug.print("text-editor <input file>\n", .{});
        return error.noInput;
    }

    const alloc = std.heap.page_allocator;

    var file = try std.fs.cwd().openFileZ(argv[1], .{ .mode = .read_write });
    try file.seekFromEnd(0);
    const size = try file.getPos();
    var text = try alloc.alloc(u8, @as(usize, size));
    defer alloc.free(text);
    try file.seekTo(0);
    _ = try file.read(text);
    file.close();

    var text_iterator = std.mem.split(u8, text, "\n");
    var split_text = std.ArrayList([]const u8).init(alloc);
    defer split_text.deinit();
    while (text_iterator.next()) |text_item| {
        try split_text.append(text_item);
    }
    _ = split_text.pop();

    const stdin = std.io.getStdIn();

    var lexerBuffer = std.ArrayList(u8).init(alloc);

    programLoop: while (true) {
        var lineBuffer: [100]u8 = undefined;

        const line = (try nextLine(stdin.reader(), &lineBuffer)).?;
        for (line) |input| {
            if (isNumeric(input)) {
                std.debug.print("{}\n", .{input});
                try lexerBuffer.append(input);
            }
            if (input == 'q') {
                break :programLoop;
            } else if (input == '%') {
                for (split_text.items) |text_item| {
                    std.debug.print("{s}\n", .{text_item});
                }
            } else if (input == 'n') {
                for (split_text.items, 0..) |text_item, index| {
                    std.debug.print("{}: {s}\n", .{ index, text_item });
                }
            } else if (input == 'p') {
                if (lexerBuffer.items.len >= 1) {
                    const index = try std.fmt.parseInt(usize, lexerBuffer.items, 10);
                    lexerBuffer.clearAndFree();
                    std.debug.print("{}: {s}\n", .{ index, split_text.items[index] });
                } else {
                    std.debug.print("error no pointer specified\n", .{});
                }
            }
        }
    }
}
