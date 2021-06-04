#include "breakpt.h"
#include "dump.h"
#include "machine.h"

class BreakPt final: public HookHandler<BreakPt, MemHook>
{
public:
    void handlerProc(MemAccessType access, MemBankType bank, uint16_t addr) {
        if(access == MemAccessType::read &&
                bank == MemBankType::ram && addr == 0x4000)
            dump();
        if(access == MemAccessType::read &&
                bank == MemBankType::ram && addr >= 0xfb85 && addr < 0xfb95)
            dump();
    }
};

    //--//

std::unique_ptr<BreakPt> breakPt;

    //--//

void initBreakPt()
{
    breakPt = std::make_unique<BreakPt>();
}
