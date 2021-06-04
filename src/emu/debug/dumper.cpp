#include "dumper.h"
#include "libretro.h"
#include "filesys.h"
#include "file.h"
#include "bytes.h"
#include <iomanip>
#include <bitset>

class Dumper::Impl final:
        public Dumper
{
public:
    explicit Impl(Timeline * timeline, Memory * memory, Cpu * cpu, Keyboard * keyboard):
        timeline_m(timeline), memBanks_m(memory->memBanks()), ioPorts_m(memory->ioPorts()), cpuRegs_m(cpu->cpuRegs()),
        retroKeyboardHook_m(keyboard->createRetroKeyboardHook(memFunc(this, &Impl::retroKeyboardHookFunc)))
    {}

    virtual void init() override
    {
    }

    virtual void reset() override
    {
    }

    virtual void close() override
    {
    }

    virtual void dump() override
    {
        std::string dumpDir = LibRetro::dumpDir();
        if(dumpDir.empty()) {
            msg(LogLevel::error, "Dump directory doesn't specified");
            return;
        }

        FileSys fileSys;
        captureLog(&fileSys);
        if(!fileSys.mkdir(dumpDir)) {
            return;
        }

        std::string fileName = stringf("%s/dump-%d-%06d-%06d", dumpDir.data(),
                                       timeline_m->startTime(), timeline_m->frameNum(), cpuRegs_m->clock);
        std::unique_ptr<FileWriter> file = FileWriter::create();
        captureLog(file.get());

        file->open(fileName + ".ram.bin", true);
        file->write(memBanks_m->ram.data(), memBanks_m->ram.size());

        std::stringstream regDump;

        regDump << "    TIMELINE" << std::endl
                << std::endl;

        regDump << std::setbase(10)
                << "frame   = " << timeline_m->frameNum() << std::endl
                << "clock   = " << cpuRegs_m->clock << std::endl
                << std::endl;

        regDump << "    FLAGS" << std::endl
                << std::endl;

        regDump << "          sz a p c" << std::endl
                << "flags   = " << std::bitset<8>(cpuRegs_m->f) << std::endl
                << "inte    = " << (cpuRegs_m->state & CpuState::inte ? "true" : "false") << std::endl
                << "halt    = " << (cpuRegs_m->state & CpuState::halt ? "true" : "false") << std::endl
                << std::endl;

        regDump << "    REGISTERS" << std::endl
                << std::endl;

        regDump << std::setbase(16) << std::setfill('0')
                << "a       = " << std::setw(2) << uint16_t(cpuRegs_m->a) << std::endl
                << "bc      = " << std::setw(4)
    #if BYTE_ORDER == BIG_ENDIAN
                << byteswap(cpuRegs_m->bc) << std::endl
    #elif BYTE_ORDER == LITTLE_ENDIAN
                << cpuRegs_m->bc << std::endl
    #endif
                << "de      = " << std::setw(4)
    #if BYTE_ORDER == BIG_ENDIAN
                << byteswap(cpuRegs_m->de) << std::endl
    #elif BYTE_ORDER == LITTLE_ENDIAN
                << cpuRegs_m->de << std::endl
    #endif
                << "hl      = " << std::setw(4)
    #if BYTE_ORDER == BIG_ENDIAN
                << byteswap(cpuRegs_m->hl) << std::endl
    #elif BYTE_ORDER == LITTLE_ENDIAN
                << cpuRegs_m->hl << std::endl
    #endif
                << "sp      = " << std::setw(4)
    #if BYTE_ORDER == BIG_ENDIAN
                << byteswap(cpuRegs_m->sp) << std::endl
    #elif BYTE_ORDER == LITTLE_ENDIAN
                << cpuRegs_m->sp << std::endl
    #endif
                << "pc      = " << std::setw(4)
    #if BYTE_ORDER == BIG_ENDIAN
                << byteswap(cpuRegs_m->pc) << std::endl
    #elif BYTE_ORDER == LITTLE_ENDIAN
                << cpuRegs_m->pc << std::endl
    #endif
                << std::endl;

        regDump << "    PORTS" << std::endl
                << std::endl;

        regDump << std::setbase(16) << std::setfill('0')
                << "80      = " << std::setw(2) << uint16_t(ioPorts_m->port80)    << " " << std::bitset<8>(ioPorts_m->port80)    << std::endl
                << "81      = " << std::setw(2) << uint16_t(ioPorts_m->port81[0]) << " " << std::bitset<8>(ioPorts_m->port81[0]) << std::endl
                << "          " << std::setw(2) << uint16_t(ioPorts_m->port81[1]) << " " << std::bitset<8>(ioPorts_m->port81[1]) << std::endl
                << "          " << std::setw(2) << uint16_t(ioPorts_m->port81[2]) << " " << std::bitset<8>(ioPorts_m->port81[2]) << std::endl
                << "          " << std::setw(2) << uint16_t(ioPorts_m->port81[3]) << " " << std::bitset<8>(ioPorts_m->port81[3]) << std::endl
                << "          " << std::setw(2) << uint16_t(ioPorts_m->port81[4]) << " " << std::bitset<8>(ioPorts_m->port81[4]) << std::endl
                << "          " << std::setw(2) << uint16_t(ioPorts_m->port81[5]) << " " << std::bitset<8>(ioPorts_m->port81[5]) << std::endl
                << "          " << std::setw(2) << uint16_t(ioPorts_m->port81[6]) << " " << std::bitset<8>(ioPorts_m->port81[6]) << std::endl
                << "          " << std::setw(2) << uint16_t(ioPorts_m->port81[7]) << " " << std::bitset<8>(ioPorts_m->port81[7]) << std::endl
                << "          " << std::setw(2) << uint16_t(ioPorts_m->port81[8]) << " " << std::bitset<8>(ioPorts_m->port81[8]) << std::endl
                << "          " << std::setw(2) << uint16_t(ioPorts_m->port81[9]) << " " << std::bitset<8>(ioPorts_m->port81[9]) << std::endl
                << "82      = " << std::setw(2) << uint16_t(ioPorts_m->port82)    << " " << std::bitset<8>(ioPorts_m->port82)    << std::endl
                << "84      = " << std::setw(2) << uint16_t(ioPorts_m->port84)    << " " << std::bitset<8>(ioPorts_m->port84)    << std::endl
                << "85      = " << std::setw(2) << uint16_t(ioPorts_m->port85)    << " " << std::bitset<8>(ioPorts_m->port85)    << std::endl
                << "86      = " << std::setw(2) << uint16_t(ioPorts_m->port86)    << " " << std::bitset<8>(ioPorts_m->port86)    << std::endl
                << "88      = " << std::setw(2) << uint16_t(ioPorts_m->port88)    << " " << std::bitset<8>(ioPorts_m->port88)    << std::endl
                << "8c      = " << std::setw(2) << uint16_t(ioPorts_m->port8c)    << " " << std::bitset<8>(ioPorts_m->port8c)    << std::endl
                << "8d      = " << std::setw(2) << uint16_t(ioPorts_m->port8d)    << " " << std::bitset<8>(ioPorts_m->port8d)    << std::endl
                << "90      = " << std::setw(2) << uint16_t(ioPorts_m->port90)    << " " << std::bitset<8>(ioPorts_m->port90)    << std::endl
                << "91      = " << std::setw(2) << uint16_t(ioPorts_m->port91)    << " " << std::bitset<8>(ioPorts_m->port91)    << std::endl
                << "92      = " << std::setw(2) << uint16_t(ioPorts_m->port92)    << " " << std::bitset<8>(ioPorts_m->port92)    << std::endl
                << "93      = " << std::setw(2) << uint16_t(ioPorts_m->port93)    << " " << std::bitset<8>(ioPorts_m->port93)    << std::endl
                << std::endl;

        file->open(fileName + ".reg.txt", true);
        file->write(reinterpret_cast<uint8_t *>(regDump.str().data()), regDump.str().length());
    }
private:
    void retroKeyboardHookFunc(RetroKeyboardKey key, bool isPressed, bool * isConsumed)
    {
        if(*isConsumed) {
            return;
        }
        switch(key) {
            case RETROK_F6:
                if(isPressed) {
                    dump();
                    LibRetro::popupMsg("Dump created");
                    *isConsumed = true;
                }
                break;
            default:
                break;
        }
    }

    Timeline * timeline_m;
    MemBanks * memBanks_m;
    IoPorts * ioPorts_m;
    CpuRegs * cpuRegs_m;

    std::unique_ptr<RetroKeyboardHook> retroKeyboardHook_m;
};

auto Dumper::create(Timeline * timeline, Memory * memory, Cpu * cpu, Keyboard * keyboard)
                    -> std::unique_ptr<Dumper>
{
    return std::make_unique<Impl>(timeline, memory, cpu, keyboard);
}
