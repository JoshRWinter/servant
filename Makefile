CPP := g++
REMOVE := rm

CPPFLAGS := -std=c++11 -g
LFLAGS := -pthread

OBJECTS := main.o network.o Servant.o Session.o
HEADERS := Servant.h Session.h

servant: $(OBJECTS)
	$(CPP) -o $@ $(OBJECTS) $(LFLAGS)

%.o: %.cpp $(HEADERS)
	$(CPP) -c $(CPPFLAGS) $<

.PHONY: clean
clean:
	$(REMOVE) $(OBJECTS)
