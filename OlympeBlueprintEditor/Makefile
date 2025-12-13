# Olympe Blueprint Editor - Makefile
# Simple build system for Linux/Unix environments

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I../Source -I../Source/third_party -Iinclude
LDFLAGS = 

TARGET = OlympeBlueprintEditor
SRCDIR = src
INCDIR = include
BUILDDIR = build

SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/BlueprintEditor.cpp \
          $(SRCDIR)/EntityBlueprint.cpp

OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)
	@echo "Cleaned build artifacts"

run: $(TARGET)
	./$(TARGET)

# Build test program
test: $(BUILDDIR)/blueprint_test
	./$(BUILDDIR)/blueprint_test

$(BUILDDIR)/blueprint_test: $(SRCDIR)/blueprint_test.cpp $(SRCDIR)/EntityBlueprint.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $^ -o $@
