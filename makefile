export CC=g++
export CFLAGS
override CFLAGS += -std=c++0x -pthread -Werror -Wall -Wextra\
	-Wno-unused-parameter
release: CFLAGS += -O2
optimize: CFLAGS += -O3 -march=native
debug: CFLAGS += -g
test: CFLAGS += -iquote=$(CURDIR)
LIBS=-lpng -lm
LINKFLAGS=-Wl,--no-as-needed
EXENAME=ray
# test directory
TDIR=test
TEXENAME=run_tests
# build directory
BDIR=.build
# build directory for tests
BTDIR=$(BDIR)/$(TDIR)

THIRDPARTY=3rdparty
LIBPATH=-L$(THIRDPARTY)/lib
INCPATH=-I$(THIRDPARTY)/include

.PHONY: all release debug optimize clean run test

all: release

release: $(EXENAME)

optimize: $(EXENAME)

debug: $(EXENAME)

$(BDIR):
	mkdir -p $(BDIR)

$(EXENAME): $(BDIR)/main.o $(BDIR)/image.o $(BDIR)/scene.o
	$(CC) $(BDIR)/main.o $(BDIR)/image.o $(BDIR)/scene.o -o $(EXENAME)\
		 $(CFLAGS) $(LIBPATH) -lyaml-cpp $(LIBS) $(LINKFLAGS)

$(BDIR)/main.o: main.cxx *.h | $(BDIR)
	$(CC) $< -c -o $@ $(CFLAGS)

$(BDIR)/image.o: image.cxx image.h vec3f.h | $(BDIR)
	$(CC) $< -c -o $@ $(CFLAGS)

$(BDIR)/scene.o: scene.cxx scene.h vec3f.h | $(BDIR)
	$(CC) $< -c -o $@ $(CFLAGS) $(INCPATH) 

run:
	./$(EXENAME)

# tests
$(BTDIR):
	mkdir -p $(BTDIR)

$(BTDIR)/test_geometry.o: $(TDIR)/test_geometry.cxx $(TDIR)/test.h\
	geometry.h | $(BTDIR)
	$(CC) $< -c -o $@ $(CFLAGS)

$(BTDIR)/test_image.o: $(TDIR)/test_image.cxx $(TDIR)/test.h | $(BTDIR)
	$(CC) $< -c -o $@ $(CFLAGS)

$(BTDIR)/test_main.o: $(TDIR)/test_main.cxx | $(BTDIR)
	$(CC) $< -c -o $@ $(CFLAGS)

$(TEXENAME): $(BTDIR)/test_main.o $(BTDIR)/test_image.o\
	$(BTDIR)/test_geometry.o
	$(CC) $(BTDIR)/test_main.o $(BTDIR)/test_image.o\
		$(BTDIR)/test_geometry.o -o $(TEXENAME) $(CFLAGS)\
		$(LIBS) $(LINKFLAGS)

test: $(TEXENAME)
	./$(TEXENAME)

clean:
	rm -rf $(BDIR) $(EXENAME) $(TEXENAME)
