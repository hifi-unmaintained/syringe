all: main dll

main:
	i586-mingw32msvc-gcc -std=c99 -I. -Wall -O2 -o syringe.exe main.c

dll:
	i586-mingw32msvc-gcc -std=c99 -I. -Wall -O2 -shared -o syringe.dll syringe.c assembly.c

clean:
	rm -f syringe.exe syringe.dll
