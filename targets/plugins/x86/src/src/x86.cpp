#include <x86/x86.hpp>

std::size_t near_displacement_size(X86Mode mode)
{
    switch (mode)
    {
    case X86Mode::Bits16: return 2;
    case X86Mode::Bits32:
    case X86Mode::Bits64: return 4;
    }
    return 0;
}
