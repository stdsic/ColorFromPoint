g++ -static-libgcc -static-libstdc++ -c -o main.o main.cpp -mwindows
g++ -static-libgcc -static-libstdc++ -o ColorFromPoint.exe -s main.o -mwindows -L. -lMyApiDll
