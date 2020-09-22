# If you use threads, add -pthread here.
COMPILERFLAGS = -g -Wall -Wextra -Wno-sign-compare

# Any library you might need linked in
# LINKLIBS = -lm

SERVEROBJECTS = obj/server.o
CLIENTOBJECTS = obj/client.o
TALKEROBJECTS = obj/talker.o
LISTENEROBJECTS = obj/listener.o
# SOMEOBJECTS = obj/foo.o obj/foo.o

.PHONY: all clean

all: obj server client talker listener

server: $(SERVEROBJECTS)
    $(CC) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)

client: $(CLIENTOBJECTS)
    $(CC) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)

talker: $(TALKEROBJECTS)
    $(CC) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)

listener: $(LISTENEROBJECTS)
    $(CC) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)

clean:
    $(RM) obj/*.o server client talker listener

# compile and assemble but don't link
obj/%.o: src/%.c
    $(CC) $(COMPILERFLAGS) -c -o $@ $<

obj:
		mkdir -p obj
