CXX := g++
XFLAGS := -Wall
LIBRARIES := -lcurl
INCLUDES := -I/usr/include/
CXXFLAGS := $(XFLAGS) $(LIBRARIES) $(INCLUDES)

OUTDIR := out
SOURCES := $(shell find . -name '*.cpp')
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
	$(TARGET)
clean:
	rm -rf $(OUTDIR)