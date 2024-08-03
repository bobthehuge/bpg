CC = gcc
CDEVFLAGS = -std=c99 -g -Wall -Wextra
CRELFLAGS = -std=c99 -Ofast -DNDEBUG
# CBENCHFLAGS = -std=c99 -Ofast -DNDEBUG -g -pg
# CVISFLAGS = -std=c99 -g -Wall -Wextra -lraylib
LDFLAGS = 
LDLIBS = -lm -lraylib -lGL -lc -lrt

SRC = *.c
OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)
DOUT_FILES = *.dout
OUT_FILES = *.out
LOG_FILES = *.log
BENCH_FILES = *.gcda

all: rel
rel: 
	$(CC) -o main $(SRC) $(CRELFLAGS) $(LDLIBS)
dev:
	-$(CC) -o main $(SRC) $(CDEVFLAGS) $(LDLIBS) 2>&1 | cat >clang.log && cat clang.log
gdb: dev
	gdb ./main

run:
	./main

-include ${DEP}

.PHONY: clean test run plot __plot bench ref dev gdb

clean:
	$(RM) $(OBJ)
	$(RM) $(DEP)
	$(RM) $(LOG_FILES)
	$(RM) ./main

# END
