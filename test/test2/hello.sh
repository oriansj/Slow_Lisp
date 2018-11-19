#! /bin/sh
set -x

{
cat <<-EOF
(atom? 1)
(atom? (cons 1 2))
(define x 1)
(atom? x)
(define x (cons 3 4))
(atom? x)
(atom? atom?)
(atom? "hello world")
(atom? 'a')
(HALT)
EOF
} >| test/test2/temp

./bin/slow_lisp --file test/test2/temp --console test/results/test2-result || exit 1
exit 0
