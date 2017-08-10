CPP := g++
REMOVE := rm

CPPFLAGS := -std=c++11 -O2
LFLAGS := -pthread -s

OBJECTS := main.o os.o network.o Servant.o Session.o Resource.o
HEADERS := Servant.h Session.h Resource.h

servant: $(OBJECTS)
	$(CPP) -o $@ $(OBJECTS) $(LFLAGS)

%.o: %.cpp $(HEADERS)
	$(CPP) -c $(CPPFLAGS) $<

.PHONY: clean
clean:
	$(REMOVE) $(OBJECTS)
