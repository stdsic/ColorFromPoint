g++ -static-libgcc -static-libstdc++ -c -o main.o main.cpp -mwindows -municode
g++ -static-libgcc -static-libstdc++ -o ColorFromPoint.exe -s main.o -mwindows -municode -L. -lMyApiDll
