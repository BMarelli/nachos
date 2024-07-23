#include "lib.c"
#include "syscall.h"

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
}
