CXX := g++
XFLAGS := -Wall -std=c++17
LIBRARIES := -lcurl
INCLUDES := -I/usr/include/ -I$(PWD)/nlohmann_json/single_include/
CXXFLAGS := $(XFLAGS) $(LIBRARIES) $(INCLUDES)

OUTDIR := out
SOURCES := $(shell find . -maxdepth 1 -name '*.cpp')
OBJS := $(SOURCES:%.cpp=$(OUTDIR)/%.o)
DEPS := $(wildcard $(OBJS:%=%.d))
TARGET := $(OUTDIR)/peterbot

.PHONY: all clean run

$(OUTDIR)/%.o: %.cpp
	@mkdir -p $(OUTDIR)
	$(CXX) $(CXXFLAGS) -MD -MP -MF "$@.d" -c $< -o $@

include $(DEPS)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

all: $(TARGET)
run: all
	set -a && source ./config && $(TARGET)
clean:
	rm -rf $(OUTDIR)