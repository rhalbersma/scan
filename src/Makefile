
# files

EXE = scan

OBJS = bb_base.o bb_comp.o bb_index.o bit.o book.o common.o dxp.o eval.o \
       fen.o game.o hash.o hub.o libmy.o list.o main.o move.o move_gen.o \
       pos.o score.o search.o socket.o sort.o thread.o tt.o util.o var.o

# rules

all: $(EXE)

clean:
	$(RM) $(OBJS) .depend # keep exe

# general

CXX      = g++
CXXFLAGS = -pthread
LDFLAGS  = -pthread
LIBS     = -lm

# warnings

CXXFLAGS += -ansi -pedantic -Wall -Wextra

# C++

CXXFLAGS += -std=c++11 -fno-rtti

# optimisation

CXXFLAGS += -O2 -mpopcnt
LDFLAGS  += -O2

CXXFLAGS += -flto
LDFLAGS  += -flto

# dependencies

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.depend:
	$(CXX) $(CXXFLAGS) -MM $(OBJS:.o=.cpp) > $@

include .depend

