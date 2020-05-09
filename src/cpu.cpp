#include <nesemu/cpu.h>

#include <nesemu/apu.h>
#include <nesemu/logger.h>
#include <nesemu/ppu.h>
#include <nesemu/system_bus.h>
#include <nesemu/utils.h>

#include <iostream>


// =*=*=*=*= Logging =*=*=*=*=

template <typename... T>
inline void log(uint16_t addr, uint8_t opcode, const char* format, T... args) {
  size_t size   = snprintf(nullptr, 0, "$%04X> $%02X %s", addr, opcode, format) + 1;
  char*  buffer = new char[size];
  snprintf(buffer, size, "$%04X> $%02X %s", addr, opcode, format);
  logger::log<logger::DEBUG_CPU>(buffer, args...);
  delete buffer;
}


// =*=*=*=*= CPU Setup =*=*=*=*=

void cpu::CPU::connectBus(system_bus::SystemBus* bus) {
  bus_ = bus;
}

void cpu::CPU::connectChips(apu::APU* apu, ppu::PPU* ppu) {
  apu_ = apu;
  ppu_ = ppu;
}


// =*=*=*=*= CPU Execution =*=*=*=*=

void cpu::CPU::executeInstruction() {
  using utils::asInt;

  // This is the correct interrupt priority
  if (irq_reset_) {
    interrupt(0xFFFC);
    return;
  } else if (!prev_nmi_ && bus_->hasNMI()) {
    prev_nmi_ = true;
    P.b |= 0b10;
    interrupt(0xFFFA);
    return;
  } else if (irq_brk_ || (bus_->hasIRQ() && !P.i)) {
    P.b |= 0b10;
    interrupt(0xFFFE);
    return;
  }

  prev_nmi_ = bus_->hasNMI();

#if DEBUG
  static uint16_t next_op = PC;
  std::cout << next_op << std::endl;
  if (next_op <= PC) {
    std::cin >> std::hex >> std::noskipws >> next_op;
  }
#endif

  // Fetch next instruction from PC
  const uint16_t opcode_addr = PC++;
  const uint8_t  opcode      = readByte(opcode_addr);

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
      const uint8_t arg    = readByte(getArgAddr(opcode - asInt(Instruction::ADC), true));
      const uint8_t result = A + arg + P.c;
      P.c                  = ((A + arg + P.c) >> 8) & 0x01;
      P.z                  = (result == 0);
      P.v                  = (A >> 7 == arg >> 7) && (A >> 7 != result >> 7);
      P.n                  = (result >> 7);
      A                    = result;
      log(opcode_addr, opcode, "ADC:       A <- $%02X\n", A);
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
      log(opcode_addr, opcode, "AND:       A <- $%02X\n", A);
    } break;


    // Arithmetic shift left
    case (asInt(Instruction::ASL) + asInt(AddressingMode::accumulator)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      P.c = (A >> 7);
      A <<= 1;
      P.z = (A == 0);
      P.n = (A >> 7);
      log(opcode_addr, opcode, "ASL:       A <- $%02X\n", A);
    } break;
    case (asInt(Instruction::ASL) + asInt(AddressingMode::absolute_x)):
      tick();
      __attribute__((fallthrough));
    case (asInt(Instruction::ASL) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::ASL) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::ASL) + asInt(AddressingMode::absolute)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::ASL));
      uint8_t        arg  = readByte(addr);
      writeByte(addr, arg);  // Double write (For absolute addressing)

      P.c = (arg >> 7);
      arg <<= 1;
      P.z = (arg == 0);
      P.n = (arg >> 7);
      writeByte(addr, arg);
      log(opcode_addr, opcode, "ASL:   $%04X <- $%02X\n", addr, arg);
    } break;


    // Test bits
    case (asInt(Instruction::BIT) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::BIT) + asInt(AddressingMode::absolute)): {
      const uint8_t arg = readByte(getArgAddr(opcode - asInt(Instruction::BIT)));
      P.z               = ((A & arg) == 0);
      P.v               = (arg >> 6) & 0x01;
      P.n               = (arg >> 7);
      log(opcode_addr, opcode, "BIT:     $%02X\n", arg);
    } break;


    // Branch
    case (asInt(Instruction::BPL) + asInt(AddressingMode::relative)):
      branch(!P.n);
      log(opcode_addr, opcode, "BPL\n");
      break;
    case (asInt(Instruction::BMI) + asInt(AddressingMode::relative)):
      branch(P.n);
      log(opcode_addr, opcode, "BMI\n");
      break;
    case (asInt(Instruction::BVC) + asInt(AddressingMode::relative)):
      branch(!P.v);
      log(opcode_addr, opcode, "BVC\n");
      break;
    case (asInt(Instruction::BVS) + asInt(AddressingMode::relative)):
      branch(P.v);
      log(opcode_addr, opcode, "BVS\n");
      break;
    case (asInt(Instruction::BCC) + asInt(AddressingMode::relative)):
      branch(!P.c);
      log(opcode_addr, opcode, "BCC\n");
      break;
    case (asInt(Instruction::BCS) + asInt(AddressingMode::relative)):
      branch(P.c);
      log(opcode_addr, opcode, "BCS\n");
      break;
    case (asInt(Instruction::BNE) + asInt(AddressingMode::relative)):
      branch(!P.z);
      log(opcode_addr, opcode, "BNE\n");
      break;
    case (asInt(Instruction::BEQ) + asInt(AddressingMode::relative)):
      branch(P.z);
      log(opcode_addr, opcode, "BEQ\n");
      break;


    // Break
    case (asInt(Instruction::BRK) + asInt(AddressingMode::implied)):
      readByte(PC);  // Dummy read (For one-byte opcodes)
      irq_brk_ = true;
      P.b      = 0b11;
      log(opcode_addr, opcode, "BRK\n");
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
      tick();
      const uint8_t arg = readByte(getArgAddr(opcode - asInt(Instruction::CMP), true));
      P.z               = (A == arg);
      P.c               = (A >= arg);
      P.n               = ((A - arg) >> 7);
      log(opcode_addr, opcode, "CMP:     $%02X\n", arg);
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

      tick();
      const uint8_t arg = readByte(getArgAddr(mode));
      P.z               = (X == arg);
      P.c               = (X >= arg);
      P.n               = ((X - arg) >> 7);
      log(opcode_addr, opcode, "CPX:     $%02X\n", arg);
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

      tick();
      const uint8_t arg = readByte(getArgAddr(mode));
      P.z               = (Y == arg);
      P.c               = (Y >= arg);
      P.n               = ((Y - arg) >> 7);
      log(opcode_addr, opcode, "CPY:     $%02X\n", arg);
    } break;


    // Decrement memory
    case (asInt(Instruction::DEC) + asInt(AddressingMode::absolute_x)):
      tick();
      __attribute__((fallthrough));
    case (asInt(Instruction::DEC) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::DEC) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::DEC) + asInt(AddressingMode::absolute)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::DEC));
      uint8_t        arg  = readByte(addr);
      writeByte(addr, arg);  // Double write (For absolute addressing)

      arg -= 1;
      P.z = (arg == 0);
      P.n = (arg >> 7);
      writeByte(addr, arg);
      log(opcode_addr, opcode, "DEC:   $%04X <- $%02X\n", addr, arg);
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
      tick();
      A ^= readByte(getArgAddr(opcode - asInt(Instruction::EOR), true));
      P.z = (A == 0);
      P.n = (A >> 7);
      log(opcode_addr, opcode, "EOR:       A <- $%02X\n", A);
    } break;


    // Flag (Processor Status) manipulation
    case (asInt(Instruction::CLC) + asInt(AddressingMode::implied)):
      readByte(PC);  // Dummy read (For one-byte opcodes)
      P.c = false;
      log(opcode_addr, opcode, "CLC\n");
      break;
    case (asInt(Instruction::SEC) + asInt(AddressingMode::implied)):
      readByte(PC);  // Dummy read (For one-byte opcodes)
      P.c = true;
      log(opcode_addr, opcode, "SEC\n");
      break;
    case (asInt(Instruction::CLI) + asInt(AddressingMode::implied)):
      readByte(PC);  // Dummy read (For one-byte opcodes)
      P.i = false;
      log(opcode_addr, opcode, "CLI\n");
      break;
    case (asInt(Instruction::SEI) + asInt(AddressingMode::implied)):
      readByte(PC);  // Dummy read (For one-byte opcodes)
      P.i = true;
      log(opcode_addr, opcode, "SEI\n");
      break;
    case (asInt(Instruction::CLV) + asInt(AddressingMode::implied)):
      readByte(PC);  // Dummy read (For one-byte opcodes)
      P.v = false;
      log(opcode_addr, opcode, "CLV\n");
      break;
    case (asInt(Instruction::CLD) + asInt(AddressingMode::implied)):
      readByte(PC);  // Dummy read (For one-byte opcodes)
      P.d = false;
      log(opcode_addr, opcode, "CLD\n");
      break;
    case (asInt(Instruction::SED) + asInt(AddressingMode::implied)):
      readByte(PC);  // Dummy read (For one-byte opcodes)
      P.d = true;
      log(opcode_addr, opcode, "SED\n");
      break;


    // Increment memory
    case (asInt(Instruction::INC) + asInt(AddressingMode::absolute_x)):
      tick();
      __attribute__((fallthrough));
    case (asInt(Instruction::INC) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::INC) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::INC) + asInt(AddressingMode::absolute)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::INC));
      uint8_t        arg  = readByte(addr);
      writeByte(addr, arg);  // Double write (For absolute addressing)

      arg += 1;
      P.z = (arg == 0);
      P.n = (arg >> 7);
      writeByte(addr, arg);
      log(opcode_addr, opcode, "INC:   $%04X <- $%02X\n", addr, arg);
    } break;


    // Jump
    case (asInt(Instruction::JMP) + asInt(AddressingMode::immediate)):  // Irregular opcode, actually absolute
      PC = getArgAddr(AddressingMode::absolute);
      log(opcode_addr, opcode, "JMP:   $%04X\n", PC);
      break;
    case (asInt(Instruction::JMP) + asInt(AddressingMode::indirect)): {  // Like indirect, but with page wrap bug
      uint16_t addr = readByte(PC++);
      addr |= (readByte(PC++) << 8);
      const uint16_t page = addr & 0xFF00;
      PC                  = readByte(addr) | (readByte(((addr + 1) & 0x00FF) | page) << 8);
      log(opcode_addr, opcode, "JMP:   $%04X\n", PC);
    } break;


    // Jump to service routine
    case (asInt(Instruction::JSR) + asInt(AddressingMode::absolute)): {
      const uint16_t address = getArgAddr(AddressingMode::absolute);
      tick();
      push((PC - 1) >> 8);
      push(PC - 1);
      PC = address;
      log(opcode_addr, opcode, "JSR:   $%04X\n", PC);
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
      A   = readByte(getArgAddr(opcode - asInt(Instruction::LDA), true));
      P.z = (A == 0);
      P.n = (A >> 7);
      log(opcode_addr, opcode, "LDA:       A <- $%02X\n", A);
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

      X   = readByte(getArgAddr(mode, true));
      P.z = (X == 0);
      P.n = (X >> 7);
      log(opcode_addr, opcode, "LDX:       X <- $%02X\n", X);
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

      Y   = readByte(getArgAddr(mode, true));
      P.z = (Y == 0);
      P.n = (Y >> 7);
      log(opcode_addr, opcode, "LDY:       Y <- $%02X\n", Y);
    } break;


    // Logical shift right
    case (asInt(Instruction::LSR) + asInt(AddressingMode::accumulator)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      P.c = (A & 0x01);
      A >>= 1;
      P.z = (A == 0);
      P.n = (A >> 7);  // Always 0
      log(opcode_addr, opcode, "LSR:       A <- $%02X\n", A);
    } break;
    case (asInt(Instruction::LSR) + asInt(AddressingMode::absolute_x)):
      tick();
      __attribute__((fallthrough));
    case (asInt(Instruction::LSR) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::LSR) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::LSR) + asInt(AddressingMode::absolute)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::LSR));
      uint8_t        arg  = readByte(addr);
      writeByte(addr, arg);  // Double write (For absolute addressing)

      P.c = (arg & 0x01);
      arg >>= 1;
      P.z = (arg == 0);
      P.n = (arg >> 7);  // Always 0
      writeByte(addr, arg);
      log(opcode_addr, opcode, "LSR:   $%04X <- $%02X\n", addr, arg);
    } break;


    // No operation
    case (asInt(Instruction::NOP) + asInt(AddressingMode::implied)):
      readByte(PC);  // Dummy read (For one-byte opcodes)
      log(opcode_addr, opcode, "NOP\n");
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
      A |= readByte(getArgAddr(opcode - asInt(Instruction::ORA), true));
      P.z = (A == 0);
      P.n = (A >> 7);
      log(opcode_addr, opcode, "ORA:       A <- $%02X\n", A);
    } break;


    // Register manipulation
    case (asInt(Instruction::TAX) + asInt(AddressingMode::implied)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      X   = A;
      P.z = (X == 0);
      P.n = (X >> 7);
      log(opcode_addr, opcode, "TAX:       X <- $%02X\n", X);
    } break;
    case (asInt(Instruction::TXA) + asInt(AddressingMode::implied)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      A   = X;
      P.z = (A == 0);
      P.n = (A >> 7);
      log(opcode_addr, opcode, "TXA:       A <- $%02X\n", A);
    } break;
    case (asInt(Instruction::DEX) + asInt(AddressingMode::implied)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      X--;
      P.z = (X == 0);
      P.n = (X >> 7);
      log(opcode_addr, opcode, "DEX:       X <- $%02X\n", X);
    } break;
    case (asInt(Instruction::INX) + asInt(AddressingMode::implied)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      X++;
      P.z = (X == 0);
      P.n = (X >> 7);
      log(opcode_addr, opcode, "INX:       X <- $%02X\n", X);
    } break;
    case (asInt(Instruction::TAY) + asInt(AddressingMode::implied)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      Y   = A;
      P.z = (Y == 0);
      P.n = (Y >> 7);
      log(opcode_addr, opcode, "TAY:       Y <- $%02X\n", Y);
    } break;
    case (asInt(Instruction::TYA) + asInt(AddressingMode::implied)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      A   = Y;
      P.z = (A == 0);
      P.n = (A >> 7);
      log(opcode_addr, opcode, "TAY:       A <- $%02X\n", A);
    } break;
    case (asInt(Instruction::DEY) + asInt(AddressingMode::implied)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      Y--;
      P.z = (Y == 0);
      P.n = (Y >> 7);
      log(opcode_addr, opcode, "DEY:       Y <- $%02X\n", Y);
    } break;
    case (asInt(Instruction::INY) + asInt(AddressingMode::implied)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      Y++;
      P.z = (Y == 0);
      P.n = (Y >> 7);
      log(opcode_addr, opcode, "INY:       Y <- $%02X\n", Y);
    } break;


    // Rotate left
    case (asInt(Instruction::ROL) + asInt(AddressingMode::accumulator)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      const bool old_carry = (A >> 7);
      A                    = (A << 1) | P.c;
      P.c                  = old_carry;
      P.z                  = (A == 0);
      P.n                  = (A >> 7);
      log(opcode_addr, opcode, "ROL:       A <- $%02X\n", A);
    } break;
    case (asInt(Instruction::ROL) + asInt(AddressingMode::absolute_x)):
      tick();
      __attribute__((fallthrough));
    case (asInt(Instruction::ROL) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::ROL) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::ROL) + asInt(AddressingMode::absolute)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::ROL));
      uint8_t        arg  = readByte(addr);
      writeByte(addr, arg);  // Double write (For absolute addressing)

      const bool old_carry = (arg >> 7);
      arg                  = (arg << 1) | P.c;
      P.c                  = old_carry;
      P.z                  = (arg == 0);
      P.n                  = (arg >> 7);
      writeByte(addr, arg);
      log(opcode_addr, opcode, "ROL:   $%04X <- $%02X\n", addr, arg);
    } break;


    // Rotate right
    case (asInt(Instruction::ROR) + asInt(AddressingMode::accumulator)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      const bool old_carry = (A & 0x01);
      A                    = (A >> 1) | (P.c << 7);
      P.c                  = old_carry;
      P.z                  = (A == 0);
      P.n                  = (A >> 7);
      log(opcode_addr, opcode, "ROR:       A <- $%02X\n", A);
    } break;
    case (asInt(Instruction::ROR) + asInt(AddressingMode::absolute_x)):
      tick();
      __attribute__((fallthrough));
    case (asInt(Instruction::ROR) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::ROR) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::ROR) + asInt(AddressingMode::absolute)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::ROR));
      uint8_t        arg  = readByte(addr);
      writeByte(addr, arg);  // Double write (For absolute addressing)

      const bool old_carry = (arg & 0x01);
      arg                  = (arg >> 1) | (P.c << 7);
      P.c                  = old_carry;
      P.z                  = (arg == 0);
      P.n                  = (arg >> 7);
      writeByte(addr, arg);
      log(opcode_addr, opcode, "ROR:   $%04X <- $%02X\n", addr, arg);
    } break;


    // Return from interrupt
    case (asInt(Instruction::RTI) + asInt(AddressingMode::implied)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      P.raw = pop() & 0xCF;
      PC    = pop16();
      log(opcode_addr, opcode, "RTI:   $%04X\n", PC);
    } break;


    // Return from subroutine
    case (asInt(Instruction::RTS) + asInt(AddressingMode::implied)): {
      readByte(PC);  // Dummy read (For one-byte opcodes)
      tick();
      PC = pop16() + 1;
      log(opcode_addr, opcode, "RTS:   $%04X\n", PC);
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
      const uint8_t  arg      = readByte(getArgAddr(opcode - asInt(Instruction::SBC), true));
      const uint16_t res_long = A + ~arg + P.c;
      const uint8_t  result   = res_long;
      P.c                     = !((res_long >> 8) & 0x01);
      P.z                     = (result == 0);
      P.v                     = (A >> 7 != arg >> 7) && (A >> 7 != result >> 7);
      P.n                     = (result >> 7);
      A                       = result;
      log(opcode_addr, opcode, "SBC:       A <- $%02X\n", A);
    } break;


    // Store accumulator
    case (asInt(Instruction::STA) + asInt(AddressingMode::absolute_x)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::absolute_y)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::indirect_y)):
      tick();
      __attribute__((fallthrough));
    case (asInt(Instruction::STA) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::absolute)):
    case (asInt(Instruction::STA) + asInt(AddressingMode::indirect_x)): {
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::STA));
      writeByte(addr, A);
      log(opcode_addr, opcode, "STA:   $%04X <- $%02X\n", addr, A);
    } break;


    // Stack manipulation
    case (asInt(Instruction::TXS) + asInt(AddressingMode::implied)):
      tick();
      SP = X;
      log(opcode_addr, opcode, "TXS:      SP <- $%02X\n", SP);
      break;
    case (asInt(Instruction::TSX) + asInt(AddressingMode::implied)):
      tick();
      X   = SP;
      P.z = (X == 0);
      P.n = (X >> 7);
      log(opcode_addr, opcode, "TSX:       X <- $%02X\n", X);
      break;
    case (asInt(Instruction::PHA) + asInt(AddressingMode::implied)):
      tick(2);
      push(A);
      log(opcode_addr, opcode, "PHA:      SP <- $%02X\n", A);
      break;
    case (asInt(Instruction::PLA) + asInt(AddressingMode::implied)):
      tick(3);
      A   = pop();
      P.z = (A == 0);
      P.n = (A >> 7);
      log(opcode_addr, opcode, "PLA:       A <- $%02X\n", A);
      break;
    case (asInt(Instruction::PHP) + asInt(AddressingMode::implied)):
      tick(2);
      P.b = 0b11;
      push(P.raw);
      log(opcode_addr, opcode, "PHP:      SP <- $%02X\n", P.raw);
      break;
    case (asInt(Instruction::PLP) + asInt(AddressingMode::implied)):
      tick(3);
      P.raw = pop() & 0xCF;
      log(opcode_addr, opcode, "PHP:       P <- $%02X\n", P.raw);
      break;


    // Store X register
    case (asInt(Instruction::STX) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::STX) + asInt(AddressingMode::zero_page_x)):  // Should be zero_page_y
    case (asInt(Instruction::STX) + asInt(AddressingMode::absolute)): {
      tick();
      uint16_t addr;
      if (opcode == asInt(Instruction::STX) + asInt(AddressingMode::zero_page_x)) {
        addr = getArgAddr(AddressingMode::zero_page_y);
      } else {
        addr = getArgAddr(opcode - asInt(Instruction::STX));
      }
      writeByte(addr, X);
      log(opcode_addr, opcode, "STX:   $%04X <- $%02X\n", addr, X);
    } break;


    // Store Y register
    case (asInt(Instruction::STY) + asInt(AddressingMode::zero_page)):
    case (asInt(Instruction::STY) + asInt(AddressingMode::zero_page_x)):
    case (asInt(Instruction::STY) + asInt(AddressingMode::absolute)): {
      tick();
      const uint16_t addr = getArgAddr(opcode - asInt(Instruction::STY));
      writeByte(addr, Y);
      log(opcode_addr, opcode, "STX:   $%04X <- $%02X\n", addr, Y);
    } break;


    // Illegal instruction
    default:
      std::cout << "Illegal instruction " << unsigned(opcode) << " at address $" << unsigned(PC - 1) << std::endl;
      reset(true);
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

void cpu::CPU::reset(bool active) {
  irq_reset_ = active;
}


// =*=*=*=*= CPU Internal Operations =*=*=*=*=

uint8_t cpu::CPU::readByte(uint16_t address) {
  tick();
  return bus_->read(address);
}

void cpu::CPU::writeByte(uint16_t address, uint8_t data) {
  tick();
  bus_->write(address, data);
}

void cpu::CPU::push(uint8_t data) {
  writeByte(0x0100 + SP--, data);
}

uint8_t cpu::CPU::pop() {
  tick();  // Increment SP
  return readByte(0x0100 + (++SP));
}

uint16_t cpu::CPU::pop16() {
  tick();  // Increment SP by 2
  uint16_t val = readByte(0x0100 + (++SP));
  val |= readByte(0x0100 + (++SP)) << 8;
  return val;
}

void cpu::CPU::tick(int ticks) {
  for (; ticks > 0; ticks--) {
    ppu_->tick();
    ppu_->tick();
    ppu_->tick();
    timer_.sleep();
  }
}

void cpu::CPU::interrupt(uint16_t vector_table) {
  tick();  // TODO: Confirm number of ticks for NMI
  push(PC >> 8);
  push(PC);
  push(P.raw);

  irq_brk_ = false;
  P.i      = true;
  PC       = readByte(vector_table) | (readByte(vector_table + 1) << 8);
  logger::log<logger::DEBUG_CPU>("Interrupt, jumping to $%04X\n", vector_table);
}

void cpu::CPU::branch(bool condition) {
  const int8_t offset = utils::deComplement(readByte(getArgAddr(AddressingMode::relative)));
  if (condition) {
    tick();
    const uint8_t src_page = PC >> 8;
    PC += offset;

    // Extra tick if page boundary crossed
    if (src_page != (PC >> 8)) {
      tick();
    }
  }
}

uint16_t cpu::CPU::getArgAddr(std::underlying_type<AddressingMode>::type mode, bool check_page_boundary) {
  return getArgAddr(utils::asEnum<AddressingMode>(mode), check_page_boundary);
}

uint16_t cpu::CPU::getArgAddr(AddressingMode mode, bool check_page_boundary) {
  switch (mode) {
    case (AddressingMode::immediate):  // Or relative
      return PC++;

    case (AddressingMode::zero_page):
      return readByte(PC++);

    case (AddressingMode::zero_page_x):
      tick();
      return (readByte(PC++) + X) & 0xFF;

    case (AddressingMode::zero_page_y):
      tick();
      return (readByte(PC++) + Y) & 0xFF;

    case (AddressingMode::absolute): {
      const uint16_t addr = readByte(PC++);
      return addr | (readByte(PC++) << 8);
    }

    case (AddressingMode::absolute_x): {
      const uint16_t addr = readByte(PC++);

      // If low byte + X carries, take an extra tick to correct the high byte
      if (check_page_boundary && ((addr >> 8) != ((addr + X) >> 8))) {
        tick();
      }
      return (addr | (readByte(PC++) << 8)) + X;
    }

    case (AddressingMode::absolute_y): {
      const uint16_t addr = readByte(PC++);

      // If low byte + Y carries, take an extra tick to correct the high byte
      if (check_page_boundary && ((addr >> 8) != ((addr + Y) >> 8))) {
        tick();
      }
      return (addr | (readByte(PC++) << 8)) + Y;
    }

    case (AddressingMode::indirect): {
      uint16_t addr = readByte(PC++);
      addr |= (readByte(PC++) << 8);
      return readByte(addr) | (readByte(addr + 1) << 8);
    }

    case (AddressingMode::indirect_x): {
      tick();
      const uint8_t addr = readByte(PC++) + X;
      return readByte(addr) | (readByte((addr + 1) & 0xFF) << 8);
    }

    case (AddressingMode::indirect_y): {
      const uint8_t addr = readByte(PC++);

      // If low byte + Y carries, take an extra tick to correct the high byte
      if (check_page_boundary && ((addr >> 8) != ((addr + Y) >> 8))) {
        tick();
      }
      return (readByte(addr) | (readByte((addr + 1) & 0xFF) << 8)) + Y;
    }

    default:
      std::cerr << "Unknown addressing mode" << std::endl;
      exit(1);
  };
}
