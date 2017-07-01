#! /bin/sh
set -x
out=$( bin/slow_lisp -f test/test0/hello.scm 2>&1)
[ "$out" = "(Hello, Mescc!)" ] || exit 1
exit 0
