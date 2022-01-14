# Avengers: assemble ARM assembly to logisim compatible binary format
## How to compile avengers
`cmake -S . -B build`
`cmake --build cmake-build-debug`

## How to use avengers
### How to compile C to assembly
`clang -S --target=arm-none-eabi -mcpu=cortex-m0 -O0 -mfloat-abi=soft main.c`

### How to assemble assembly to binary
`avengers assembly.s`

File is written to `assembly.bin`

## How to run the tests
Run `./test.sh` (you must be in the same directory)

The script needs to be run in a UNIX environment, with bash and git installed.
cmake is not mandatory but is the only supported way of building avengers.

The tests can run as long as the avengers binary has been put in the right directory.

[parm_public](https://bitbucket.org/edge-team-leat/parm_public) needs to be cloned in the same parent directory as this repo

avengers must be compiled in the directory cmake-build-debug

The script will, for each assembly file (*.s) in parm_public, call avengers which will assemble in a .bin file, with the same name and in the same directory.

It will then use `git diff` to check if the assembly is equals to the one provided in parm_public.

- `no difference` means the file has no difference.
- `M something` means there is a difference.

There should be no difference for the tests to pass.
