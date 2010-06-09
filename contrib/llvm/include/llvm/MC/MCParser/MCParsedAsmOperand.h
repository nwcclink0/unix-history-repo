//===-- llvm/MC/MCParsedAsmOperand.h - Asm Parser Operand -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCASMOPERAND_H
#define LLVM_MC_MCASMOPERAND_H

namespace llvm {
class SMLoc;

/// MCParsedAsmOperand - This abstract class represents a source-level assembly
/// instruction operand.  It should be subclassed by target-specific code.  This
/// base class is used by target-independent clients and is the interface
/// between parsing an asm instruction and recognizing it.
class MCParsedAsmOperand {
public:  
  MCParsedAsmOperand() {}
  virtual ~MCParsedAsmOperand() {}
  
  /// getStartLoc - Get the location of the first token of this operand.
  virtual SMLoc getStartLoc() const;
  /// getEndLoc - Get the location of the last token of this operand.
  virtual SMLoc getEndLoc() const;
};

} // end namespace llvm.

#endif
