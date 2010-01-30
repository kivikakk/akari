TEST_SRCS := $(wildcard *_test.cpp)
TESTS := $(patsubst %_test.cpp,%_test.o,$(TEST_SRCS))

all: $(TESTS)
	@echo "Tests pass"

%_test.o: %_test.cpp %_test.expected
	@g++ -I../inc $< -o $@
	@(./$@ | diff - $(patsubst %.cpp,%.expected,$<)) || (r=$$?; rm $@; echo $< failed; exit $$r)
	@rm $@
