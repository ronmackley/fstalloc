CXX=g++  -DLINUX

CXXFLAGS=-g -pg -O0 #-DDEBUG
#CXXFLAGS= -O2
.SUFFIXES:
.SUFFIXES: .cpp .o .s .i

.cpp.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.cpp.s:
	$(CXX) -S $(CXXFLAGS) $(CPPFLAGS) -c $<

.cpp.ii:
	$(CXX) -E $(CXXFLAGS) $(CPPFLAGS) -c $<

OBJS=fastnew.o mem_aloc.o mem_bmap.o mem_clst.o mem_node.o mem_vsiz.o test.o

LIBS=libfastalloc.a

PROGS=vtest mem_clst

all: lib vtest mem_clst

lib: ${OBJS}
	rm -f libfastalloc.a
	ar qv libfastalloc.a ${OBJS}

vtest: ${OBJS} test.o
	${CXX} -g -pg -o vtest ${OBJS} test.o 

mem_clst: mem_clst.o
	${CXX} -g -pg -o mem_clst mem_clst.cpp -DTEST

clean:
	@echo Cleaning up.
	rm -f ${OBJS}  ${PROGS}

squeaky: clean
	@echo Making it squeaky.
	rm -f ${LIBS}
	rm -f *~ *.bak *.exe
