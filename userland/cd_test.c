#include "lib.c"
#include "syscall.h"

#define HELLO_WORLD "Hello, world!\n"

int main() {
    int res;

    puts("$ mkdir dir1\n");
    Mkdir("dir1");

    puts("$ mkdir dir2\n");
    Mkdir("dir2");

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: [self] dir1 dir2\n");

    puts("$ cd dir1\n");
    Cd("dir1");

    puts("$ mkdir foo\n");
    Mkdir("foo");

    puts("$ touch bar\n");
    Create("bar");

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: foo bar\n");

    puts("$ cd\n");
    Cd(NULL);

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: [self] dir1 dir2\n");

    puts("$ mkdir dir3\n");
    Mkdir("dir3");

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: [self] dir1 dir2 dir3\n");

    puts("$ rmdir dir1\n");
    res = RemoveDir("dir1");
    puts("debug: expected output: -1, actual output: ");
    puti(res);
    puts("\n");

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: [self] dir1 dir2 dir3\n");

    puts("$ cd dir1\n");
    Cd("dir1");

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: foo bar\n");

    puts("$ rmdir foo\n");
    res = RemoveDir("foo");
    puts("debug: expected output: 0, actual output: ");
    puti(res);
    puts("\n");

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: bar\n");

    puts("$ cd\n");
    Cd(NULL);

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: [self] dir1 dir2 dir3\n");

    puts("$ rmdir dir1\n");
    res = RemoveDir("dir1");
    puts("debug: expected output: -1, actual output: ");
    puti(res);
    puts("\n");

    puts("$ cd dir1\n");
    Cd("dir1");

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: bar\n");

    puts("$ rm bar\n");
    Remove("bar");

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output:\n");

    puts("$ cd\n");
    Cd(NULL);

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: [self] dir1 dir2 dir3\n");

    puts("$ rmdir dir1\n");
    res = RemoveDir("dir1");
    puts("debug: expected output: 0, actual output: ");
    puti(res);
    puts("\n");

    puts("$ ls\n");
    Ls(NULL);
    puts("debug: expected output: [self] dir2 dir3\n");

    puts("$ touch dir2/foo\n");
    Create("dir2/foo");

    puts("$ ls dir2\n");
    Ls("dir2");  // FIXME: check all is good with Ls and unhappy paths
    puts("debug: expected output: foo\n");

    puts("$ mkdir dir2/test\n");
    Mkdir("dir2/test");

    puts("$ open dir2/foo\n");
    OpenFileId fid = Open("dir2/foo");

    puts("$ echo 'Hello, world!\\n' >> dir2/foo\n");
    res = Write(HELLO_WORLD, sizeof(HELLO_WORLD), fid);
    puts("debug: expected output: ");
    puti(sizeof(HELLO_WORLD));
    puts(", actual output: ");
    puti(res);
    puts("\n");

    puts("$ close dir2/foo\n");
    res = Close(fid);

    puts("$ rm dir2/foo\n");
    res = Remove("dir2/foo");
    puts("debug: expected output: 0, actual output: ");
    puti(res);
    puts("\n");

    puts("$ ls dir2\n");
    Ls("dir2");
    puts("debug: expected output: test\n");
}
