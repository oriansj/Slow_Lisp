;; Copyright (C) 2017 Jeremiah Orians
;; This file is part of stage0.
;;
;; stage0 is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; stage0 is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with stage0.  If not, see <http://www.gnu.org/licenses/>.

; Functions not yet determined to be universally essential or part of the standard

; pair-up lists
(define pair-up
	(lambda (vars vals)
		(cond
			((null? vars) (if (null? vals) nil (error "pair-up TMA")))
			((null? vals) (error "pair-up TFA"))
			(#t (cons (cons (car vars) (car vals)) (pair-up (cdr vars) (cdr vals)))))))

; extend an environmental list
(define env-bind
	(lambda (vars vals env)
		(cons
			(pair-up
				(if (list? vars) vars (cons vars nil))
				(if (list? vals) vals (cons vals nil)))
			env)))

; Lookup a symbol's value in an environment list
(define env-lookup
	(lambda (sym env)
		(if (null? env)
			(error "lookup UBV")
			((lambda (vcell)
				(if (null? vcell)
					(env-lookup sym (cdr env))
					(cdr vcell)))
			 (assoc sym (car env))))))
