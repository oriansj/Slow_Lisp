#! /bin/sh
set -x

{
cat <<-EOF
(cons 1 2 3)
(cons (cons 4 5) (cons 6 7))
(cons (cons (cons (cons (cons (cons 1 2) 3) 4) 5) 6) 7)
(cons 1 (cons 2 (cons 3 (cons 4 (cons 5 (cons 6 7))))))
(HALT)
EOF
} >| test/test3/temp

./bin/slow_lisp --file test/test3/temp --console test/results/test3-result || exit 1
exit 0
