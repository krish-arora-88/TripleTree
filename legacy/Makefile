TEST_MAIN = testpa3

OBJS_TREE = tripletree.o tripletree_given.o
OBJS_MAIN = testpa3.o
OBJS_UTILS  = lodepng.o RGBAPixel.o PNG.o

INCLUDE_TREE = tripletree.h
INCLUDE_UTILS = cs221util/PNG.cpp cs221util/PNG.h cs221util/RGBAPixel.h cs221util/lodepng/lodepng.h

CXX = clang++
LD = clang++
CXXFLAGS = -std=c++1y -c -g -O0 -Wall -Wextra -pedantic
LDFLAGS = -std=c++1y -lpthread -lm

all: $(TEST_MAIN)

$(TEST_MAIN) : $(OBJS_UTILS) $(OBJS_TREE) $(OBJS_MAIN)
	$(LD) $^ $(LDFLAGS) -o $@

testpa3.o : testpa3.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# Pattern rules for object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

PNG.o : cs221util/PNG.cpp $(INCLUDE_UTILS)
	$(CXX) $(CXXFLAGS) $< -o $@

RGBAPixel.o : cs221util/RGBAPixel.cpp $(INCLUDE_UTILS)
	$(CXX) $(CXXFLAGS) $< -o $@

lodepng.o : cs221util/lodepng/lodepng.cpp cs221util/lodepng/lodepng.h
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(TEST_MAIN) $(OBJS_DIR) *.o
