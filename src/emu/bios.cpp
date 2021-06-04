#include "bios.h"
#include "libretro.h"
#include "cas.h"
#include <cstring>

constexpr size_t biosSize = 16384;
extern uint8_t bios[biosSize];

class Bios::Impl final:
        public Bios
{
public:
    explicit Impl(Media * media, Memory * memory, Cpu * cpu):
        media_m(media), memBanks_m(memory->memBanks()),
        cpu_m(cpu), cpuRegs_m(cpu->cpuRegs())
    {}

    virtual void init() override
    {
        memcpy(memBanks_m->rom.data(), bios, biosSize);
    }

    virtual void reset() override
    {
        casFileReader_m->close();
        isLoadHacked_m = false;
    }

    virtual void close() override
    {
    }

    virtual void initMediaHooks() override
    {
        switch(media_m->playbackFile.fileFmt()) {
            case FileFmt::cas:
                opHook_m = cpu_m->createOpHook(memFunc(this, &Impl::casOpHookFunc));
                casFileReader_m = CasFileReader::create();
                captureLog(casFileReader_m.get());
                break;
            case FileFmt::wav:
                opHook_m = cpu_m->createOpHook(memFunc(this, &Impl::wavOpHookFunc));
                break;
            default:
                opHook_m = nullptr;
                break;
        }
    }

private:
    void casOpHookFunc()
    {
        switch(cpuRegs_m->pc) {
            case 0x0199: // test keyboard buffer
            {
                if(isLoadHacked_m) {
                    break;
                }
                isLoadHacked_m = true;

                std::unique_ptr<CasFileReader> casFileReader = CasFileReader::create();
                casFileReader->open(media_m->playbackFile.fileName());
                std::array<uint8_t, 16> buf;
                casFileReader->read(buf.data(), 16);
                if(!casFileReader->isValid()) {
                    break;
                }

                size_t i = 1;
                while(i < 10 && buf[i++] == buf[0]);
                if(i < 10) {
                    break;
                }

                // TODO: Find direct bios entry points. Although it seems that
                // method used below is the most native and harmless
                switch(buf[0]) {
                    case 0xd0: // load binary program and run
                    {
                        std::string inject = stringf("bload\"%.6s\",r\x0d", buf.data() + 10);
                        memcpy(memBanks_m->ram.data() + 0xfb85, inject.data(), inject.size());
                        memcpy(memBanks_m->ram.data() + 0xfa2a, "\x95\xfb\x85\xfb", 4);
                        break;
                    }
                    case 0xd3: // load BASIC program and run
                    {
                        std::string inject = stringf("cload\"%.6s\":run\x0d", buf.data() + 10);
                        memcpy(memBanks_m->ram.data() + 0xfb85, inject.data(), inject.size());
                        memcpy(memBanks_m->ram.data() + 0xfa2a, "\x97\xfb\x85\xfb", 4);
                        break;
                    }
                }
                break;
            }
            case 0x34ca: // load routine entry
            {
                casFileReader_m->close(); // to reopen file later in wait for initial tone
                break;
            }
            case 0x36d2: // wait for initial tone
            {
                if(!casFileReader_m->isValid()) {
                    casFileReader_m->open(media_m->playbackFile.fileName());
                    if(!casFileReader_m->isValid()) {
                        cpuRegs_m->pc = 0x3772; // "Device I/O error"
                        break;
                    }
                    atEnd_m = false;
                    cpuRegs_m->pc = 0x370d; // initial tone detected
                    break;
                }
                if(atEnd_m) {
                    break; // wait forever
                }
                casFileReader_m->nextBlock();
                if(!casFileReader_m->isValid()) {
                    casFileReader_m->recover();
                    if(!casFileReader_m->isValid()) {
                        cpuRegs_m->pc = 0x3772; // "Device I/O error"
                        break;
                    }
                    LibRetro::popupMsg("End of media reached");
                    atEnd_m = true;
                    break;
                }
                cpuRegs_m->pc = 0x370d; // initial tone detected
                break;
            }
            case 0x370e: // load byte
            {
                casFileReader_m->read(&cpuRegs_m->h, sizeof(cpuRegs_m->h));
                if(!casFileReader_m->isValid()) {
                    cpuRegs_m->pc = 0x3772; // "Device I/O error"
                    break;
                }
                cpuRegs_m->pc = 0x3751; // byte loaded
                break;
            }
        }
    }

    void wavOpHookFunc()
    {
    }

    Media * media_m;
    MemBanks * memBanks_m;
    Cpu * cpu_m;
    CpuRegs * cpuRegs_m;

    std::unique_ptr<OpHook> opHook_m;
    std::unique_ptr<CasFileReader> casFileReader_m;
    bool isLoadHacked_m;
    bool atEnd_m;
};

auto Bios::create(Media * media, Memory * memory, Cpu * cpu) -> std::unique_ptr<Bios>
{
    return std::make_unique<Impl>(media, memory, cpu);
}
