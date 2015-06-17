CFLAGS  = -g -O
TARGET   = libfftease
LIB = libfftease.a

DESTDIR =  . 
BINARIES = bloscbank.o convert.o fft.o fft4.o fold.o leanconvert.o \
leanunconvert.o makewindows.o overlapadd.o unconvert.o \
power_of_two.o PenroseRand.o PenroseOscil.o qsortE.o 


all:    $(TARGET)

clean:
	-rm -f a.out *.o ; wipe.pl

$(TARGET):	$(BINARIES)
	-ld -S -x -r $(BINARIES)
	-ar r $(LIB) $(BINARIES)
	-ranlib $(LIB)
	-mv $(LIB) $(DESTDIR)
