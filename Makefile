# Italo Tobler Silva
# Makefile for project containing C and C++ files
CFLAGS = -g -Wall
CPPFLAGS = -g -Wall
LDFLAGS = -lX11 -lpthread -lXtst
PROJECT = autoclicker
CC = g++
CXX = g++
BINDIR = bin
SRCDIR = .
LIBDIR = .
CLIBS := $(wildcard $(LIBDIR)/*.h)
CPPLIBS := $(wildcard $(LIBDIR)/*.hpp)
CFILES := $(wildcard $(SRCDIR)/*.c)
CPPFILES := $(wildcard $(SRCDIR)/*.cpp)
CPPOBJS := $(patsubst $(SRCDIR)/%.cpp, $(BINDIR)/%.o, $(CPPFILES))
COBJS := $(patsubst $(SRCDIR)/%.c, $(BINDIR)/%.o, $(CFILES))

all : build

build : $(BINDIR) $(COBJS) $(CPPOBJS)
	$(CXX) $(COBJS) $(CPPOBJS) $(LDFLAGS) -o $(PROJECT)

$(BINDIR) :
	mkdir -p $@

$(COBJS):$(BINDIR)/%.o : $(SRCDIR)/%.c $(CLIBS)
	$(CC) -c $< -I $(LIBDIR) $(CFLAGS) -o $@

$(CPPOBJS):$(BINDIR)/%.o : $(SRCDIR)/%.cpp $(CPPLIBS)
	$(CXX) -c $< -I $(LIBDIR) $(CPPFLAGS) -o $@

clean :
	rm -f $(BINDIR)/*.o
	rm -f $(PROJECT).zip
	rm -f $(PROJECT)
	rm -f debug*.out
	clear

run : build
	./$(PROJECT)

.zip : clean
	zip -r $(PROJECT).zip $(SRCDIR) $(LIBDIR) Makefile

debug : all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(PROJECT) 2> debug.out

