#! /bin/sh
set -x

{
cat <<-EOF
(define append (lambda (x y)
	(cond
		((null? x) y)
		(#t (cons (car x) (append (cdr x) y))))))
(define foo (list 1 2 3 4 5))
(append (append foo (list 6)) (list 7))
(HALT)
EOF
} >| test/test1/temp

./bin/slow_lisp --file test/test1/temp --console test/results/test1-result || exit 1
exit 0
