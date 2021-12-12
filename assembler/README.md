# Avengers: assemble ARM assembly to logisim compatible binary format
## How to compile avengers
`cmake -S . -B build`
`cmake --build build`

## How to use avengers
### How to compile C to assembly
`clang -S --target=arm-none-eabi -mcpu=cortex-m0 -O0 -mfloat-abi=soft main.c`

### How to assemble assembly to binary
`avengers assembly.s`

File is written to `assembly.bin`