# Game Boy emulator

Example build commands (Windows): produced executable is `build/bin/app.exe`
```sh
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release -G "Ninja"
cd build
ninja
```

## useful resources:
- [Pan Docs](https://gbdev.io/pandocs/)
- [GBDev opcode table](https://gbdev.io/gb-opcodes/optables/octal)
- [Game Boy: Complete Technical Reference](https://gekkio.fi/files/gb-docs/gbctr.pdf)
- [Game Boy CPU internals](https://gist.github.com/SonoSooS/c0055300670d678b5ae8433e20bea595)
- [DAA explanation](https://blog.ollien.com/posts/gb-daa/)