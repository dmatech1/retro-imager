# retro-imager
Transfers hard disk data from a DOS 3.1 computer to a Linux/WSL computer.

# Introduction
The purpose of this program is to transfer data from an [IBM PC-XT](https://en.wikipedia.org/wiki/IBM_Personal_Computer_XT) running PC-DOS 3.10 my parents gave me.  The first transport method will be a standard serial port using anything from an [8250](https://en.wikipedia.org/wiki/8250_UART) to a [16550A UART](https://en.wikipedia.org/wiki/16550_UART) (with the receiving end using a null modem and a USB to serial adapter.  If it's useful, a version using a parallel port could be created (assuming anyone's willing to get the requisite adapters).  The receiving end will write out an image along with any metadata that can be gathered.

# Compatibility and Performance
Because the transmission program will need to run on a computer with very little computing power and support for newer BIOS/DOS functions, I'm going to need to be careful with which features I use.  Just because it's supported in an emulated test environment doesn't mean it is supported on that device.  I just happen to have a copy of [Advanced MS-DOS Programming](https://www.amazon.com/Advanced-MS-DOS-Programming-Microsoft-Programmers/dp/1556151578) that contains compatibility information, and it looks like I'll need to use the [CHS](https://en.wikipedia.org/wiki/Cylinder-head-sector) technique for reading the data.  For performing the actual transmission, the default INT 14H serial routines are too slow.  I'll either need to use a [FOSSIL driver](http://ftsc.org/docs/fsc-0015.001) or just roll my own.

# Transmission Strategy
The easiest strategy would probably be to just do a one-shot transmission from source to destination like [tschak909/disk-xfer](https://github.com/tschak909/disk-xfer) does, but allow for disk read errors that get recorded on the receiving side.  At least I'll get most of the data this way.  A better approach would be for the receiving end to maintain something like a [`ddrescue`](https://en.wikipedia.org/wiki/Ddrescue) log that allows for automatic resumption and retries by sending the incomplete ranges to the sender.  Additionally, this would track exactly which error codes were returned for each range, and would prevent resuming with the wrong hard drive (assuming the geometry is different).

# Development Strategy
The first version will be written in C for convenience, but will probably be ported over to assembly so that it can be more easily built by others.  I'll likely do development on [DOSBox](https://www.dosbox.com/), and initial testing will likely use a file instead of serial output.  Testing will require [mounting a hard disk image](https://www.dosbox.com/wiki/IMGMOUNT#Hard_disk_images).

