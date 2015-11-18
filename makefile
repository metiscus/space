COPTS= -I. -Icommon `pkg-config --cflags sdl2`

CXX := g++
CXXFLAGS := -g -Wall -Wextra -std=c++11  $(COPTS)
LDFLAGS := `pkg-config --libs sdl2` -lzmq -pthread

# Rule for building .o from .cpp with dependency generation
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $*.cpp -o $*.o
	$(CXX) -MM $(CXXFLAGS) $*.cpp > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

default: libcommon.a serverBin api_test

COMMON_SRC =\
	common/Object.h common/Object.cpp\
	common/Ship.h common/Ship.cpp

COMMON_CPP = $(filter %.cpp,$(COMMON_SRC))
COMMON_OBJ = $(COMMON_CPP:.cpp=.o)
-include $(COMMON_OBJ:.o=.d)
libcommon.a: $(COMMON_OBJ)
	ar -rcs libcommon.a $(COMMON_OBJ)

serverBin: libcommon.a server/server.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o serverBin server/server.o -L. -lcommon

api_test: libcommon.a client/api_test.o
		$(CXX) $(CXXFLAGS) $(LDFLAGS) -o api_test client/api_test.o -L. -lcommon

clean:
	-rm -f libcommon.a $(COMMON_OBJ) serverBin
	-find -iname "*.d" -exec rm -f {} \; -print
	-rm -f server/server.o client/api_test.o api_test

.PHONY: kill
kill:
	-kill `pgrep serverBin`
	-kill `pgrep api_test`

test:
	$(MAKE) && $(MAKE) kill
	./serverBin&
	./api_test
