CXX      = g++
CXXFLAGS = -std=c++14 -O2 -Wall
TARGET   = BankSystem.exe
SRCS     = main.cpp ui.cpp fileio.cpp employee.cpp customer.cpp bankcard.cpp \
           transaction.cpp bankqueue.cpp branchgraph.cpp smartmgr.cpp
OBJS     = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	del /Q *.o $(TARGET) 2>nul || true

run: $(TARGET)
	$(TARGET)

.PHONY: all clean run
