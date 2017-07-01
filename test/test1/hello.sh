#! /bin/sh
set -x
out=$( bin/slow_lisp -f test/test1/test1.scm 2>&1)
[ "$out" = "#<PROC>
(1 2 3 4 5)
(1 2 3 4 5 6 7)" ] || exit 1
echo "$out" >| test/results/test1-result
exit 0
