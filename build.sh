PATH="$PATH:/usr/share/gnush_v1001_elf-1/bin"
C_FILES="assembler.c main.c g1a_utils.c linker.c"
G1A_NAME="mcasm.g1a"
GNU85_ROOT="/home/leo/GNU85"

INCLUDE_DIRS="-I$GNU85_ROOT/include/sh -I$GNU85_ROOT/include/revolution -I$GNU85_ROOT/include/fxlib"
LIB_DIRS="-L$GNU85_ROOT/lib"
LIBRARIES="-lstdio -lmonochrome -lsdk85 -lrevolution -lgcc"

sh-elf-gcc -m3 -mb -Os -nostdlib -T"addin.ld" $GNU85_ROOT/lib/crt0.o $C_FILES $LIB_DIRS $INCLUDE_DIRS $LIBRARIES -o myaddin.elf
sh-elf-objcopy -R .comment -R .bss -O binary myaddin.elf myaddin.bin
~/GNU85/g1awrapper/g1awrapper myaddin.bin -o $G1A_NAME -i icon.bmp
sleep 1000
