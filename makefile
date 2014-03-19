export CC=g++
export CFLAGS
override CFLAGS += -std=c++0x -Werror -Wall -Wextra -Wno-unused-parameter
release: CFLAGS += -O2
debug: CFLAGS += -g
test: CFLAGS += -iquote=$(CURDIR)
LIBS=-lpng -lGLEW -lGL -lGLU -lglut -lm
EXENAME=ray
# test directory
TDIR=test
TEXENAME=ray
# build directory
BDIR=.build
# build directory for tests
BTDIR=$(BDIR)/$(TDIR)

THIRDPARTY=3rdparty
LIBPATH=-L$(THIRDPARTY)/lib
INCPATH=-I$(THIRDPARTY)/include

all: release

release: main

debug: main

bdir:
	mkdir -p $(BDIR)

main: main.cxx vector_math.h vector_debug.h vec3f.h image bdir
	$(CC) $< -c -o $(BDIR)/$@.o $(CFLAGS) $(INCPATH) 
	$(CC) $(BDIR)/main.o $(BDIR)/image.o -o $(EXENAME) $(CFLAGS)\
		$(LIBPATH) -lyaml-cpp $(LIBS)

image: image.cxx image.h vec3f.h bdir
	$(CC) $< -c -o $(BDIR)/$@.o $(CFLAGS)

run:
	./$(EXENAME)

# tests
btdir:
	mkdir -p $(BTDIR)

test_geometry: $(TDIR)/test_geometry.cxx $(TDIR)/test.h btdir
	$(CC) $< -c -o $(BTDIR)/$@.o $(CFLAGS)

test_image: $(TDIR)/test_image.cxx $(TDIR)/test.h btdir
	$(CC) $< -c -o $(BTDIR)/$@.o $(CFLAGS)

test_main: $(TDIR)/test_main.cxx btdir
	$(CC) $< -c -o $(BTDIR)/$@.o $(CFLAGS)

link_test: test_main test_image test_geometry
	$(CC) $(BTDIR)/test_main.o $(BTDIR)/test_image.o\
		$(BTDIR)/test_geometry.o -o $(TEXENAME) $(CFLAGS) $(LIBS)

test: link_test
	./$(TEXENAME)

clean:
	rm -rf $(BDIR) $(EXENAME) $(TEXENAME)
