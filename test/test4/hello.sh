#! /bin/sh
set -x

{
cat <<-EOF
(car (cons 'a' 'b'))
(cdr (cons 'a' 'b'))

(car (car (cons (cons 1 2) (cons 3 4))))
(car (cdr (cons (cons 1 2) (cons 3 4))))
(cdr (car (cons (cons 1 2) (cons 3 4))))
(cdr (cdr (cons (cons 1 2) (cons 3 4))))

(car (car (car (cons (cons (cons 1 2) (cons 3 4)) (cons (cons 5 6) (cons 7 8))))))
(car (car (cdr (cons (cons (cons 1 2) (cons 3 4)) (cons (cons 5 6) (cons 7 8))))))
(car (cdr (car (cons (cons (cons 1 2) (cons 3 4)) (cons (cons 5 6) (cons 7 8))))))
(car (cdr (cdr (cons (cons (cons 1 2) (cons 3 4)) (cons (cons 5 6) (cons 7 8))))))
(cdr (car (car (cons (cons (cons 1 2) (cons 3 4)) (cons (cons 5 6) (cons 7 8))))))
(cdr (car (cdr (cons (cons (cons 1 2) (cons 3 4)) (cons (cons 5 6) (cons 7 8))))))
(cdr (cdr (car (cons (cons (cons 1 2) (cons 3 4)) (cons (cons 5 6) (cons 7 8))))))
(cdr (cdr (cdr (cons (cons (cons 1 2) (cons 3 4)) (cons (cons 5 6) (cons 7 8))))))

(HALT)
EOF
} >| test/test4/temp

./bin/slow_lisp --file test/test4/temp --console test/results/test4-result || exit 1
exit 0
