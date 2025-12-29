CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Isrc
LDFLAGS = -lreadline

TARGET = shell

# Source files
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/shell.cpp \
          $(SRCDIR)/parser.cpp \
          $(SRCDIR)/executor.cpp \
          $(SRCDIR)/job_control.cpp \
          $(SRCDIR)/builtins.cpp \
          $(SRCDIR)/completion.cpp \
          $(SRCDIR)/heredoc.cpp \
          $(SRCDIR)/utils.cpp

# Object files
OBJDIR = build
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(TARGET) $(OBJDIR)

run: $(TARGET)
	./$(TARGET)
