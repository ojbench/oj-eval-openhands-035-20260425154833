CXX := g++
CXXFLAGS := -std=gnu++17 -O2 -pipe -static -s
LDFLAGS := 

all: code

code: code.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f code
