CXX ?= g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -Itests/stubs
SKETCH := firmware/two_tank_controller/two_tank_controller.ino

.PHONY: check

check:
	$(CXX) $(CXXFLAGS) -x c++ -fsyntax-only $(SKETCH)
