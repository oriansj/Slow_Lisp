# Prevent rebuilding
VPATH = bin:test:test/results

all: slow_lisp

slow_lisp-M2: lisp.h lisp.c lisp_cell.c lisp_eval.c lisp_print.c lisp_read.c | bin
	./kaem.run

slow_lisp-debug: lisp.h lisp.c lisp_cell.c lisp_eval.c lisp_print.c lisp_read.c | bin
	gcc -ggdb \
	lisp.h \
	lisp.c \
	lisp_cell.c \
	lisp_eval.c \
	lisp_print.c \
	lisp_read.c \
	functions/numerate_number.c \
	functions/match.c \
	functions/file_print.c \
	-o bin/slow_lisp-debug

slow_lisp: lisp.h lisp.c lisp_cell.c lisp_eval.c lisp_print.c lisp_read.c | bin
	gcc \
	lisp.h \
	lisp.c \
	lisp_cell.c \
	lisp_eval.c \
	lisp_print.c \
	lisp_read.c \
	functions/numerate_number.c \
	functions/match.c \
	functions/file_print.c \
	-o bin/slow_lisp

coverage-test: lisp.h lisp.c lisp_cell.c lisp_eval.c lisp_print.c lisp_read.c
	gcc -fprofile-arcs \
	-ftest-coverage \
	lisp.h \
	lisp.c \
	lisp_cell.c \
	lisp_eval.c \
	lisp_print.c \
	lisp_read.c \
	functions/numerate_number.c \
	functions/match.c \
	functions/file_print.c \
	-o bin/coverage-test

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


# tests
test: \
	test0-result \
	test1-result \
	test2-result \
	test3-result \
	test4-result | results
	sha256sum -c test/test.answers

test0-result: results slow_lisp
	test/test0/hello.sh

test1-result: results slow_lisp
	test/test1/hello.sh

test2-result: results slow_lisp
	test/test2/hello.sh

test3-result: results slow_lisp
	test/test3/hello.sh

test4-result: results slow_lisp
	test/test4/hello.sh

# Generate test answers
.PHONY: Generate-test-answers
Generate-test-answers:
	sha256sum test/results/* >| test/test.answers
