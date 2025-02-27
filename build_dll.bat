g++ -static-libgcc -static-libstdc++ -c -o MyApiDll.o MyApiDll.cpp -mwindows -D MYAPIDLL_EXPORTS
g++ -static-libgcc -static-libstdc++ -o MyApiDll.dll MyApiDll.o -mwindows -s -shared -Wl,--subsystem,windows
