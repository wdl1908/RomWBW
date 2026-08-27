/* RNDTEST.C -- minimal random-data write/read-back test for an
   I2CEEPROM DIO unit, via the real HBIOS DIO dispatch (BF_DIOSEEK/
   READ/WRITE). Scoped strictly to DIODEV_I2CEEPROM ($12). Always tests
   block 0.

   No argument: auto-selects if exactly one I2CEEPROM unit is active,
   otherwise lists all active units (or says none found) and stops.
   One argument (unit number): tests that specific unit directly.
 */

#include <stdio.h>
#include "hbio.h"

#define DIODEV_I2CEEPROM 0x12
#define MAXBLKSIZ	128	/* driver caps SHIFT at 7, BLKSIZ can't exceed this */
#define MAXUNITS	16	/* devinfo() scan range, 0-15 */

unsigned char wbuf[MAXBLKSIZ], rbuf[MAXBLKSIZ];

unsigned char *tail = (unsigned char *)0x80;
unsigned char tailidx, taillen;

nexttoken(buf, maxlen)
char *buf;
unsigned char maxlen;
{
    unsigned char n;

    while (tailidx <= taillen && tail[tailidx] == ' ')
	tailidx++;
    n = 0;
    while (tailidx <= taillen && tail[tailidx] != ' ' && n < maxlen - 1) {
	buf[n] = tail[tailidx];
	n++;
	tailidx++;
    }
    buf[n] = 0;
    return n;
}

parsenum(s)
char *s;
{
    unsigned int v;

    v = 0;
    while (*s >= '0' && *s <= '9') {
	v = v * 10 + (*s - '0');
	s++;
    }
    return v;
}

dumpbuf(label, buf, len)
char *label;
unsigned char *buf;
unsigned int len;
{
    unsigned char i;

    printf("%s:", label);
    for (i = 0; i < len; i++) {
	if ((i & 0x0F) == 0)
	    printf("\n%5u: ", i);
	printf("%02x", buf[i]);
	putchar(' ');
    }
    putchar('\n');
}

/* Z80 R register into rseed -- free-running refresh counter, a cheap
   seed source with no dedicated hardware RNG available */
unsigned char rseed;

getrseed()
{
#asm
    LD	A,R
    LD	(_rseed),A
#endasm
}

/* scan all DIO units for I2CEEPROM devices, filling units[] with their
   unit numbers. returns the count found (0 if none) */
scanunits(units)
unsigned char *units;
{
    unsigned char u, dtype, cnt;
    unsigned int addr;

    cnt = 0;
    for (u = 0; u < MAXUNITS; u++) {
	if (devinfo(u, &dtype, &addr) == 0 && dtype == DIODEV_I2CEEPROM) {
	    units[cnt] = u;
	    cnt++;
	}
    }
    return cnt;
}

/* print one line per active unit: number, I2C address, geometry */
listunits(units, cnt)
unsigned char *units;
unsigned char cnt;
{
    unsigned char i, u, dtype;
    unsigned int addr, blksz, blkcnt;

    for (i = 0; i < cnt; i++) {
	u = units[i];
	devinfo(u, &dtype, &addr);
	blksz = blksize(u, &blkcnt);
	printf("Unit %u: I2C ADDR=0x%02x BLKSIZ=%u BLKCNT=%u\n",
	    u, addr, blksz, blkcnt);
    }
}

main()
{
    char unitstr[8];
    unsigned char i, x, mismatch, bank, dtype;
    char unit;
    unsigned char units[MAXUNITS], cnt;
    unsigned int addr, blksz, blkcnt;

    bank = getbank();
    getrseed();

    taillen = tail[0];
    tailidx = 1;

    if (nexttoken(unitstr, sizeof(unitstr)) != 0) {
	/* explicit unit given, use it directly */
	unit = parsenum(unitstr);
	if (devinfo(unit, &dtype, &addr) != 0 || dtype != DIODEV_I2CEEPROM) {
	    printf("Unit %u is not an I2CEEPROM device.\n", unit);
	    return;
	}
    } else {
	cnt = scanunits(units);
	if (cnt == 0) {
	    printf("No I2CEEPROM units found.\n");
	    return;
	}
	if (cnt == 1) {
	    unit = units[0];
	} else {
	    printf("%u I2CEEPROM units found:\n", cnt);
	    listunits(units, cnt);
	    printf("Re-run with a unit number to test one, e.g. RNDTEST %u\n", units[0]);
	    return;
	}
    }

    blksz = blksize(unit, &blkcnt);
    if (blksz == 0 || blksz > MAXBLKSIZ) {
	printf("Unit %u reports an unusable block size (%u).\n", unit, blksz);
	return;
    }
    printf("Unit %u: BLKSIZ=%u BLKCNT=%u\n", unit, blksz, blkcnt);

    /* fill wbuf with pseudo-random bytes, an 8-bit LCG (x = x*141+1),
       seeded off rseed | 1 so a zero seed can't produce a degenerate
       all-zero run */
    x = rseed | 1;
    for (i = 0; i < blksz; i++) {
	x = (x * 141 + 1) & 0xFF;
	wbuf[i] = x;
    }

    if (seekblock(unit, 0) != 0 || writeblock(unit, wbuf, bank) != 0) {
	printf("Write error on unit %u\n", unit);
	return;
    }
    if (seekblock(unit, 0) != 0 || readblock(unit, rbuf, bank) != 0) {
	printf("Read error on unit %u\n", unit);
	return;
    }

    mismatch = 0;
    for (i = 0; i < blksz; i++) {
	if (wbuf[i] != rbuf[i]) {
	    if (mismatch == 0) {
		printf("FAIL: first mismatch at offset %u (wrote 0x", i);
		printf("%02x", wbuf[i]);
		printf(", read 0x");
		printf("%02x", rbuf[i]);
		printf(")\n");
	    }
	    mismatch++;
	}
    }
    if (mismatch == 0)
	printf("PASS: unit %u block 0, %u bytes verified\n", unit, blksz);
    else
	printf("FAIL: unit %u, %u byte(s) mismatched\n", unit, mismatch);

    dumpbuf("Wrote", wbuf, blksz);
    dumpbuf("Read ", rbuf, blksz);
}
