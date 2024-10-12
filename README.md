This project expects SFML 2.6.0 to be in SFML_DIR.

Example build commands (Windows):
cmake -S . -B ./build -G "NMake Makefiles" `
	-DCMAKE_BUILD_TYPE=Release `
	-D"SFML_DIR=./ext/SFML-2.6.0/lib/cmake/SFML"