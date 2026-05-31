CXX = clang++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra -fPIC -march=native -flto

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	SHARED_LIB_FLAGS = -dynamiclib
	LIB_EXT = dylib
else
	SHARED_LIB_FLAGS = -shared
	LIB_EXT = so
endif

TARGET = libmatchingengine.$(LIB_EXT)
SRCS = MatchingEngine.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(SHARED_LIB_FLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
