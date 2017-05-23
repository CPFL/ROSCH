COMPILER  = g++
CFLAGS    = -g -std=c++11 -MMD -MP -Wall -Wextra -Winit-self -Wno-missing-field-initializers -DSCHED_ANALYZER
ifeq "$(shell getconf LONG_BIT)" "64"
  LDFLAGS = -lyaml-cpp `pkg-config --cflags opencv` `pkg-config --libs opencv` -L/usr/local/cuda-7.5/lib64
else
  LDFLAGS =
endif
LIBS      =
INCLUDE   = -I./include
TARGET    = ./bin/$(shell basename `readlink -f .`)
SRCDIR    = ./src
ifeq "$(strip $(SRCDIR))" ""
  SRCDIR  = .
endif
SOURCES   = $(wildcard $(SRCDIR)/*.cpp)
OBJDIR    = ./obj
BINDIR    = ./bin
ifeq "$(strip $(OBJDIR))" ""
  OBJDIR  = .
endif
OBJECTS   = $(addprefix $(OBJDIR)/, $(notdir $(SOURCES:.cpp=.o)))
DEPENDS   = $(OBJECTS:.o=.d)

$(TARGET): $(OBJECTS) $(LIBS)
	$(COMPILER) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	-mkdir -p $(OBJDIR)
	-mkdir -p $(BINDIR)
	$(COMPILER) $(CFLAGS) $(INCLUDE) -o $@ -c $<

all: clean $(TARGET)

run:
	$(TARGET)

clean:
	-rm -f $(OBJECTS) $(DEPENDS) $(TARGET)

-include $(DEPENDS)
