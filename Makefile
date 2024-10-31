CC := gcc

CFLAGS := -g -Wall -Werror -std=c99 -fPIC -D_DEFAULT_SOURCE

TESTS := test_bulk test_simple_malloc

all: libcsemalloc.so

libcsemalloc.so: src/mm.o src/bulk.o
	$(CC) -shared -fPIC -o $@ $^

test: $(TESTS) $(NEWTESTS)
	@echo
	@for test in $^; do                                   \
	    printf "Running %-30s: " "$$test";                \
	    (./$$test && echo "passed") || echo "failed";       \
	done
	@echo


# This pattern rule should be starting to look familiar.  It will use
# the definitions of CC and CFLAGS, above, to create an object file from
# a source C file.
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# This pattern will build any self-contained test file in tests/.  If
# your test file needs more support, you will need to write an explicit
# rule for it.
#
# To add a test, create a file called tests/testname.c that contains a
# main function and all of the relevant test code, then add the basename
# of the file (e.g., testname in this example) to TESTS, above.
%: tests/%.o src/mm.o src/bulk.o
	$(CC) -o $@ $^

clean:
	rm -f $(TESTS) libcsemalloc.so malloc.tar
	rm -f src/*.o tests/*.o *~ src/*~ tests/*~

# See previous assignments for a description of .PHONY
.PHONY: all clean submission test
