#include <nesemu/cpu.h>
#include <nesemu/utils.h>

#include <iostream>


// =*=*=*=*= CPU Setup =*=*=*=*=

void cpu::CPU::loadCart(uint8_t* prg_rom, uint8_t prg_banks, uint8_t* expansion_ram) {
  prg_rom_       = prg_rom;
  prg_banks_     = prg_banks;
  expansion_ram_ = expansion_ram;
}

void cpu::CPU::setPPUReadRegisterPtr(std::function<uint8_t(uint16_t)> func) {
  ppu_read_register_ = func;
}

void cpu::CPU::setPPUWriteRegisterPtr(std::function<void(uint16_t, uint8_t)> func) {
  ppu_write_register_ = func;
}

void cpu::CPU::setPPUSpriteDMAPtr(std::function<void(uint8_t*)> func) {
  ppu_sprite_dma_ = func;
}

void cpu::CPU::setClockTickPtr(std::function<void(void)> func) {
  tick_ = func;
}


// =*=*=*=*= CPU Execution =*=*=*=*=

void cpu::CPU::executeInstruction() {
  using utils::asInt;

  // This is the correct interrupt priority
  if (irq_reset_) {
    interrupt(0xFFFC);
    return;
  } else if (irq_nmi_) {
    irq_nmi_ = false;
    interrupt(0xFFFA);
    return;
  } else if (irq_irq_ && !P.i) {
    interrupt(0xFFFE);
    return;
  }

#if DEBUG
  static uint16_t next_op = PC;
  std::cout << next_op << std::endl;
  if (next_op <= PC) {
    std::cin >> std::hex >> std::noskipws >> next_op;
  }
#endif

  // Fetch next instruction from PC
  const uint8_t opcode = readByte(PC++);

#if DEBUG
  std::cout << "Opcode: " << std::hex << unsigned(opcode) << " from addr " << std::hex << unsigned(PC - 1) << std::endl;
#endif

  switch (opcode) {

    // Add with carry
    case (asInt(Instruction::ADC) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::ADC) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::ADC) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::ADC) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::ADC) + asInt(AddressingMode::absolute_x)):
    case (asInt(Instruction::ADC) + asInt(AddressingMode::absolute_y)):
    case (asInt(Instruction::ADC) + asInt(AddressingMode::indirect_x)):
    case (asInt(Instruction::ADC) + asInt(AddressingMode::indirect_y)): {
      const uint8_t arg    = readByte(getArgAddr(opcode - asInt(Instruction::ADC)));
      const uint8_t result = A + arg + P.c;
      P.c                  = ((A + arg + P.c) >> 8) & 0x01;
      P.z                  = (result == 0);
      P.v                  = (A >> 7 == arg >> 7) && (A >> 7 != result >> 7);
      P.n                  = (result >> 7);
      A                    = result;
    } break;


    // Bitwise AND with accumulator
    case (asInt(Instruction::AND) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::AND) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::AND) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::AND) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::AND) + asInt(AddressingMode::absolute_x)):
    case (asInt(Instruction::AND) + asInt(AddressingMode::absolute_y)):
    case (asInt(Instruction::AND) + asInt(AddressingMode::indirect_x)):
    case (asInt(Instruction::AND) + asInt(AddressingMode::indirect_y)): {
      A &= readByte(getArgAddr(opcode - asInt(Instruction::AND)));
      P.z = (A == 0);
      P.n = (A >> 7);
    } break;


    // Arithmetic shift left
    case (asInt(Instruction::ASL) + asInt(AddressingMode::accumulator)): {
      P.c = (A >> 7);
      A <<= 1;
      P.z = (A == 0);
      P.n = (A >> 7);
    } break;
    case (asInt(Instruction::ASL) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::ASL) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::ASL) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::ASL) + asInt(AddressingMode::absolute_x)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::ASL));
      uint8_t        arg  = readByte(addr);

      P.c = (arg >> 7);
      arg <<= 1;
      P.z = (arg == 0);
      P.n = (arg >> 7);
      writeByte(addr, arg);
    } break;


    // Test bits
    case (asInt(Instruction::BIT) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::BIT) + asInt(AddressingMode::absolute)): {
      const uint8_t arg = readByte(getArgAddr(opcode - asInt(Instruction::BIT)));
      P.z               = ((A & arg) == 0);
      P.v               = (arg >> 6) & 0x01;
      P.n               = (arg >> 7);
    } break;


    // Branch
    case (asInt(Instruction::BPL) + asInt(AddressingMode::relative)):
      branch(!P.n);
      break;
    case (asInt(Instruction::BMI) + asInt(AddressingMode::relative)):
      branch(P.n);
      break;
    case (asInt(Instruction::BVC) + asInt(AddressingMode::relative)):
      branch(!P.v);
      break;
    case (asInt(Instruction::BVS) + asInt(AddressingMode::relative)):
      branch(P.v);
      break;
    case (asInt(Instruction::BCC) + asInt(AddressingMode::relative)):
      branch(!P.c);
      break;
    case (asInt(Instruction::BCS) + asInt(AddressingMode::relative)):
      branch(P.c);
      break;
    case (asInt(Instruction::BNE) + asInt(AddressingMode::relative)):
      branch(!P.z);
      break;
    case (asInt(Instruction::BEQ) + asInt(AddressingMode::relative)):
      branch(P.z);
      break;


    // Break
    case (asInt(Instruction::BRK) + asInt(AddressingMode::implied)):
      IRQ(true);
      break;


    // Compare accumulator
    case (asInt(Instruction::CMP) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::CMP) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::CMP) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::CMP) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::CMP) + asInt(AddressingMode::absolute_x)):
    case (asInt(Instruction::CMP) + asInt(AddressingMode::absolute_y)):
    case (asInt(Instruction::CMP) + asInt(AddressingMode::indirect_x)):
    case (asInt(Instruction::CMP) + asInt(AddressingMode::indirect_y)): {
      const uint8_t arg = readByte(getArgAddr(opcode - asInt(Instruction::CMP)));
      P.z               = (A == arg);
      P.c               = (A >= arg);
      P.n               = ((A - arg) >> 7);
    } break;


    // Compare X register
    case (asInt(Instruction::CPX) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::CPX) + asInt(AddressingMode::alt_zero_page)):
    case (asInt(Instruction::CPX) + asInt(AddressingMode::alt_absolute)): {
      AddressingMode mode = AddressingMode::immediate;
      if (opcode == asInt(Instruction::CPX) + asInt(AddressingMode::alt_zero_page)) {
        mode = AddressingMode::zero_page;
      } else if (opcode == asInt(Instruction::CPX) + asInt(AddressingMode::alt_absolute)) {
        mode = AddressingMode::absolute;
      }

      const uint8_t arg = readByte(getArgAddr(mode));
      P.z               = (X == arg);
      P.c               = (X >= arg);
      P.n               = ((X - arg) >> 7);
    } break;


    // Compare Y register
    case (asInt(Instruction::CPY) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::CPY) + asInt(AddressingMode::alt_zero_page)):
    case (asInt(Instruction::CPY) + asInt(AddressingMode::alt_absolute)): {
      AddressingMode mode = AddressingMode::immediate;
      if (opcode == asInt(Instruction::CPY) + asInt(AddressingMode::alt_zero_page)) {
        mode = AddressingMode::zero_page;
      } else if (opcode == asInt(Instruction::CPY) + asInt(AddressingMode::alt_absolute)) {
        mode = AddressingMode::absolute;
      }

      const uint8_t arg = readByte(getArgAddr(mode));
      P.z               = (Y == arg);
      P.c               = (Y >= arg);
      P.n               = ((Y - arg) >> 7);
    } break;


    // Decrement memory
    case (asInt(Instruction::DEC) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::DEC) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::DEC) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::DEC) + asInt(AddressingMode::absolute_x)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::DEC));
      const uint8_t  arg  = readByte(addr) - 1;
      P.z                 = (arg == 0);
      P.n                 = (arg >> 7);
      writeByte(addr, arg);
    } break;


    // Bitwise exclusive OR
    case (asInt(Instruction::EOR) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::EOR) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::EOR) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::EOR) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::EOR) + asInt(AddressingMode::absolute_x)):
    case (asInt(Instruction::EOR) + asInt(AddressingMode::absolute_y)):
    case (asInt(Instruction::EOR) + asInt(AddressingMode::indirect_x)):
    case (asInt(Instruction::EOR) + asInt(AddressingMode::indirect_y)): {
      A ^= readByte(getArgAddr(opcode - asInt(Instruction::EOR)));
      P.z = (A == 0);
      P.n = (A >> 7);
    } break;


    // Flag (Processor Status) manipulation
    case (asInt(Instruction::CLC) + asInt(AddressingMode::implied)):
      P.c = 0;
      break;
    case (asInt(Instruction::SEC) + asInt(AddressingMode::implied)):
      P.c = 1;
      break;
    case (asInt(Instruction::CLI) + asInt(AddressingMode::implied)):
      P.i = 0;
      break;
    case (asInt(Instruction::SEI) + asInt(AddressingMode::implied)):
      P.i = 1;
      break;
    case (asInt(Instruction::CLV) + asInt(AddressingMode::implied)):
      P.v = 0;
      break;
    case (asInt(Instruction::CLD) + asInt(AddressingMode::implied)):
      P.d = 0;
      break;
    case (asInt(Instruction::SED) + asInt(AddressingMode::implied)):
      P.d = 1;
      break;


    // Increment memory
    case (asInt(Instruction::INC) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::INC) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::INC) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::INC) + asInt(AddressingMode::absolute_x)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::INC));
      const uint8_t  arg  = readByte(addr) + 1;
      P.z                 = (arg == 0);
      P.n                 = (arg >> 7);
      writeByte(addr, arg);
    } break;


    // Jump
    case (asInt(Instruction::JMP) + asInt(AddressingMode::immediate)):  // Irregular opcode, actually absolute
      PC = getArgAddr(AddressingMode::absolute);
      break;
    case (asInt(Instruction::JMP) + asInt(AddressingMode::indirect)):
      PC = getArgAddr(AddressingMode::indirect);
      break;


    // Jump to service routine
    case (asInt(Instruction::JSR) + asInt(AddressingMode::absolute)): {
      const uint16_t address = getArgAddr(AddressingMode::absolute);
      push((PC - 1) >> 8);
      push(PC - 1);
      PC = address;
    } break;


    // Load accumulator
    case (asInt(Instruction::LDA) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::LDA) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::LDA) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::LDA) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::LDA) + asInt(AddressingMode::absolute_x)):
    case (asInt(Instruction::LDA) + asInt(AddressingMode::absolute_y)):
    case (asInt(Instruction::LDA) + asInt(AddressingMode::indirect_x)):
    case (asInt(Instruction::LDA) + asInt(AddressingMode::indirect_y)): {
      A   = readByte(getArgAddr(opcode - asInt(Instruction::LDA)));
      P.z = (A == 0);
      P.n = (A >> 7);
    } break;


    // Load X register
    case (asInt(Instruction::LDX) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::LDX) + asInt(AddressingMode::alt_zero_page)):
    case (asInt(Instruction::LDX) + asInt(AddressingMode::alt_zero_page_y)):
    case (asInt(Instruction::LDX) + asInt(AddressingMode::alt_absolute)):
    case (asInt(Instruction::LDX) + asInt(AddressingMode::alt_absolute_y)): {
      AddressingMode mode = AddressingMode::immediate;
      if (opcode == asInt(Instruction::LDX) + asInt(AddressingMode::alt_zero_page)) {
        mode = AddressingMode::zero_page;
      } else if (opcode == asInt(Instruction::LDX) + asInt(AddressingMode::alt_zero_page_y)) {
        mode = AddressingMode::zero_page_y;
      } else if (opcode == asInt(Instruction::LDX) + asInt(AddressingMode::alt_absolute)) {
        mode = AddressingMode::absolute;
      } else if (opcode == asInt(Instruction::LDX) + asInt(AddressingMode::alt_absolute_y)) {
        mode = AddressingMode::absolute_y;
      }

      X   = readByte(getArgAddr(mode));
      P.z = (X == 0);
      P.n = (X >> 7);
    } break;


    // Load Y register
    case (asInt(Instruction::LDY) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::LDY) + asInt(AddressingMode::alt_zero_page)):
    case (asInt(Instruction::LDY) + asInt(AddressingMode::alt_zero_page_x)):
    case (asInt(Instruction::LDY) + asInt(AddressingMode::alt_absolute)):
    case (asInt(Instruction::LDY) + asInt(AddressingMode::alt_absolute_x)): {
      AddressingMode mode = AddressingMode::immediate;
      if (opcode == asInt(Instruction::LDY) + asInt(AddressingMode::alt_zero_page)) {
        mode = AddressingMode::zero_page;
      } else if (opcode == asInt(Instruction::LDY) + asInt(AddressingMode::alt_zero_page_x)) {
        mode = AddressingMode::zero_page_x;
      } else if (opcode == asInt(Instruction::LDY) + asInt(AddressingMode::alt_absolute)) {
        mode = AddressingMode::absolute;
      } else if (opcode == asInt(Instruction::LDY) + asInt(AddressingMode::alt_absolute_x)) {
        mode = AddressingMode::absolute_x;
      }

      Y   = readByte(getArgAddr(mode));
      P.z = (Y == 0);
      P.n = (Y >> 7);
    } break;


    // Logical shift right
    case (asInt(Instruction::LSR) + asInt(AddressingMode::accumulator)): {
      P.c = (A & 0x01);
      A >>= 1;
      P.z = (A == 0);
      P.n = (A >> 7);  // Always 0
    } break;
    case (asInt(Instruction::LSR) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::LSR) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::LSR) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::LSR) + asInt(AddressingMode::absolute_x)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::LSR));
      uint8_t        arg  = readByte(addr);

      P.c = (arg & 0x01);
      arg >>= 1;
      P.z = (arg == 0);
      P.n = (arg >> 7);  // Always 0
      writeByte(addr, arg);
    } break;


    // No operation
    case (asInt(Instruction::NOP) + asInt(AddressingMode::implied)):
      tick_();
      break;


    // Logical inclusive OR
    case (asInt(Instruction::ORA) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::ORA) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::ORA) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::ORA) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::ORA) + asInt(AddressingMode::absolute_x)):
    case (asInt(Instruction::ORA) + asInt(AddressingMode::absolute_y)):
    case (asInt(Instruction::ORA) + asInt(AddressingMode::indirect_x)):
    case (asInt(Instruction::ORA) + asInt(AddressingMode::indirect_y)): {
      A |= readByte(getArgAddr(opcode - asInt(Instruction::ORA)));
      P.z = (A == 0);
      P.n = (A >> 7);
    } break;


    // Register manipulation
    case (asInt(Instruction::TAX) + asInt(AddressingMode::implied)): {
      X   = A;
      P.z = (X == 0);
      P.n = (X >> 7);
    } break;
    case (asInt(Instruction::TXA) + asInt(AddressingMode::implied)): {
      A   = X;
      P.z = (A == 0);
      P.n = (A >> 7);
    } break;
    case (asInt(Instruction::DEX) + asInt(AddressingMode::implied)): {
      X--;
      P.z = (X == 0);
      P.n = (X >> 7);
    } break;
    case (asInt(Instruction::INX) + asInt(AddressingMode::implied)): {
      X++;
      P.z = (X == 0);
      P.n = (X >> 7);
    } break;
    case (asInt(Instruction::TAY) + asInt(AddressingMode::implied)): {
      Y   = A;
      P.z = (Y == 0);
      P.n = (Y >> 7);
    } break;
    case (asInt(Instruction::TYA) + asInt(AddressingMode::implied)): {
      A   = Y;
      P.z = (A == 0);
      P.n = (A >> 7);
    } break;
    case (asInt(Instruction::DEY) + asInt(AddressingMode::implied)): {
      Y--;
      P.z = (Y == 0);
      P.n = (Y >> 7);
    } break;
    case (asInt(Instruction::INY) + asInt(AddressingMode::implied)): {
      Y++;
      P.z = (Y == 0);
      P.n = (Y >> 7);
    } break;


    // Rotate left
    case (asInt(Instruction::ROL) + asInt(AddressingMode::accumulator)): {
      const bool old_carry = (A >> 7);
      A                    = (A << 1) | P.c;
      P.c                  = old_carry;
      P.z                  = (A == 0);
      P.n                  = (A >> 7);
    } break;
    case (asInt(Instruction::ROL) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::ROL) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::ROL) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::ROL) + asInt(AddressingMode::absolute_x)): {
      const uint16_t addr      = getArgAddr(opcode - asInt(Instruction::ROL));
      uint8_t        arg       = readByte(addr);
      const bool     old_carry = (arg >> 7);
      arg                      = (arg << 1) | P.c;
      P.c                      = old_carry;
      P.z                      = (arg == 0);
      P.n                      = (arg >> 7);
      writeByte(addr, arg);
    } break;


    // Rotate right
    case (asInt(Instruction::ROR) + asInt(AddressingMode::accumulator)): {
      const bool old_carry = (A & 0x01);
      A                    = (A >> 1) | (P.c << 7);
      P.c                  = old_carry;
      P.z                  = (A == 0);
      P.n                  = (A >> 7);
    } break;
    case (asInt(Instruction::ROR) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::ROR) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::ROR) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::ROR) + asInt(AddressingMode::absolute_x)): {
      const uint16_t addr      = getArgAddr(opcode - asInt(Instruction::ROR));
      uint8_t        arg       = readByte(addr);
      const bool     old_carry = (arg & 0x01);
      arg                      = (arg >> 1) | (P.c << 7);
      P.c                      = old_carry;
      P.z                      = (arg == 0);
      P.n                      = (arg >> 7);
      writeByte(addr, arg);
    } break;


    // Return from interrupt
    case (asInt(Instruction::RTI) + asInt(AddressingMode::implied)): {
      P.raw = pop();
      PC    = pop() + (pop() << 8);
    } break;


    // Return from subroutine
    case (asInt(Instruction::RTS) + asInt(AddressingMode::implied)): {
      PC = pop() + (pop() << 8) + 1;
    } break;


    // Subtract with carry
    case (asInt(Instruction::SBC) + asInt(AddressingMode::immediate)):
    case (asInt(Instruction::SBC) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::SBC) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::SBC) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::SBC) + asInt(AddressingMode::absolute_x)):
    case (asInt(Instruction::SBC) + asInt(AddressingMode::absolute_y)):
    case (asInt(Instruction::SBC) + asInt(AddressingMode::indirect_x)):
    case (asInt(Instruction::SBC) + asInt(AddressingMode::indirect_y)): {
      const uint8_t  arg      = readByte(getArgAddr(opcode - asInt(Instruction::SBC)));
      const uint16_t res_long = A + (~arg + 1) + (~P.c + 1);
      const uint8_t  result   = res_long;
      P.c                     = !((res_long >> 8) & 0x01);
      P.z                     = (result == 0);
      P.v                     = (A >> 7 == arg >> 7) && (A >> 7 != result >> 7);
      P.n                     = (result >> 7);
      A                       = result;
    } break;


    // Store accumulator
    case (asInt(Instruction::STA) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::absolute_x)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::absolute_y)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::indirect_x)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::indirect_y)): {
      writeByte(getArgAddr(opcode - asInt(Instruction::STA)), A);
    } break;


    // Stack manipulation
    case (asInt(Instruction::TXS) + asInt(AddressingMode::implied)):
      SP = X;
      break;
    case (asInt(Instruction::TSX) + asInt(AddressingMode::implied)):
      X = SP;
      break;
    case (asInt(Instruction::PHA) + asInt(AddressingMode::implied)):
      push(A);
      break;
    case (asInt(Instruction::PLA) + asInt(AddressingMode::implied)):
      A = pop();
      break;
    case (asInt(Instruction::PHP) + asInt(AddressingMode::implied)):
      push(P.raw);
      break;
    case (asInt(Instruction::PLP) + asInt(AddressingMode::implied)):
      P.raw = pop();
      break;


    // Store X register
    case (asInt(Instruction::STX) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::STX) + asInt(AddressingMode::zero_page_y)):
    case (asInt(Instruction::STX) + asInt(AddressingMode::absolute)): {
      writeByte(getArgAddr(opcode - asInt(Instruction::STX)), X);
    } break;


    // Store Y register
    case (asInt(Instruction::STY) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::STY) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::STY) + asInt(AddressingMode::absolute)): {
      writeByte(getArgAddr(opcode - asInt(Instruction::STY)), Y);
    } break;


    // Illegal instruction
    default:
      std::cout << "Illegal instruction " << unsigned(opcode) << std::endl;
      exit(1);
      break;
  }

#if DEBUG
  if (std::cin.fail()) {
    std::cin.clear();
    std::cin.ignore();
    next_op = PC;
  }
#endif
}

void cpu::CPU::NMI() {
  irq_nmi_ = true;
}

void cpu::CPU::reset(bool active) {
  irq_reset_ = active;
}

void cpu::CPU::IRQ(bool active) {
  irq_irq_ = active;
}


// =*=*=*=*= CPU Internal Operations =*=*=*=*=

uint8_t cpu::CPU::readByte(uint16_t address) {
#if DEBUG
  const uint8_t data = readByteInternal(address);
  std::cout << "Read " << unsigned(data) << " from $(" << address << ")\n";
  return data;
}

uint8_t cpu::CPU::readByteInternal(uint16_t address) {
#endif
  tick_();
  address &= 0xFFFF;

  if (address < 0x2000)  // Stack and RAM
    return ram_[address & 0x07FF];

  else if (address < 0x4000)  // PPU Registers
    return ppu_read_register_((address & 0x0007) | 0x2000);

  else if (address < 0x4014)  // Sound Registers
    return 0;                 // TODO

  else if (address == 0x4014)  // PPU DMA Access
    return 0;                  // Cannot read DMA register

  else if (address == 0x4015)  // Sound Channel Switch
    return 0;                  // Cannot read sound channel register

  else if (address == 0x4016)  // Joystick 1
    return 0;                  // TODO

  else if (address == 0x4017)  // Joystick 2
    return 0;                  // TODO

  else if (address == 0x4020)  // Unused
    return 0;

  else if (address < 0x6000)  // Expansion Modules, ie. Famicom Disk System
    return 0;

  else if (address < 0x8000 && expansion_ram_)  // Cartridge RAM
    return expansion_ram_[address - 0x6000];

  else  // Cartridge ROM
    return prg_rom_[(address - 0x8000) & (prg_banks_ == 2 ? 0x7FFF : 0x3FFF)];
}

void cpu::CPU::writeByte(uint16_t address, uint8_t data) {
#if DEBUG
  std::cout << "Write " << unsigned(data) << " to $(" << address << ")\n";
#endif
  tick_();
  address &= 0xFFFF;

  if (address < 0x2000) {  // Stack and RAM
    ram_[address & 0x07FF] = data;
  }

  else if (address < 0x4000) {  // PPU Registers
    ppu_write_register_((address & 0x0007) | 0x2000, data);
  }

  else if (address < 0x4014) {  // Sound Registers
    ;                           // TODO
  }

  else if (address == 0x4014) {  // PPU DMA Access
    if (data * 0x100 < 0x2000) {
      ppu_sprite_dma_(ram_ + (data * 0x100));  // Internal RAM
    } else if (data * 0x100 < 0x6000) {
      ;  // Cannot DMA from MMIO
    } else if (data * 0x100 < 0x8000) {
      if (expansion_ram_) {
        ppu_sprite_dma_(expansion_ram_ + (data * 0x100) - 0x6000);
      } else {
        ;  // No cartridge RAM
      }
    } else {
      ppu_sprite_dma_(prg_rom_ + (data * 0x100) - 0x8000);  // Cartridge ROM
    }
  }

  else if (address == 0x4015) {  // Sound Channel Switch
    ;                            // TODO
  }

  else if (address == 0x4016) {  // Joystick 1
    ;                            // TODO
  }

  else if (address == 0x4017) {  // Joystick 2
    ;                            // TODO
  }

  else if (address == 0x4020) {  // Unused
    ;
  }

  else if (address < 0x6000) {  // Expansion Modules, ie. Famicom Disk System
    ;
  }

  else if (address < 0x8000) {  // Cartridge RAM
    if (expansion_ram_) {
      expansion_ram_[address - 0x6000] = data;
    } else {
      ;  // No cartridge RAM
    }
  }

  else {  // Cartridge ROM
    prg_rom_[(address - 0x8000) & (prg_banks_ == 2 ? 0x7FFF : 0x3FFF)] = data;
  }
}

void cpu::CPU::push(uint8_t data) {
#if DEBUG
  std::cout << "Push " << unsigned(data) << " @ " << 0x0100 + SP << std::endl;
#endif
  writeByte(0x0100 + SP--, data);
}

uint8_t cpu::CPU::pop() {
#if DEBUG
  uint8_t data = readByte(0x0100 + (++SP));
  std::cout << "Pop  " << unsigned(data) << " @ " << 0x0100 + SP << std::endl;
  return data;
#else
  return readByte(0x0100 + (++SP));
#endif
}

void cpu::CPU::interrupt(uint16_t vector_table) {
#if DEBUG
  std::cout << "Interrupt, jumping to $(" << vector_table << ")\n";
#endif
  push(PC >> 8);
  push(PC);
  push(P.raw);

  P.i = true;
  PC  = readByte(vector_table) | (readByte(vector_table + 1) << 8);
}

void cpu::CPU::branch(bool condition) {
  const int8_t offset = utils::deComplement(readByte(getArgAddr(AddressingMode::relative)));
  if (condition) {
    PC += offset;
    tick_();
  }
}

uint16_t cpu::CPU::getArgAddr(std::underlying_type<AddressingMode>::type mode) {
  return getArgAddr(utils::asEnum<AddressingMode>(mode));
}

uint16_t cpu::CPU::getArgAddr(AddressingMode mode) {
  switch (mode) {
    case (AddressingMode::immediate):  // Or relative
      return PC++;

    case (AddressingMode::zero_page):
      return readByte(PC++);

    case (AddressingMode::zero_page_x):
      return (readByte(PC++) + X) & 0xFF;

    case (AddressingMode::zero_page_y):
      return (readByte(PC++) + Y) & 0xFF;

    case (AddressingMode::absolute):
      return readByte(PC++) + (readByte(PC++) << 8);

    case (AddressingMode::absolute_x):
      return readByte(PC++) + (readByte(PC++) << 8) + X;

    case (AddressingMode::absolute_y):
      return readByte(PC++) + (readByte(PC++) << 8) + Y;

    case (AddressingMode::indirect): {
      const uint16_t addr = readByte(PC++) + (readByte(PC++) << 8);
      return readByte(addr) + (readByte(addr + 1) << 8);
    }

    case (AddressingMode::indirect_x): {
      const uint8_t addr = readByte(PC++) + X;
      return readByte(addr) + (readByte((addr + 1) & 0xFF) << 8);
    }

    case (AddressingMode::indirect_y): {
      const uint8_t addr = readByte(PC++);
      return readByte(addr) + (readByte((addr + 1) & 0xFF) << 8) + Y;
    }

    default:
      std::cerr << "Unknown addressing mode" << std::endl;
      exit(1);
  };
}
