
# files

EXE = scan

OBJS = bb_base.o bb_comp.o bb_index.o bit.o board.o book.o dxp.o eval.o \
       fen.o game.o hash.o libmy.o list.o main.o move.o move_gen.o pos.o \
       score.o search.o socket.o sort.o thread.o trans.o tuple.o util.o var.o

# rules

all: $(EXE) .depend

clean:
	$(RM) *.o .depend

# general

CXX      = g++
# CXXFLAGS = 
LDFLAGS  = -L.
LIBS     = -lm -lpthread

# C++

CXXFLAGS += -std=c++11 -fno-rtti

# optimisation

CXXFLAGS += -O2
CXXFLAGS += -mpopcnt

CXXFLAGS += -flto
LDFLAGS  += -flto

# branch optimisation

# CXXFLAGS += -fprofile-arcs
# CXXFLAGS += -fbranch-probabilities

# debug

# CXXFLAGS += -DDEBUG
# CXXFLAGS += -g
# LDFLAGS  += -g

# profile

# CXXFLAGS += -pg
# LDFLAGS  += -pg

# dependencies

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.s: %.cpp
	$(CXX) -S $(CXXFLAGS) $<

.depend:
	$(CXX) -MM $(OBJS:.o=.cpp) > $@

include .depend

