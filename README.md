# baba-is-you

A baba is you clone with c++ &amp; raylib

> This project is just for programming practice.  
> [Buy Baba Is You on Steam](https://store.steampowered.com/app/736260/Baba_Is_You/)

## quick start

### macOS Users

Dependence

```bash
brew install raylib
```

```bash
git clone https://github.com/qiekn/baba-is-you.git
cmake -B build && make -j$(nproc) -C build
./build/game
```

`compile_commands.json` for neovim lsp config:

```bash
cd path/to/your/project
ls -s build/compile_commands.json ./compile_commands.json
```
