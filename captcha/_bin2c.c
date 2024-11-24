#include <stdio.h>


int main(int argc, char **argv)
{
    FILE *f;
    int i, c;

    if(argc != 3) {
        fprintf(stderr, "Usage: %s <bin_file_name> <array_name>\n", *argv);
        return 1;
    }

    f = fopen(argv[1], "rb");
    if(!f) {
        perror(argv[1]);
        return 1;
    }
    printf("/* GENERATED WITH _bin2c, FROM FILE %s */\n", argv[1]);
    printf("const unsigned char %s[] = {\n    ", argv[2]);
    for(i = 0; (c = fgetc(f)) != EOF; i++) {
        printf("0x%02x%s", c, (i % 8 == 7) ? ",\n    " : ", ");
    }
    printf("\n};\n");
    fclose(f);
    return 0;
}
