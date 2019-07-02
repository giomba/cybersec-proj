.POSIX:
CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++11 -g -O0
LDFLAGS=-lpthread -lcrypto

# To debug add
# 	DBGFLAGS=-DdebugLevel=x
# where x is
# 	FATAL	0
# 	ERROR	1
# 	WARNING	2
# 	INFO	3
# 	DEBUG	4

# Header files
CXX_S_HDR=$(wildcard src/server/*.h)
CXX_C_HDR=$(wildcard src/client/*.h)
CXX_A_HDR=$(wildcard src/common/*.h)

# Common sources and object files
CXX_A_SRC=$(wildcard src/common/*.cpp)
CXX_A_OBJ=$(patsubst src/common/%.cpp, obj/c_a_%.o, $(CXX_A_SRC))

# Server sources and object files
CXX_S_SRC=$(wildcard src/server/*.cpp)
CXX_S_OBJ=$(patsubst src/server/%.cpp, obj/c_s_%.o, $(CXX_S_SRC))

# Client sources and object files
CXX_C_SRC=$(wildcard src/client/*.cpp)
CXX_C_OBJ=$(patsubst src/client/%.cpp, obj/c_c_%.o, $(CXX_C_SRC))

.PHONY: all environment clean

all: environment bin/server bin/client

environment:
	mkdir -p bin obj

obj/c_a_%.o: src/common/%.cpp $(CXX_A_HDR)
	$(CXX) -c $(CXXFLAGS) $(DBGFLAGS) -o $@ $<

obj/c_s_%.o: src/server/%.cpp $(CXX_S_HDR) $(CXX_A_HDR)
	$(CXX) -c $(CXXFLAGS) $(DBGFLAGS) -o $@ $<

obj/c_c_%.o: src/client/%.cpp $(CXX_C_HDR) $(CXX_A_HDR)
	$(CXX) -c $(CXXFLAGS) $(DBGFLAGS) -o $@ $<

bin/server: $(CXX_A_OBJ) $(CXX_S_OBJ)
	$(CXX) $(CXXFLAGS) $(DBG_FLAGS) -o bin/server obj/c_s_*.o obj/c_a_*.o $(LDFLAGS)

bin/client: $(CXX_A_OBJ) $(CXX_C_OBJ)
	$(CXX) $(CXXFLAGS) $(DBG_FLAGS) -o bin/client obj/c_c_*.o obj/c_a_*.o $(LDFLAGS)

clean:
	rm -rf bin obj
