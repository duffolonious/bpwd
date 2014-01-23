
#Lanner watchdog/bypass module code

EXT=
RM=rm -rf
CP=cp -f
BIN_PATH=../bin/

ifneq ($(DJGPP),)

EXT=.exe
RM=del
CP=copy
BIN_PATH=..\bin\

endif 

TARGET=bpwd_tst$(EXT)

all: $(TARGET)

$(TARGET): smbustst.o ioaccess.o
	-$(CC) $(CFLAGS) -o $(TARGET) smbustst.o ioaccess.o
	-$(CP) $(TARGET) $(BIN_PATH) 

smbustst.o: smbustst.c
	-$(CC) $(CFLAGS) -c $^

ioaccess.o: ioaccess.c
	-$(CC) $(CFLAGS) -c $^

clean:
	-$(RM) $(TARGET)
	-$(RM) smbustst.o
	-$(RM) ioaccess.o
