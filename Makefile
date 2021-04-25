TARGET = exelink.exe

SOURCES = \
	exelink.c 

LIBS = \
	Pathcch.lib \
	Shlwapi.lib

CFLAGS = /W4 /WX /MT /O2 /DNDEBUG /DUNICODE /D_UNICODE

.PHONY: all
all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) /Fe$@ $** /link $(LIBS)

clean:
	del exelink.exe exelink.obj
