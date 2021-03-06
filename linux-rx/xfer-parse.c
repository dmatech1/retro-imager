#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

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
    uint16_t    cylinder;
    uint8_t     head;
    uint8_t     sector;
    uint16_t    crc16;          // CRC-16 of entire packet up to this point.
};

struct data_packet_t {
    uint8_t     cmd;            // Always 1.
    uint8_t     status;
    uint16_t    cylinder;
    uint8_t     head;
    uint8_t     sector;
    uint8_t     contents[512];  // Actual contents of the sector.
    uint16_t    crc16;          // CRC-16 of entire packet up to this point.
};
#pragma pack(pop)

const char *get_disk_error(uint8_t status) {
    switch (status) {
    case 0x00:  return "no error";
    case 0x01:  return "invalid command";
    case 0x02:  return "address mark not found";
    case 0x03:  return "disk write-protected (F)";
    case 0x04:  return "sector not found";
    case 0x05:  return "reset failed (H)";
    case 0x06:  return "floppy disk removed (F)";
    case 0x07:  return "bad parameter table (H)";
    case 0x08:  return "DMA overrun (F)";
    case 0x09:  return "DMA crossed 64 KB boundary";
    case 0x0A:  return "bad sector flag (H)";
    case 0x0B:  return "bad track flag (H)";
    case 0x0C:  return "media type not found (F)";
    case 0x0D:  return "invalid number of sectors on format (H)";
    case 0x0E:  return "control data address mark detected (H)";
    case 0x0F:  return "DMA arbitration level out of range (H)";
    case 0x10:  return "uncorrectable CRC or ECC data error";
    case 0x11:  return "ECC corrected data error (H)";
    case 0x20:  return "controller failed";
    case 0x40:  return "seek failed";
    case 0x80:  return "disk timed out (failed to respond)";
    case 0xAA:  return "drive not ready (H)";
    case 0xBB:  return "undefined error (H)";
    case 0xCC:  return "write fault (H)";
    case 0xE0:  return "status register error (H)";
    case 0xFF:  return "sense operation failed (H)";
    default:    return "unknown error code";
    }
}


ssize_t readfully(int fd, void *buf, size_t count) {
    char *b = (char*) buf;
    size_t total_count = 0;
    ssize_t r;

    while (count > 0) {
        r = read(fd, b, count);
        if (r == -1) {
            return -1;
        } else if (r == 0) {
            return total_count;
        } else {
            count -= r;
            total_count += r;
            b += r;
        }
    }
    return total_count;
}



int main(int argc, char **argv) {
    uint8_t cmd;
    struct header_packet_t hp;
    struct data_packet_t  dp;
    FILE *f;
    
    f = fopen("hardcard.img", "wb");

    for (;;) {
        int z = readfully(STDIN_FILENO, &cmd, sizeof(uint8_t));
        if (z != 1) {
            printf("wtf? %i\n", z);
            fclose(f);
            return 1;
        } else if (cmd == 0) {
            hp.cmd = cmd;
            readfully(STDIN_FILENO, &hp.status, sizeof(hp) - 1);
            uint16_t expected_crc16 = crc16_update(0, &hp, sizeof(hp) - sizeof(uint16_t));

            printf("Header: [%i, %i, %i], status=%i, crc16=%i, expected_crc16=%i\n", hp.cylinder, hp.head, hp.sector, hp.status, hp.crc16, expected_crc16);
        } else if (cmd == 1) {
            dp.cmd = cmd;
            readfully(STDIN_FILENO, &dp.status, sizeof(dp) - 1);
            uint16_t expected_crc16 = crc16_update(0, &dp, sizeof(dp) - sizeof(uint16_t));
            long lba = (dp.cylinder * (hp.head + 1) + dp.head) * hp.sector + dp.sector - 1;

            printf("Sector: [%i, %i, %i], status=%i, crc16=%i, expected_crc16=%i, lba=%li, good=%c\n", dp.cylinder, dp.head, dp.sector, dp.status, dp.crc16, expected_crc16, lba,
                ((expected_crc16 == dp.crc16) && (dp.status == 0)) ? 'Y' : 'N');

            // There should be a seek here, but right now, I don't care.
            fwrite(dp.contents, 512, 1, f);

        } else if (cmd == 0xFF) {
            printf("Done!\n");
        }
    }
    fclose(f);

}
