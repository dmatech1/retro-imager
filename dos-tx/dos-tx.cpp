#ifndef __TURBOC__
    // These would otherwise break intellisense.
    #define near
    #define far
    #define _seg
    #define interrupt
#endif


#include <stdio.h>


#include "dos.h"
#include "bios.h"

#ifdef __TURBOC__
    /* Turbo C++ doesn't have "stdint.h", so I'll define these here. */
    typedef long            int32_t;
    typedef unsigned long   uint32_t;

    typedef int             int16_t;
    typedef unsigned int    uint16_t;

    typedef unsigned char   uint8_t;
    typedef signed char     int8_t;
#else
    // Turbo C++ register pseudo-variables.  I'm not sure if it's wise to use them.

    #include "stdint.h"
    static uint16_t _AX, _BX, _CX, _DX, _SI, _DI, _BP, _SP,
    _CS, _DS, _ES, __SS, _FLAGS;

    static uint8_t  _AH, _AL, _BH, _BL, _CH, _CL, _DH, _DL;

    // Generate an interrupt.
    static void geninterrupt(uint8_t i) { }
#endif


// See http://ftsc.org/docs/fsc-0015.001
unsigned char int14_init(uint16_t port)
{
    union REGS regs;
    regs.x.dx = port;
    regs.h.ah = 0x04;       // Extended initialize (for FOSSIL)
    regs.x.bx = 0;
    int86(0x14,&regs,&regs);

    printf("ax=%x\n", regs.x.ax);

    if (regs.x.ax!=0x1954)
        return 1;

    // Change it to 38400.
    // http://www.oldlinux.org/Linux.old/docs/interrupts/int-html/rb-0812.htm
    regs.x.dx = port;
    regs.h.ah = 0;
    regs.h.al = 0x23;
    int86(0x14,&regs,&regs);
    
    // Set RTS/CTS flow control
    // http://www.oldlinux.org/Linux.old/docs/interrupts/int-html/rb-0894.htm
    regs.h.ah = 0x0f;
    regs.h.al = 0x02;
    regs.x.dx = port;
    int86(0x14,&regs,&regs);

    return 0;
}

/*
    Meant to serve as both input to and output from INT 13H calls.  Existing
    functions in Turbo C++ 3.0 were not sufficient.
*/
struct int13_data_t {
    uint16_t    cylinder;       // Range: 0..1023
    uint8_t     head;           // Range: 0..255
    uint8_t     sector;         // Range: 1..63

    uint8_t     sect_count;     // Number of sectors to read/write.

    void far    *buffer;        // Stuff that would go in ES:BX.
};


void int14_flush(uint16_t port) {
    union REGS regs;
    regs.x.dx = port;
    regs.h.ah = 0x08;
    int86(0x14,&regs,&regs);
}

void int14_deinit(uint16_t port) {
    union REGS regs;
    regs.x.dx = port;
    regs.h.ah = 0x05;
    int86(0x14,&regs,&regs);
}


/*
* Send byte
*/
void int14_send_byte(uint16_t port, uint8_t b)
{
    union REGS regs;
    regs.x.dx = port;
    regs.h.al = b;
    regs.h.ah = 0x01;
    int86(0x14,&regs,&regs);
}

void bulk_xmit(uint16_t port, const void *data, size_t count) {
    const uint8_t *d = (const uint8_t *) data;

    for (; count > 0; --count) {
        int14_send_byte(port, *d);
        d++;
    }
}

/*
    Makes a call to the INT 13H disk I/O routines.  I could possibly squeeze
    out a bit more performance (and user-friendliness) by not doing the
    sections that aren't needed for each command.
*/
uint8_t int13(uint8_t drive, uint8_t cmd, struct int13_data_t *data) {
    union REGS regs;
    struct SREGS seg_regs;
    segread(&seg_regs);

    seg_regs.es = FP_SEG(data->buffer);
    regs.x.bx = FP_OFF(data->buffer);
    regs.h.ah = cmd;
    regs.h.al = data->sect_count;
    regs.h.dh = data->head;
    regs.h.dl = drive;
    regs.h.ch = data->cylinder & 0xFF;

    // Lower 6 bits of the sector and top two bits of the 10-bit cylinder.
    regs.h.cl = (data->sector & 0x3F) | ((data->cylinder >> 2) & 0xC0);
    int86x(0x13, &regs, &regs, &seg_regs);

    // Now do the reverse.
    data->sect_count    = regs.h.al;
    data->head          = regs.h.dh;
    data->sector        = regs.h.cl & 0x3F;
    data->cylinder      = (regs.x.cx >> 8) | ((regs.x.cx << 2) & 0x300);

    return regs.h.ah;
}


/*
    Adapted from:
    python pycrc.py --model xmodem --algorithm table-driven --std=c89 --generate h -o crc.h
*/
static const uint16_t crc16_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

/*
    Adapted from:
    python pycrc.py --model xmodem --algorithm table-driven --std=c89 --generate c -o crc.c
*/
uint16_t crc16_update(uint16_t crc, const void *data, size_t data_len)
{
    const unsigned char *d = (const unsigned char *)data;
    unsigned int tbl_idx;

    while (data_len--) {
        tbl_idx = ((crc >> 8) ^ *d) & 0xff;
        crc = (crc16_table[tbl_idx] ^ (crc << 8)) & 0xffff;
        d++;
    }
    return crc & 0xffff;
}


#pragma pack(push)
#pragma pack(1)
struct header_packet_t {
    uint8_t     cmd;            // Always 0.
    uint8_t     status;         // Should be 0.
    uint16_t    cylinder;       // [0..1023]    Maximum cylinder number value.
    uint8_t     head;           // [0..255]     Maximum head number value.
    uint8_t     sector;         // [1..63]      Maximum sector number.
    uint16_t    crc16;          // CRC-16 of entire packet up to this point.
};

struct footer_packet_t {
    uint8_t     cmd;
};

struct data_packet_t {
    uint8_t     cmd;            // Always 1.
    uint8_t     status;         // Should be 0.
    uint16_t    cylinder;       // [0..1023]    Cylinder number.
    uint8_t     head;           // [0..255]     Head number.
    uint8_t     sector;         // [1..63]      Sector number.
    uint8_t     contents[512];  // Actual contents of the sector.
    uint16_t    crc16;          // CRC-16 of entire packet up to this point.
};
#pragma pack(pop)

struct data_packet_t packet;

#define SERIAL_PORT 0
#define DISK_NUMBER 0x80

int main(int argc, char **argv) {
    int13_data_t    di;
    uint16_t        max_cyl;
    uint8_t         max_head;
    uint8_t         max_sect;
    di.buffer = &packet.contents;

    int14_init(SERIAL_PORT);

    // Get the disk geometry.
    uint8_t n = int13(DISK_NUMBER, 8, &di);
    printf("n=%x, cylinders=0..%i, heads=0..%i, sectors=1..%i\n", n, di.cylinder, di.head, di.sector);
    max_cyl = di.cylinder;
    max_head = di.head;
    max_sect = di.sector;

    // Send the header packet.
    struct header_packet_t header;
    header.cmd = 0;
    header.cylinder = max_cyl;
    header.head = max_head;
    header.sector = max_sect;
    header.status = n;
    header.crc16 = crc16_update(0, &header, sizeof(header) - sizeof(uint16_t));
    bulk_xmit(SERIAL_PORT, &header, sizeof(header));

    // Now loop through all cylinders, heads, and sectors.
    for (uint16_t c=0; c <= max_cyl; c++) {
        for (uint8_t h=0; h <= max_head; h++) {
            for (uint8_t s = 1; s <= max_sect; s++) {

                di.sect_count = 1;
                di.cylinder = c;
                di.head = h;
                di.sector = s;

                n = int13(DISK_NUMBER, 2, &di);

                packet.cmd = 1;
                packet.status = n;
                packet.cylinder = c;
                packet.head = h;
                packet.sector = s;
                packet.crc16 = crc16_update(0, &packet, sizeof(packet) - sizeof(uint16_t));

                printf("n=%x, c=%i, h=%i, s=%i\n", n, c, h, s);

                // Send it.
                bulk_xmit(SERIAL_PORT, &packet, sizeof(packet));
            }
        }
    }

    // This is my end-of-transfer marker.  Not ideal, but it works.
    int14_send_byte(SERIAL_PORT, 0xFF);

    int14_flush(SERIAL_PORT);
    int14_deinit(SERIAL_PORT);

    return 0;
}

