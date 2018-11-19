#! /bin/sh
set -x

{
cat <<-EOF
(quote (Hello, Mescc!))
(HALT)
EOF
} >| test/test0/temp

./bin/slow_lisp --file test/test0/temp --console test/results/test0-result || exit 1
exit 0
