const std = @import("std");
const nc = @cImport({
    @cInclude("curses.h");
});

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
        try split_text.append(@ptrCast(text_item));
    }
    _ = split_text.pop();

    _ = nc.initscr();
    for (split_text.items) |text_item| {
        _ = nc.printw(text_item);
    }
    _ = nc.refresh();
    _ = nc.getch();
    _ = nc.endwin();
}
