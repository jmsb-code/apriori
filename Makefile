# THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING A TUTOR,
# CODE WRITTEN BY OTHER STUDENTS, OR CODE DERIVED FROM AN AI TOOL- James Broderick

TARGET=apriori
CXX=g++

CXXFLAGS=

.PHONY: clean

#source files
SRCS = apriori.cpp

#generate compile object for each source file
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

#link all compiled object files into executable (build)
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)