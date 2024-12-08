/*
 * pagewalk.c
 *
 * Public Domain by bzt
 *
 * @brief quick'n'dirty tool to display page tables from memory dumps, ugly as hell but gets the job done
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define phyaddr_x86(x) ((x)&~((0xFFUL<<56)|0xFFF))
#define phyaddr_arm(x) ((x)&~((0xFFFFUL<<48)|0xFFF))

uint8_t *buf = NULL;
uint64_t size = 0;

uint64_t get64(uint64_t addr)
{
    if(!buf || addr >= size - 8) {
        fprintf(stderr, "address %016lX is outside of memdump (0 - %lX)\n", addr, size);
        exit(1);
    }
    return *((uint64_t*)(buf + addr));
}

static void data(uint64_t p)
{
    uint64_t t;
    uint8_t buf[16];
    t = get64(p); memcpy(buf, &t, 8);
    t = get64(p+8); memcpy(buf+8, &t, 8);
    printf("data         %02X %02X %02X %02X %02X %02X %02X %02X  %02X %02X %02X %02X %02X %02X %02X %02X  %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15],
        buf[0] > ' ' && buf[0] < 127 ? buf[0] : '.', buf[1] > ' ' && buf[1] < 127 ? buf[1] : '.',
        buf[2] > ' ' && buf[2] < 127 ? buf[2] : '.', buf[3] > ' ' && buf[3] < 127 ? buf[3] : '.',
        buf[4] > ' ' && buf[4] < 127 ? buf[4] : '.', buf[5] > ' ' && buf[5] < 127 ? buf[5] : '.',
        buf[6] > ' ' && buf[6] < 127 ? buf[6] : '.', buf[7] > ' ' && buf[7] < 127 ? buf[7] : '.',
        buf[8] > ' ' && buf[8] < 127 ? buf[8] : '.', buf[9] > ' ' && buf[9] < 127 ? buf[9] : '.',
        buf[10] > ' ' && buf[10] < 127 ? buf[10] : '.', buf[11] > ' ' && buf[11] < 127 ? buf[11] : '.',
        buf[12] > ' ' && buf[12] < 127 ? buf[12] : '.', buf[13] > ' ' && buf[13] < 127 ? buf[13] : '.',
        buf[14] > ' ' && buf[14] < 127 ? buf[14] : '.', buf[15] > ' ' && buf[15] < 127 ? buf[15] : '.');
}

static void flags_x86(uint64_t p)
{
    printf("%c%c%c%c%c",(p>>11)&1?'L':'.',(p>>8)&1?'G':'.',(p>>7)&1?'T':'.',(p>>6)&1?'D':'.',(p>>5)&1?'A':'.');
    printf("%c%c%c%c%c",(p>>4)&1?'.':'C',(p>>3)&1?'W':'.',(p>>2)&1?'U':'S',(p>>1)&1?'W':'R',p&(1UL<<63)?'.':'X');
    if(!(p & 1)) printf(" (not present)"); else printf(" %08X", (uint32_t)phyaddr_x86(p));
}

static void flags_arm(uint64_t p)
{
    printf("%c%c%x%c%c", (p>>58)&1?'L':'.', (p>>63)&1?'N':'.', (p>>61)&3, (p>>60)&1?'U':'.', (p>>59)&1?'P':'.');
    printf("%c%c%c%x", (p>>53)&1?'P':'.', (p>>11)&1?'.':'G', (p>>10)&1?'A':'.', (p>>8)&3?(((p>>8)&3)==1?'I':'O'):'N');
    printf("%c%c%c%c%c", (p>>2)&3?((p>>2)==1?'D':'.'):'C', p&2?'.':'G', (p>>6)&1?'U':'S', (p>>7)&1?'R':'W', p&(1UL<<54)?'.':'X');
    if(!(p & 1)) printf(" (not present)"); else printf(" %08X", (uint32_t)phyaddr_arm(p));
}

static void walk_x86(uint64_t p, int lvl, int idx)
{
    uint64_t r = phyaddr_x86(p);
    int i;

    if(!p) return;
    for(i = 0; i < 4 - lvl; i++) printf("  ");
    printf("idx %3d %016lX ", idx, p); if(lvl != 4) flags_x86(p);
    if(r >= size - 8 && lvl && (r & 1)) printf(" (address is outside of memdump, not descending)");
    printf("\n");
    if(r >= size - 8 || !lvl || (lvl < 4 && (!(p & 1) || (p & 0x80)))) return;
    for(i = 0; i < 512; i++)
        walk_x86(get64(r + i * 8), lvl - 1, i);
}

static void walk_arm(uint64_t p, int lvl, int idx)
{
    uint64_t r = phyaddr_arm(p);
    int i;

    if(!p) return;
    for(i = 0; i < 3 - lvl; i++) printf("  ");
    printf("idx %3d %016lX ", idx, p); if(lvl != 3) flags_arm(p);
    if(r >= size - 8 && lvl && (r & 1)) printf(" (address is outside of memdump, not descending)");
    printf("\n");
    if(r >= size - 8 || !lvl || (lvl < 3 && (p & 3) != 3)) return;
    for(i = 0; i < 512; i++)
        walk_arm(get64(r + i * 8), lvl - 1, i);
}

uint64_t gethex(char *ptr)
{
    uint64_t ret = 0;
    if(ptr[0] == '0' && ptr[1] == 'x') ptr += 2;
    for(;*ptr;ptr++) {
        if(*ptr>='0' && *ptr<='9') {          ret <<= 4; ret += (uint32_t)(*ptr - '0'); }
        else if(*ptr >= 'a' && *ptr <= 'f') { ret <<= 4; ret += (uint32_t)(*ptr - 'a' + 10); }
        else if(*ptr >= 'A' && *ptr <= 'F') { ret <<= 4; ret += (uint32_t)(*ptr - 'A' + 10); }
        else if(*ptr != '_') break;
    }
    return ret;
}

int main(int argc, char **argv)
{
    FILE *f;
    uint64_t root = 0, address = 0, r;
    int arch = 0, tree = 0;

    /* parse arguments */
    if(argc < 4) {
        printf("\npagewalk <arch> <memdump> <root> [virtual address]\n\n");
        printf("  <arch>      x86_64 / aarch64\n");
        printf("  <memdump>   memory dump file (from address 0, as big as necessary)\n");
        printf("  <root>      paging table root in hex (value of CR3 / TTBR0 / TTBR1)\n");
        printf("  [address]   address to look up in hex (in lack draws a tree)\n\n");
        printf("Bochs (no need, use built-in \"info tab\" / \"page\" commands, but if you want):\n");
        printf("  1. press break button                 - get to debugger\n");
        printf("  2. writemem \"memdump\" 0 (size)        - create dump\n");
        printf("  3. creg                               - display value of CR3\n");
        printf("qemu:\n");
        printf("  1. Ctrl+Alt+2                         - get to debugger\n");
        printf("  2. pmemsave 0 (size) memdump          - create dump\n");
        printf("  3. info registers                     - display CR3 / TTBR\n\n");
        return 1;
    }
    if(!strcmp(argv[1], "aarch64")) arch = 1; else
    if(strcmp(argv[1], "x86_64")) { fprintf(stderr, "unknown arch\n"); return 1; }
    root = gethex(argv[3]);
    if(argc > 4) address = gethex(argv[4]); else tree = 1;

    /* read in memory dump */
    if(!(f = fopen(argv[2], "rb"))) { fprintf(stderr, "unable to open memdump file\n"); return 1; }
    fseek(f, 0, SEEK_END);
    size = (uint64_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    if(!(buf = (uint8_t*)malloc(size))) { fclose(f); fprintf(stderr, "unable to allocate memory for memdump\n"); return 1; }
    if(!fread(buf, 1, size, f)) { fclose(f); fprintf(stderr, "unable to read memdump\n"); return 1; }
    fclose(f);

    /* check root and do the parsing */
    get64(root); r = root;
    switch(arch) {
        case 0:
            if(tree) walk_x86(r, 4, 0);
            else {
                printf("CR3          %016lX\n", r);
                r = get64(phyaddr_x86(r) + ((address >> 39L) & 511) * 8);
                printf("PML4 idx %3d %016lX ", ((address >> 39L) & 511), r); flags_x86(r); printf("\n");
                if(r & 1) {
                    r = get64(phyaddr_x86(r) + ((address >> 30L) & 511) * 8);
                    printf("PDPE idx %3d %016lX ", ((address >> 30L) & 511), r); flags_x86(r); printf("\n");
                    if(r & 1) {
                        r = get64(phyaddr_x86(r) + ((address >> 21L) & 511) * 8);
                        printf("PDE  idx %3d %016lX ", ((address >> 21L) & 511), r); flags_x86(r); printf("\n");
                        if(!(r & 0x80) && (r & 1)) {
                            r = get64(phyaddr_x86(r) + ((address >> 12L) & 511) * 8);
                            printf("PTE  idx %3d %016lX ", ((address >> 12L) & 511), r); flags_x86(r); printf("\n");
                            if(r & 1) data(phyaddr_x86(r)); else printf("data (?)\n");
                        } else if(!(r & 1)) printf("PTE  (?)\ndata (?)\n"); else data(phyaddr_x86(r));
                    } else printf("PDE  (?)\nPTE  (?)\ndata (?)\n");
                } else printf("PDPE (?)\nPDE  (?)\nPTE  (?)\ndata (?)\n");
            }
        break;
        case 1:
            if(tree) walk_arm(r, 3, 0);
            else {
                printf("TTBR%d        %016lX\n", address >> 48 ? 1 : 0, r);
                r = get64(phyaddr_arm(r) + ((address >> 30L) & 511) * 8);
                printf("L1   idx %3d %016lX ", ((address >> 30L) & 511), r); flags_arm(r); printf("\n");
                if(r & 1) {
                    r = get64(phyaddr_arm(r) + ((address >> 21L) & 511) * 8);
                    printf("L2   idx %3d %016lX ", ((address >> 21L) & 511), r); flags_arm(r); printf("\n");
                    if((r & 3) == 3) {
                        r = get64(phyaddr_arm(r) + ((address >> 12L) & 511) * 8);
                        printf("L3   idx %3d %016lX ", ((address >> 12L) & 511), r); flags_arm(r); printf("\n");
                        if(r & 1) data(phyaddr_arm(r)); else printf("data (?)\n");
                    } else if(!(r & 1)) printf("L3   (?)\ndata (?)\n"); else data(phyaddr_arm(r));
                } else printf("L2   (?)\nL3   (?)\ndata (?)\n");
            }
        break;
    }

    free(buf);
    return 0;
}
