#include <bus.h>
#include <cart.h>
#include <ram.h>
#include <cpu.h>
#include <io.h>
#include <ppu.h>
#include <dma.h>

// 0x0000 - 0x3FFF : ROM Bank 0
// 0x4000 - 0x7FFF : ROM Bank 1 - Switchable    
// 0x8000 - 0x97FF : CHR RAM
// 0x9800 - 0x9BFF : BG Map 1
// 0x9C00 - 0x9FFF : BG Map 2
// 0xA000 - 0xBFFF : Cartridge RAM
// 0xC000 - 0xCFFF : RAM Bank 0
// 0xD000 - 0xDFFF : RAM Bank 1-7 - switchable - Color only
// 0xE000 - 0xFDFF : Reserved - Echo RAM
// 0xFE00 - 0xFE9F : Object Attribute Memory
// 0xFEA0 - 0xFEFF : Reserved - Unusable
// 0xFF00 - 0xFF7F : I/O Registers
// 0xFF80 - 0xFFFE : Zero Page/High Ram

u8 bus_read(u16 address) {
    if (address < 0x8000) {
        return cart_read(address);
    } else if (address < 0xA000) {
        //Map and Character Data VRAM
        return ppu_vram_read(address);
    } else if (address < 0xC000) {
        //Cartridge RAM
        return cart_read(address);  
    } else if (address < 0xE000) {
        //RAM Banks (Working RAM)
        return wram_read(address);
    } else if (address < 0xFE00) {
        //Reserved - Echo RAM (not gonna use)
        return 0;
    } else if (address < 0xFEA0) {
        //object attribute memory
        if (dma_transferring()) {
            return 0xFF;
        }
        return ppu_oam_read(address);
    } else if (address < 0xFF00) {
        //Reserved (not gonna use)
        return 0;
    } else if (address < 0xFF80) {
        //I/O Registers
        return io_read(address);
    } else if (address == 0xFFFF) {
        //Interrupt enable register
        return cpu_get_ie_register();
    }

    //NOT_IMPL
    //remaining addresses: 0xFF80 - 0xFFFE
    return hram_read(address);
};

u16 bus_read16(u16 address) {
    u16 high = bus_read(address+1);
    u16 low = bus_read(address);

    return (low | (high << 8));
}

void bus_write(u16 address, u8 value) {
    if (address < 0x8000) {
        cart_write(address, value);
    } else if (address < 0xA000) {
        //Map and character data VRAM
        ppu_vram_write(address, value);
    } else if (address < 0xC000) {
        //Cartridge RAM
        cart_write(address, value);
    } else if (address < 0xE000) {
        //RAM Banks (Working RAM)
        wram_write(address, value);
    } else if (address < 0xFE00) {
        //Reserved (not gonna use)
    } else if (address < 0xFEA0) {
        //object attribute memory
        if (dma_transferring()) {
            return;
        }
        ppu_oam_write(address, value);
    } else if (address < 0xFF00) {
        //Reserved (not gonna use)
    } else if (address < 0xFF80) {
        //I/O REGISTERS
        io_write(address, value);
    } else if (address == 0xFFFF) {
        //Interrupt enable register
        //TODO
        cpu_set_ie_register(value);
    } else {
        hram_write(address, value);
    }

    return;
}; 

void bus_write16(u16 address, u16 value) {
    u16 high = (value >> 8) & 0xFF;
    u16 low = value & 0xFF;
    bus_write(address + 1, high);
    bus_write(address, low);

    return;
}
