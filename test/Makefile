TEST_SRCS := $(wildcard *_test.cpp)
TESTS := $(patsubst %_test.cpp,%_test.o,$(TEST_SRCS))

all: clean $(TESTS)
	@echo "Tests pass"

clean:
	@-rm *.o

%_test.o: %_test.cpp %_test.expected
	@g++ -I../inc $< -o $@
	@# Do nothing in case of segfaultry - we want to pick that up early.
	@./$@ > /dev/null
	@./$@ | diff - $(patsubst %.cpp,%.expected,$<)
