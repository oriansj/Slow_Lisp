# Prevent rebuilding
VPATH = bin:test:test/results

all: slow_lisp

slow_lisp-debug: lisp.h lisp.c lisp_cell.c lisp_eval.c lisp_print.c lisp_read.c | bin
	gcc -ggdb lisp.h lisp.c lisp_cell.c lisp_eval.c lisp_print.c lisp_read.c -o bin/slow_lisp-debug

slow_lisp: lisp.h lisp.c lisp_cell.c lisp_eval.c lisp_print.c lisp_read.c | bin
	gcc -O2 lisp.h lisp.c lisp_cell.c lisp_eval.c lisp_print.c lisp_read.c -o bin/slow_lisp

coverage-test: lisp.h lisp.c lisp_cell.c lisp_eval.c lisp_print.c lisp_read.c
	gcc -fprofile-arcs -ftest-coverage lisp.h lisp.c lisp_cell.c lisp_eval.c lisp_print.c lisp_read.c -o bin/coverage-test

# Clean up after ourselves
.PHONY: clean
clean:
	rm -rf bin/ test/results/

# Directories
bin:
	mkdir -p bin

results:
	mkdir -p test/results

Coverage-cleanup:
	rm -f *.gc{da,no,ov}
