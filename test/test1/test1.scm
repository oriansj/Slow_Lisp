(define append (lambda (x y)
	(cond
		((null? x) y)
		(#t (cons (car x) (append (cdr x) y))))))
(define foo (list 1 2 3 4 5))
(append (append foo (list 6)) (list 7))
(HALT)
