<h1 align="center">ctmp</h1>
  
<p align="center">
    <img src="https://img.shields.io/badge/license-MIT-green?style=flat-square" alt="MIT License" />
  <img src="https://img.shields.io/github/last-commit/simon-danielsson/ctmp/main?style=flat-square&color=blue" alt="Last commit" />
</p>
  
<p align="center">
  <a href="#info">Info</a> •
  <a href="#install">Install</a> •
  <a href="#usage">Usage</a> •
  <a href="#license">License</a>
</p>  
  
---
<div id="info"></div>

## Info
  
This is my opinionated template for initializing, building, and maintaining C projects. Powered by [nob.h](https://github.com/tsoding/nob.h)
  
> [!IMPORTANT]
> This template is designed for myself specifically. If adopting this system yourself I recommend that you fork this repo and set it up to your own liking.
  
### Features
- Ultra portable
    - No dependencies except for a C compiler.
- Convert static files to header files automatically.
    - [nob.c](init/nob.c) collects all the files within 'src/static' and embeds their contents into header files for use within your program.
    - This feature is obviously not necessary if you're using [c23](https://en.cppreference.com/c/preprocessor/embed).
- Project environment variables (inspired by [uv](https://github.com/astral-sh/uv) and [cargo](https://github.com/rust-lang/cargo))
  
### Requirements
- gcc/clang/msvc
  
<div id="install"></div>
  
## Install
  
Append this function to your `.bashrc` (or an equivalent file in your shell path) and you're good to go.
  
``` bash
#!/usr/bin/env bash

cinit() {
    set -x
    local tmp=$(mktemp -d)
    git clone --depth 1 git@github.com:simon-danielsson/ctmp.git "$tmp"
    cp -r "$tmp/init" .
    rm -rf "$tmp"
    # TODO: uncomment this and comment the ssh code later when the repo is public
    # curl -L https://github.com/simon-danielsson/ctmp/archive/refs/heads/main.tar.gz \
    #     | tar -xz --strip-components=1 ctmp-main/init
    echo "#define PROJ_NAME \"$1\"" | cat - init/nob.c > init/tmp && mv -f init/tmp init/nob.c
    if [[ -d init ]]; then
        mv init "$1"
        cd "$1"
        gcc -o nob nob.c
        git init -b main
        git add --all
        git commit -m init
        git tag v0.1.0
        cd ..
    fi
    set +x
}
```
  
---
<div id="usage"></div>
  
## Usage
  
### Creating a new project
    
Run `cinit` with the name of your new project as an argument. A new project
folder will be created in the current directory. 
   
``` bash
cinit <project-name>
```
  
The generated project will have the following hierarchy:  
  
``` terminal
(root)
├── .clangd
├── LICENSE
├── nob
├── nob.c
├── nob.h
├── /src
│  ├── main.c
│  └── main.h
└── /tests
   ├── tests.c
   └── tests.h
```
  
### CLI commands (nob)
  
``` terminal
USAGE
    ./nob [options|command]

    * Execute without options to compile and run a debug build.
    * Program arguments are passed after a divider '--'.

META OPTIONS
    -h, --help             Display help.

OPTIONS
    -n, --no-run           Don't run after compilation.

COMMANDS
    test                   Run test(s).
    release                Compile and run a release build.
```
  
---
<div id="license"></div>
  
## License
  
This project is licensed under the [MIT License](https://github.com/simon-danielsson/ctmp/blob/main/LICENSE).  
 
