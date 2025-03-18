CXX = g++
CXXFLAGS = -Wall -Wextra

ifneq ($(ENABLE_SANITIZER),)
CXXFLAGS += -fsanitize=address -fsanitize=leak -fsanitize=undefined
else
ifneq ($(ENABLE_THREAD_SANITIZER),)
CXXFLAGS += -fsanitize=thread -fsanitize=undefined
endif
endif
ifneq ($(DEBUG),)
CXXFLAGS += -g3
endif

all: test

test: main.cpp fastbuffer.hpp
	$(CXX) $(CXXFLAGS) main.cpp -o $@

clean:
	rm -f test

.PHONY: clean
