CXX := g++
XFLAGS := -Wall
LIBRARIES := -lcurl
INCLUDES := -I/usr/include/ -I$(PWD)/nlohmann_json/single_include/
CXXFLAGS := $(XFLAGS) $(LIBRARIES) $(INCLUDES)

OUTDIR := out
SOURCES := $(shell find . -maxdepth 1 -name '*.cpp')
OBJS := $(SOURCES:%.cpp=$(OUTDIR)/%.o)
TARGET := $(OUTDIR)/peterbot

.PHONY: all clean run

$(OUTDIR)/%.o: %.cpp
	@mkdir -p $(OUTDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

all: $(TARGET)
run: all
	set -a && source ./config && $(TARGET)
clean:
	rm -rf $(OUTDIR)