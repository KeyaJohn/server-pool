DIR=.
SRC=$(wildcard $(DIR)/*.cpp)
OBJ=$(patsubst %.cpp,%.o,$(SRC))
CFLAG= -g 
LFLAG= -pthread  -std=c++11
BIN=./server
CC=g++

all:$(BIN)
$(DIR)/%.o:$(DIR)/%.cpp
	$(CC) -c $< -o $@ $(CFLAG)
$(BIN):$(OBJ)
	$(CC) -o $@ $^ $(LFLAG)
.PHONY:clean rebuild
clean:
	rm -rf $(BIN) $(OBJ)
rebuild: clean all
