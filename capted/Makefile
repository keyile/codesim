print-%: ; @echo $*=$($*)

RM       = rm -r
CXX      = clang++
CXXFLAGS = -g -Wall -std=c++11 -stdlib=libc++ -MMD -I./lib -I.
LDFLAGS  = -stdlib=libc++
HEADERS  = $(shell find lib -name "*.h")

SRC_DIR = .
BIN_DIR = bin
SRCS = $(shell find $(SRC_DIR) -type f -name *.cpp)
OBJS = $(subst $(SRC_DIR)/,$(BIN_DIR)/, $(subst .cpp,.o,$(SRCS)))
DEPS = $(subst .o,.d,$(OBJS))
EXEC = $(BIN_DIR)/test_runner

# Uncomment to use int64 instead of int32
# CXXFLAGS += -DCAPTED_LARGE_TREES

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

-include $(DEPS)

run: all
	./bin/test_runner

clean:
	$(RM) $(BIN_DIR)
