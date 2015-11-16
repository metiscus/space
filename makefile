COPTS= -I. -Icommon `pkg-config --cflags sdl2`

CXX := g++
CXXFLAGS := -g -Wall -Wextra -std=c++11  $(COPTS)
LDFLAGS := `pkg-config --libs sdl2`

# Rule for building .o from .cpp with dependency generation
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $*.cpp -o $*.o
	$(CXX) -MM $(CXXFLAGS) $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

default: libcommon.a

COMMON_SRC =\
	common/Object.h common/Object.cpp\
	common/Ship.h common/Ship.cpp

COMMON_CPP = $(filter %.cpp,$(COMMON_SRC))
COMMON_OBJ = $(COMMON_CPP:.cpp=.o)
-include $(COMMON_OBJ:.o=.d)
libcommon.a: $(COMMON_OBJ)
	ar -rcs libcommon.a $(COMMON_OBJ)

clean:
	-rm -f libcommon.a $(COMMON_OBJ)
	-find -iname "*.d" -exec rm -f {} \; -print
