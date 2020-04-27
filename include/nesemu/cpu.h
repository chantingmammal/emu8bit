#pragma once

#include <nesemu/utils.h>

#include <cstdint>
#include <functional>
#include <type_traits>


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
  zero_page_y = 0x0D,  // Dummy
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
  // Setup
  void loadCart(uint8_t* prg_rom, uint8_t prg_banks, uint8_t* expansion_ram);
  void setPPUReadRegisterPtr(std::function<uint8_t(uint16_t)> func);
  void setPPUWriteRegisterPtr(std::function<void(uint16_t, uint8_t)> func);
  void setPPUSpriteDMAPtr(std::function<void(uint8_t*)> func);
  void setJoyPollPtr(std::function<void(uint8_t)> func);
  void setJoy1ReadPtr(std::function<uint8_t(void)> func);
  void setJoy2ReadPtr(std::function<uint8_t(void)> func);
  void setClockTickPtr(std::function<void(void)> func);


  // Execution
  void executeInstruction();
  void NMI();
  void reset(bool active);
  void IRQ(bool active);


private:
  // Registers
  uint16_t PC = {0};       // Program counter
  uint8_t  SP = {0};       // Stack pointer, as an offset from 0x0100. Top down (Empty=0xFF).
  uint8_t  A  = {0};       // Accumulator Register
  uint8_t  X  = {0};       // Index Register X
  uint8_t  Y  = {0};       // Index Register Y
  union {                  // Processor Status Register
    uint8_t          raw;  //
    utils::RegBit<0> c;    //  - Carry
    utils::RegBit<1> z;    //  - Zero
    utils::RegBit<2> i;    //  - Interrupt Disable
    utils::RegBit<3> d;    //  - Decimal Mode (Ignored by the 2A03)
    utils::RegBit<4> b;    //  - Break
                           //  - Reserved
    utils::RegBit<6> v;    //  - Overflow
    utils::RegBit<7> n;    //  - Negative
  } P = {0};               //


  // Interrupt Requests
  bool irq_nmi_   = {false};
  bool irq_reset_ = {false};
  bool irq_irq_   = {false};


  // Memory
  uint8_t  ram_[0x800]    = {0};        // 2KiB RAM, mirrored 4 times, at address 0x0000-0x1FFF
  uint8_t* prg_rom_       = {nullptr};  // 16/32KiB program ROM,       at address 0x8000-0xFFFF
  uint8_t  prg_banks_     = {0};        // Number of prg_rom banks (1 or 2)
  uint8_t* expansion_ram_ = {nullptr};  // Optional cartridge RAM,     at address 0x7000-0x7FFF


  // Memory-mapped IO (0x2000-0x4000, mirrored, and 0x4000-0x4020)
  std::function<uint8_t(uint16_t)>       ppu_read_register_;
  std::function<void(uint16_t, uint8_t)> ppu_write_register_;
  std::function<void(uint8_t*)>          ppu_sprite_dma_;
  std::function<void(uint8_t)>           joystick_poll_;
  std::function<uint8_t(void)>           joystick_1_read_;
  std::function<uint8_t(void)>           joystick_2_read_;


// Internal operations
#if DEBUG
  uint8_t readByteInternal(uint16_t address);
#endif
  uint8_t  readByte(uint16_t address);                 // 1 cycle
  void     writeByte(uint16_t address, uint8_t data);  // 1 cycle
  void     push(uint8_t data);                         // 1 cycle
  uint8_t  pop();                                      // 2 cycles
  uint16_t pop16();                                    // 3 cycles
                                                       //  - Equivalent to pop() + (pop() << 8), but takes 1 fewer cycle

  inline void     tick(int ticks = 1);
  inline void     interrupt(uint16_t vector_table);  // 5 cycles
  inline void     branch(bool condition);            // 1 cycle
  inline uint16_t getArgAddr(std::underlying_type<AddressingMode>::type mode, bool check_page_boundary = false);
  inline uint16_t getArgAddr(AddressingMode mode, bool check_page_boundary = false);


  // Hacky clock
  std::function<void(void)> tick_;
};

}  // namespace cpu
