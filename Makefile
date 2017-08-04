CPP := g++
REMOVE := rm

CPPFLAGS := -std=c++11 -g
LFLAGS :=

OBJECTS := main.o network.o Servant.o

servant: $(OBJECTS)
	$(CPP) -o $@ $(OBJECTS) $(LFLAGS)

%.o: %.cpp Servant.h
	$(CPP) -c $(CPPFLAGS) $<

.PHONY: clean
clean:
	$(REMOVE) $(OBJECTS)
