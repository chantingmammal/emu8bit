#pragma once

#include <nesemu/steady_timer.h>
#include <nesemu/utils.h>

#include <cstdint>
#include <type_traits>


// Forward declarations
namespace apu {
class APU;
}

namespace ppu {
class PPU;
}

namespace system_bus {
class SystemBus;
}


// Ricoh RP2A03 (based on MOS6502)
// Little endian
namespace cpu {

enum class Instruction : uint8_t {
  ADC = 0x69,  // ADd with Carry
  AND = 0x29,  // bitwise AND with accumulator
  ASL = 0x0A,  // Arithmetic Shift Left
  BIT = 0x28,  // test BITs

  // Branch Instructions
  BPL = 0x10,  // Branch on PLus
  BMI = 0x30,  // Branch on MInus
  BVC = 0x50,  // Branch on oVerflow Clear
  BVS = 0x70,  // Branch on oVerflow Set
  BCC = 0x90,  // Branch on Carry Clear
  BCS = 0xB0,  // Branch on Carry Set
  BNE = 0xD0,  // Branch on Not Equal
  BEQ = 0xF0,  // Branch on EQual

  BRK = 0x00,  // BReaK
  CMP = 0xC9,  // CoMPare accumulator
  CPX = 0xE0,  // ComPare X register                 ** Alternate opcode offsets
  CPY = 0xC0,  // ComPare Y register                 ** Alternate opcode offsets
  DEC = 0xCA,  // DECrement memory
  EOR = 0x49,  // bitwise Exclusive OR

  // Flag (Processor Status) Instructions
  CLC = 0x18,  // CLear Carry
  SEC = 0x38,  // SEt Carry
  CLI = 0x58,  // CLear Interrupt
  SEI = 0x78,  // SEt Interrupt
  CLV = 0xB8,  // CLear oVerflow
  CLD = 0xD8,  // CLear Decimal
  SED = 0xF8,  // SEt Decimal

  INC = 0xEA,  // INCrement memory
  JMP = 0x4C,  // JuMP
  JSR = 0x1C,  // Jump to SubRoutine
  LDA = 0xA9,  // LoaD Accumulator
  LDX = 0xA2,  // LoaD X register                    ** Alternate opcode offsets
  LDY = 0xA0,  // LoaD Y register                    ** Alternate opcode offsets
  LSR = 0x4A,  // Logical Shift Right
  NOP = 0xEA,  // No OPeration
  ORA = 0x09,  // bitwise OR with Accumulator

  // Register Instructions
  TAX = 0xAA,  // Transfer A to X
  TXA = 0x8A,  // Transfer X to A
  DEX = 0xCA,  // DEcrement X
  INX = 0xE8,  // INcrement X
  TAY = 0xA8,  // Transfer A to Y
  TYA = 0x98,  // Transfer Y to A
  DEY = 0x88,  // DEcrement Y
  INY = 0xC8,  // INcrement Y

  ROL = 0x2A,  // ROtate Left
  ROR = 0x6A,  // ROtate Right
  RTI = 0x40,  // ReTurn from Interrupt
  RTS = 0x60,  // ReTurn from Subroutine
  SBC = 0xE9,  // SuBtract with Carry
  STA = 0x89,  // STore Accumulator

  // Stack Instructions
  TXS = 0x9A,  // Transfer X to Stack ptr
  TSX = 0xBA,  // Transfer Stack ptr to X
  PHA = 0x48,  // PusH Accumulator
  PLA = 0x68,  // PuLl Accumulator
  PHP = 0x08,  // PusH Processor status
  PLP = 0x28,  // PuLl Processor status

  STX = 0x8A,  // STore X register
  STY = 0x88,  // STore Y register
};

enum class AddressingMode : int8_t {
  implied     = 0x00,
  accumulator = 0x00,
  immediate   = 0x00,
  relative    = 0x00,
  zero_page   = -0x04,
  zero_page_x = 0x0C,
  zero_page_y = 0x0D,  // Incorrect val, should be 0x0C
  absolute    = 0x04,
  absolute_x  = 0x14,
  absolute_y  = 0x10,
  indirect    = 0x20,
  indirect_x  = -0x08,
  indirect_y  = 0x08,

  // Alternates, used for CPX, CPY, LDX, LDY
  alt_zero_page   = 0x04,
  alt_zero_page_x = 0x14,
  alt_zero_page_y = 0x14,
  alt_absolute    = 0x0C,
  alt_absolute_x  = 0x1C,
  alt_absolute_y  = 0x1C,
};


class CPU {
public:
  CPU() { timer_.start(); }

  // Setup
  void connectBus(system_bus::SystemBus* bus);
  void connectChips(apu::APU* apu, ppu::PPU* ppu);


  // Execution
  void executeInstruction();
  void reset(bool active);

private:
  // System bus
  system_bus::SystemBus* bus_ = {nullptr};

  // Other chips
  apu::APU*                 apu_ = {nullptr};
  ppu::PPU*                 ppu_ = {nullptr};
  SteadyTimer<22, 39375000> timer_;  //  ~1.79MHz


  // Registers
  uint16_t PC = {0};          // Program counter
  uint8_t  SP = {0};          // Stack pointer, as an offset from 0x0100. Top down (Empty=0xFF).
  uint8_t  A  = {0};          // Accumulator Register
  uint8_t  X  = {0};          // Index Register X
  uint8_t  Y  = {0};          // Index Register Y
  union {                     // Processor Status Register
    uint8_t             raw;  //
    utils::RegBit<0>    c;    //  - Carry
    utils::RegBit<1>    z;    //  - Zero
    utils::RegBit<2>    i;    //  - Interrupt Disable
    utils::RegBit<3>    d;    //  - Decimal Mode (Ignored by the 2A03)
    utils::RegBit<4, 2> b;    //  - B flag, used in push/pop
                              //  - Reserved
    utils::RegBit<6> v;       //  - Overflow
    utils::RegBit<7> n;       //  - Negative
  } P = {0};                  //


  // Interrupt Requests
  bool prev_nmi_  = {false};  // Edge sensitive
  bool irq_reset_ = {false};  // Level sensitive
  bool irq_brk_   = {false};  // Level sensitive


  // Internal operations
  inline uint8_t  readByte(uint16_t address);                 // 1 cycle
  inline void     writeByte(uint16_t address, uint8_t data);  // 1 cycle
  inline void     push(uint8_t data);                         // 1 cycle
  inline uint8_t  pop();                                      // 2 cycles
  inline uint16_t pop16();                                    // 3 cycles
                                                              // NOTE: pop16() is equivalent to
                                                              //       `pop() + (pop() << 8)`, but takes 1 fewer cycle

  inline void     tick(int ticks = 1);
  inline void     interrupt(uint16_t vector_table);  // 5 cycles
  inline void     branch(bool condition);            // 1 cycle
  inline uint16_t getArgAddr(std::underlying_type<AddressingMode>::type mode, bool check_page_boundary = false);
  inline uint16_t getArgAddr(AddressingMode mode, bool check_page_boundary = false);
};

}  // namespace cpu
