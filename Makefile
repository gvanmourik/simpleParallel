NAME = simpleParallel

CC := g++

INCLUDES :=
CXXFLAGS := 
CPPFLAGS :=
CFLAGS :=
LDFLAGS := -Wall
LIBS :=

TARGET = bin/$(NAME)
SOURCE = src/$(NAME).cpp
OBJECT = $(SOURCE:.cpp=.o)

all : $(TARGET)

%.o : %.cpp
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<

$(TARGET) : $(OBJECT)
	$(CC) -o $@ $+ $(LDFLAGS) $(LIBS)

clean : $(OBJECT)
	rm -f *.o $(OJBECT) $(TARGET)
	@echo clean complete...
