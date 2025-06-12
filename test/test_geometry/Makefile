CXX = g++
CXXFLAGS = -std=c++11 -Wall -I.

test_geometry: main.cpp geometry.cpp geometry_test.h
	$(CXX) $(CXXFLAGS) main.cpp geometry.cpp -o test_geometry

clean:
	rm -f test_geometry
