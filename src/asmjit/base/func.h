// [AsmJit]
// Complete x86/x64 JIT and Remote Assembler for C++.
//
// [License]
// Zlib - See LICENSE.md file in the package.

// [Guard]
#ifndef _ASMJIT_BASE_FUNC_H
#define _ASMJIT_BASE_FUNC_H

#include "../build.h"

// [Dependencies]
#include "../base/operand.h"
#include "../base/utils.h"

// [Api-Begin]
#include "../apibegin.h"

namespace asmjit {

//! \addtogroup asmjit_base
//! \{

// ============================================================================
// [asmjit::CallConv]
// ============================================================================

//! Function calling convention.
//!
//! Function calling convention is a scheme that defines how function parameters
//! are passed and how function returns its result. AsmJit defines a variety of
//! architecture and OS specific calling conventions and also provides a compile
//! time detection to make JIT code-generation easier.
struct CallConv {
  //! Calling convention id.
  ASMJIT_ENUM(Id) {
    //! None or invalid (can't be used).
    kIdNone = 0,

    // ------------------------------------------------------------------------
    // [X86]
    // ------------------------------------------------------------------------

    //! X86 `__cdecl` calling convention (used by C runtime and libraries).
    kIdX86CDecl = 1,
    //! X86 `__stdcall` calling convention (used mostly by WinAPI).
    kIdX86StdCall = 2,
    //! X86 `__thiscall` calling convention (MSVC/Intel).
    kIdX86MsThisCall = 3,
    //! X86 `__fastcall` convention (MSVC/Intel).
    kIdX86MsFastCall = 4,
    //! X86 `__fastcall` convention (GCC and Clang).
    kIdX86GccFastCall = 5,
    //! X86 `regparm(1)` convention (GCC and Clang).
    kIdX86GccRegParm1 = 6,
    //! X86 `regparm(2)` convention (GCC and Clang).
    kIdX86GccRegParm2 = 7,
    //! X86 `regparm(3)` convention (GCC and Clang).
    kIdX86GccRegParm3 = 8,

    //! X64 calling convention defined by WIN64-ABI.
    //!
    //! Links:
    //!   * <http://msdn.microsoft.com/en-us/library/9b372w95.aspx>.
    kIdX86Win64 = 16,
    //! X64 calling convention used by Unix platforms (AMD64-ABI).
    kIdX86Unix64 = 17,

    // ------------------------------------------------------------------------
    // [ARM]
    // ------------------------------------------------------------------------

    //! Legacy calling convention, floating point arguments are passed via GP registers.
    kIdArm32SoftFP = 32,
    //! Modern calling convention, uses VFP registers to pass floating point arguments.
    kIdArm32HardFP = 33,

    // ------------------------------------------------------------------------
    // [Internal]
    // ------------------------------------------------------------------------

    _kIdX86Start = 1,   //!< \internal
    _kIdX86End = 8,     //!< \internal

    _kIdX64Start = 16,  //!< \internal
    _kIdX64End = 17,    //!< \internal

    _kIdArmStart = 32,  //!< \internal
    _kIdArmEnd = 33,    //!< \internal

    // ------------------------------------------------------------------------
    // [Host]
    // ------------------------------------------------------------------------

#if defined(ASMJIT_DOCGEN)
    //! Default calling convention based on the current C++ compiler's settings.
    //!
    //! NOTE: This should be always the same as `kIdHostCDecl`, but some
    //! compilers allow to override the default calling convention. Overriding
    //! is not detected at the moment.
    kIdHost         = DETECTED_AT_COMPILE_TIME,

    //! Default CDECL calling convention based on the current C++ compiler's settings.
    kIdHostCDecl    = DETECTED_AT_COMPILE_TIME,

    //! Default STDCALL calling convention based on the current C++ compiler's settings.
    //!
    //! NOTE: If not defined by the host then it's the same as `kIdHostCDecl`.
    kIdHostStdCall  = DETECTED_AT_COMPILE_TIME,

    //! Compatibility for `__fastcall` calling convention.
    //!
    //! NOTE: If not defined by the host then it's the same as `kIdHostCDecl`.
    kIdHostFastCall = DETECTED_AT_COMPILE_TIME
#elif ASMJIT_ARCH_X86
    kIdHost         = kIdX86CDecl,
    kIdHostCDecl    = kIdX86CDecl,
    kIdHostStdCall  = kIdX86StdCall,
    kIdHostFastCall = ASMJIT_CC_MSC   ? kIdX86MsFastCall  :
                            ASMJIT_CC_GCC   ? kIdX86GccFastCall :
                            ASMJIT_CC_CLANG ? kIdX86GccFastCall : kIdNone
#elif ASMJIT_ARCH_X64
    kIdHost         = ASMJIT_OS_WINDOWS ? kIdX86Win64 : kIdX86Unix64,
    kIdHostCDecl    = kIdHost, // Doesn't exist, redirected to host.
    kIdHostStdCall  = kIdHost, // Doesn't exist, redirected to host.
    kIdHostFastCall = kIdHost  // Doesn't exist, redirected to host.
#elif ASMJIT_ARCH_ARM32
# if defined(__SOFTFP__)
    kIdHost         = kIdArm32SoftFP,
# else
    kIdHost         = kIdArm32HardFP,
# endif
    // These don't exist on ARM.
    kIdHostCDecl    = kIdHost, // Doesn't exist, redirected to host.
    kIdHostStdCall  = kIdHost, // Doesn't exist, redirected to host.
    kIdHostFastCall = kIdHost  // Doesn't exist, redirected to host.
#else
# error "[asmjit] Couldn't determine the target's calling convention."
#endif
  };

  //! Calling convention algorithm.
  //!
  //! This is AsmJit specific. It basically describes how should AsmJit convert
  //! the function arguments defined by `FuncSignature` into register ids or
  //! stack offsets. The default algorithm is a standard algorithm that assigns
  //! registers first, and then assigns stack. The Win64 algorithm does register
  //! shadowing as defined by `WIN64` calling convention - it applies to 64-bit
  //! calling conventions only.
  enum Algorithm {
    kAlgorithmDefault    = 0,            //!< Default algorithm (cross-platform).
    kAlgorithmWin64      = 1             //!< WIN64 specific algorithm.
  };

  //! Calling convention flags.
  enum Flags {
    kFlagCalleePopsStack = 0x0001,       //!< Callee is responsible for cleaning up the stack.
    kFlagPassFloatsByVec = 0x0002,       //!< Pass F32 and F64 arguments by VEC128 register.
    kFlagVectorCall      = 0x0004,       //!< This is a '__vectorcall' calling convention.
    kFlagIndirectVecArgs = 0x0008        //!< Pass vector arguments indirectly (as a pointer).
  };

  //! Internal limits of CallConv.
  enum Limits {
    kNumRegKinds         = 4,            //!< Number of RegKinds handled by CallConv.
    kNumRegArgsPerKind   = 8             //!< Number of maximum arguments passed in register per RegKind.
  };

  //! Passed registers' order.
  struct RegOrder {
    uint8_t id[kNumRegArgsPerKind];      //!< Passed registers, order matters.
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  ASMJIT_API Error init(uint32_t ccId) noexcept;

  ASMJIT_INLINE void reset() noexcept {
    ::memset(this, 0, sizeof(*this));
    ::memset(_passedOrder, 0xFF, sizeof(_passedOrder));
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get calling convention id, see \ref Id.
  ASMJIT_INLINE uint32_t getId() const noexcept { return _id; }

  //! Get calling convention algorithm, see \ref Algorithm.
  ASMJIT_INLINE uint32_t getAlgorithm() const noexcept { return _algorithm; }
  //! Set calling convention algorithm, see \ref Algorithm.
  ASMJIT_INLINE void setAlgorithm(uint32_t algorithm) noexcept { _algorithm = static_cast<uint8_t>(algorithm); }

  //! Get calling convention flags, see \ref Flags.
  ASMJIT_INLINE uint32_t getFlags() const noexcept { return _flags; }
  //! Add calling convention flags, see \ref Flags.
  ASMJIT_INLINE void setFlags(uint32_t flag) noexcept { _flags = flag; };
  //! Add calling convention flags, see \ref Flags.
  ASMJIT_INLINE void addFlags(uint32_t flag) noexcept { _flags |= flag; };
  //! Get if the calling convention has the given `flag` set.
  ASMJIT_INLINE bool hasFlag(uint32_t flag) const noexcept { return (_flags & flag) != 0; }

  //! Get if this calling convention specifies 'RedZone'.
  ASMJIT_INLINE bool hasRedZone() const noexcept { return _redZoneSize != 0; }
  //! Get size of 'RedZone'.
  ASMJIT_INLINE uint32_t getRedZoneSize() const noexcept { return _redZoneSize; }
  //! Set size of 'RedZone'.
  ASMJIT_INLINE void setRedZoneSize(uint32_t size) noexcept { _redZoneSize = static_cast<uint16_t>(size); }

  //! Get if this calling convention specifies 'SpillZone'.
  ASMJIT_INLINE bool hasSpillZone() const noexcept { return _spillZoneSize != 0; }
  //! Get size of 'SpillZone'.
  ASMJIT_INLINE uint32_t getSpillZoneSize() const noexcept { return _spillZoneSize; }
  //! Set size of 'SpillZone'.
  ASMJIT_INLINE void setSpillZoneSize(uint32_t size) noexcept { _spillZoneSize = static_cast<uint16_t>(size); }

  ASMJIT_INLINE const uint8_t* getPassedOrder(uint32_t regKind) const noexcept {
  }

  ASMJIT_INLINE uint32_t getPassedMask(uint32_t regKind) const noexcept {
    ASMJIT_ASSERT(regKind < kNumRegKinds);
    return _passedMask[regKind];
  }

  ASMJIT_INLINE void _setPassedPacked(uint32_t regKind, uint32_t p0, uint32_t p1) noexcept {
    ASMJIT_ASSERT(regKind < kNumRegKinds);

    reinterpret_cast<uint32_t*>(_passedOrder[regKind].id)[0] = p0;
    reinterpret_cast<uint32_t*>(_passedOrder[regKind].id)[1] = p1;
  }

  ASMJIT_INLINE void setPassedToNone(uint32_t regKind) noexcept {
    ASMJIT_ASSERT(regKind < kNumRegKinds);

    _setPassedPacked(regKind, ASMJIT_PACK32_4x8(0xFF, 0xFF, 0xFF, 0xFF),
                              ASMJIT_PACK32_4x8(0xFF, 0xFF, 0xFF, 0xFF));
    _passedMask[regKind] = 0;
  }

  ASMJIT_INLINE void setPassedOrder(uint32_t regKind, uint32_t a0, uint32_t a1 = 0xFF, uint32_t a2 = 0xFF, uint32_t a3 = 0xFF, uint32_t a4 = 0xFF, uint32_t a5 = 0xFF, uint32_t a6 = 0xFF, uint32_t a7 = 0xFF) noexcept {
    ASMJIT_ASSERT(regKind < kNumRegKinds);

    _setPassedPacked(regKind, ASMJIT_PACK32_4x8(a0, a1, a2, a3),
                              ASMJIT_PACK32_4x8(a4, a5, a6, a7));

    // NOTE: This should always be called with all arguments known at compile
    // time, so even if it looks scary it should be translated to a single
    // instruction.
    _passedMask[regKind] = (a0 != 0xFF ? 1U << a0 : 0U) |
                           (a1 != 0xFF ? 1U << a1 : 0U) |
                           (a2 != 0xFF ? 1U << a2 : 0U) |
                           (a3 != 0xFF ? 1U << a3 : 0U) |
                           (a4 != 0xFF ? 1U << a4 : 0U) |
                           (a5 != 0xFF ? 1U << a5 : 0U) |
                           (a6 != 0xFF ? 1U << a6 : 0U) |
                           (a7 != 0xFF ? 1U << a7 : 0U) ;
  }

  ASMJIT_INLINE uint32_t getPreservedMask(uint32_t regKind) const noexcept {
    ASMJIT_ASSERT(regKind < kNumRegKinds);
    return _preservedMask[regKind];
  }

  ASMJIT_INLINE void setPreservedMask(uint32_t regKind, uint32_t mask) noexcept {
    ASMJIT_ASSERT(regKind < kNumRegKinds);
    _preservedMask[regKind] = mask;
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _id;                           //!< Calling convention id.
  uint8_t _algorithm;                    //!< Algorithm to create FuncFrame.
  uint16_t _flags;                       //!< Calling convention flags.
  uint16_t _redZoneSize;                 //!< Red zone size (AMD64 == 128 bytes).
  uint16_t _spillZoneSize;               //!< Spill zone size (WIN64 == 32 bytes).

  uint32_t _preservedMask[kNumRegKinds]; //!< Mask of all preserved registers, per kind.
  uint32_t _passedMask[kNumRegKinds];    //!< Mask of all passed registers, per kind.
  RegOrder _passedOrder[kNumRegKinds];   //!< Passed registers' order, per kind.
};

// ============================================================================
// [asmjit::FuncFrame]
// ============================================================================

//! Function frame.
//!
//! This structure can be used to create a function frame in a cross-platform
//! way. It contains information about the function's stack to be used and
//! registers to be saved and restored. Based on this information in can
//! calculate the optimal layout of the function that can be then passed to an
//! architecture-specific emitter.
struct FuncFrame {
  enum Flags {
  };

  //! FuncFrame is bound to the same limits as \ref CallConv.
  enum Limits {
    kNumRegKinds = CallConv::kNumRegKinds
  };

  uint8_t _naturalStackAlignment;        //!< Natural stack alignment as defined by OS/ABI.
  uint8_t _requiredStackAlignment;       //!< Required stack alignment to spill / store data.

  uint32_t _stackDataSize;               //!< Stack size requested by the function for its data.
  uint32_t _stackDataAlignment;          //!< Minimum alignment of the stack requested by the function.
  uint32_t _savedMask[kNumRegKinds];     //!< Registers which must be saved/restored in prolog/epilog.

  uint16_t _savedGpSize;                 //!< Stack required to save GP regs.
  uint16_t _savedVecSize;                //!< Stack required to save VEC regs.
};

// ============================================================================
// [asmjit::FuncHint]
// ============================================================================

//! Function hints.
ASMJIT_ENUM(FuncHint) {
  //! Generate a naked function by omitting its prolog and epilog (default true).
  //!
  //! Naked functions should always result in less code required for function's
  //! prolog and epilog. In addition, on X86/64 naked functions save one register
  //! (ebp or rbp), which can be used by the function instead.
  kFuncHintNaked = 0,

  //! Generate a compact function prolog/epilog if possible (default true).
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Use shorter, but possible slower prolog/epilog sequence to save/restore
  //! registers. At the moment this only enables emitting `leave` in function's
  //! epilog to make the code shorter, however, the counterpart `enter` is not
  //! used in function's prolog for performance reasons.
  kFuncHintCompact = 1,

  //! Emit `emms` instruction in the function's epilog.
  kFuncHintX86Emms = 17
};

// ============================================================================
// [asmjit::FuncFlags]
// ============================================================================

//! Function flags.
ASMJIT_ENUM(FuncFlags) {
  //! Whether the function is using naked (minimal) prolog / epilog.
  kFuncFlagIsNaked = 0x00000001,

  //! Whether an another function is called from this function.
  kFuncFlagIsCaller = 0x00000002,

  //! Whether the stack is not aligned to the required stack alignment,
  //! thus it has to be aligned manually.
  kFuncFlagIsStackMisaligned = 0x00000004,

  //! Whether the stack pointer is adjusted by the stack size needed
  //! to save registers and function variables.
  //!
  //! X86/X64 Specific
  //! ----------------
  //!
  //! Stack pointer (ESP/RSP) is adjusted by 'sub' instruction in prolog and by
  //! 'add' instruction in epilog (only if function is not naked). If function
  //! needs to perform manual stack alignment more instructions are used to
  //! adjust the stack (like "and zsp, -Alignment").
  kFuncFlagIsStackAdjusted = 0x00000008,

  //! Whether the function is finished using `CodeCompiler::endFunc()`.
  kFuncFlagIsFinished = 0x80000000,

  //! Whether to emit `leave` instead of two instructions in case that the
  //! function saves and restores the frame pointer.
  kFuncFlagX86Leave = 0x00010000,

  //! Whether it's required to move arguments to a new stack location,
  //! because of manual aligning.
  kFuncFlagX86MoveArgs = 0x00040000,

  //! Whether to emit `emms` instruction in epilog (auto-detected).
  kFuncFlagX86Emms = 0x01000000
};

// ============================================================================
// [asmjit::FuncMisc]
// ============================================================================

enum {
  //! Function doesn't have variable number of arguments (`...`) (default).
  kFuncNoVarArgs = 0xFF
};

// ============================================================================
// [asmjit::FuncArgIndex]
// ============================================================================

//! Function argument index (lo/hi).
ASMJIT_ENUM(FuncArgIndex) {
  //! Maximum number of function arguments supported by AsmJit.
  kFuncArgCount = 16,
  //! Extended maximum number of arguments (used internally).
  kFuncArgCountLoHi = kFuncArgCount * 2,

  //! Index to the LO part of function argument (default).
  //!
  //! This value is typically omitted and added only if there is HI argument
  //! accessed.
  kFuncArgLo = 0,

  //! Index to the HI part of function argument.
  //!
  //! HI part of function argument depends on target architecture. On x86 it's
  //! typically used to transfer 64-bit integers (they form a pair of 32-bit
  //! integers).
  kFuncArgHi = kFuncArgCount
};

// ============================================================================
// [asmjit::FuncRet]
// ============================================================================

//! Function return value (lo/hi) specification.
ASMJIT_ENUM(FuncRet) {
  //! Index to the LO part of function return value.
  kFuncRetLo = 0,
  //! Index to the HI part of function return value.
  kFuncRetHi = 1
};

// ============================================================================
// [asmjit::FuncInOut]
// ============================================================================

//! Function in/out - argument or return value translated from `FuncSignature`.
struct FuncInOut {
  ASMJIT_ENUM(Components) {
    kTypeIdShift      = 24,
    kTypeIdMask       = 0xFF000000U,

    kRegTypeShift     = 8,
    kRegTypeMask      = 0x0000FF00U,

    kRegIdShift       = 0,
    kRegIdMask        = 0x000000FFU,

    kStackOffsetShift = 0,
    kStackOffsetMask  = 0x0000FFFFU,

    kIsByReg          = 0x00010000U,
    kIsByStack        = 0x00020000U,
    kIsIndirectArg    = 0x00040000U
  };

  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  //! Get if this FuncInOut is initialized (i.e. contains a valid data).
  ASMJIT_INLINE bool isInitialized() const noexcept { return _value != 0; }

  //! Initialize this in/out by a given `typeId`.
  ASMJIT_INLINE void initTypeId(uint32_t typeId) noexcept {
    _value = typeId << kTypeIdShift;
  }

  //! Initialize this in/out by a given `typeId`, `regType`, and `regId`.
  ASMJIT_INLINE void initReg(uint32_t typeId, uint32_t regType, uint32_t regId) noexcept {
    _value = (typeId << kTypeIdShift) | (regType << kRegTypeShift) | (regId << kRegIdShift) | kIsByReg;
  }

  //! Initialize this in/out by a given `typeId` and `offset`.
  ASMJIT_INLINE void initStack(uint32_t typeId, uint32_t offset) noexcept {
    _value = (typeId << kTypeIdShift) | (offset << kStackOffsetShift) | kIsByStack;
  }

  //! Reset FuncInOut to uninitialized state.
  ASMJIT_INLINE void reset() noexcept { _value = 0; }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void assignToReg(uint32_t regType, uint32_t regId) noexcept {
    ASMJIT_ASSERT(!isAssigned());
    _value |= (regType << kRegTypeShift) | (regId << kRegIdShift) | kIsByReg;
  }

  ASMJIT_INLINE void assignToStack(int32_t offset) noexcept {
    ASMJIT_ASSERT(!isAssigned());
    _value |= (offset << kStackOffsetShift) | kIsByStack;
  }

  //! Get if this argument is passed by register.
  ASMJIT_INLINE bool byReg() const noexcept { return (_value & kIsByReg) != 0; }
  //! Get if this argument is passed by stack.
  ASMJIT_INLINE bool byStack() const noexcept { return (_value & kIsByStack) != 0; }
  //! Get if this argument is passed by register.
  ASMJIT_INLINE bool isAssigned() const noexcept { return (_value & (kIsByReg | kIsByStack)) != 0; }
  //! Get if this argument is passed through a pointer (used by Win64 to pass XMM|YMM|ZMM).
  ASMJIT_INLINE bool isIndirectArg() const noexcept { return (_value & kIsIndirectArg) != 0; }

  //! Get virtual type of this argument or return value.
  ASMJIT_INLINE uint32_t getTypeId() const noexcept { return _value >> kTypeIdShift; }
  //! Get a physical id of the register used to pass the argument or return the value.
  ASMJIT_INLINE uint32_t getRegId() const noexcept { return (_value & kRegIdMask) >> kRegIdShift; }
  //! Get a register type of the register used to pass the argument or return the value.
  ASMJIT_INLINE uint32_t getRegType() const noexcept { return (_value & kRegTypeMask) >> kRegTypeShift; }
  //! Get a stack offset of this argument (always positive).
  ASMJIT_INLINE int32_t getStackOffset() const noexcept { return (_value & kStackOffsetMask) >> kStackOffsetShift; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint32_t _value;
};

// ============================================================================
// [asmjit::FuncSignature]
// ============================================================================

//! Function signature.
//!
//! Contains information about function return type, count of arguments and
//! their TypeIds. Function signature is a low level structure which doesn't
//! contain platform specific or calling convention specific information.
struct FuncSignature {
  // --------------------------------------------------------------------------
  // [Setup]
  // --------------------------------------------------------------------------

  //! Setup the prototype.
  ASMJIT_INLINE void setup(
    uint32_t ccId,
    uint32_t ret,
    const uint8_t* args, uint32_t argCount) noexcept {

    ASMJIT_ASSERT(ccId <= 0xFF);
    ASMJIT_ASSERT(argCount <= 0xFF);

    _callConv = static_cast<uint8_t>(ccId);
    _varArgs = kFuncNoVarArgs;
    _argCount = static_cast<uint8_t>(argCount);
    _ret = ret;
    _args = args;
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  //! Get the function's calling convention.
  ASMJIT_INLINE uint32_t getCallConv() const noexcept { return _callConv; }
  //! Get the variable arguments `...` index, `kFuncNoVarArgs` if none.
  ASMJIT_INLINE uint32_t getVarArgs() const noexcept { return _varArgs; }
  //! Get the number of function arguments.
  ASMJIT_INLINE uint32_t getArgCount() const noexcept { return _argCount; }

  ASMJIT_INLINE bool hasRet() const noexcept { return _ret != TypeId::kVoid; }
  //! Get the return value type.
  ASMJIT_INLINE uint32_t getRet() const noexcept { return _ret; }

  //! Get the type of the argument at index `i`.
  ASMJIT_INLINE uint32_t getArg(uint32_t i) const noexcept {
    ASMJIT_ASSERT(i < _argCount);
    return _args[i];
  }
  //! Get the array of function arguments' types.
  ASMJIT_INLINE const uint8_t* getArgs() const noexcept { return _args; }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _callConv;
  uint8_t _varArgs;
  uint8_t _argCount;
  uint8_t _ret;
  const uint8_t* _args;
};

// ============================================================================
// [asmjit::FuncSignatureX]
// ============================================================================

//! Custom function builder for up to 32 function arguments.
struct FuncSignatureX : public FuncSignature {
  // --------------------------------------------------------------------------
  // [Construction / Destruction]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE FuncSignatureX(uint32_t callConv = CallConv::kIdHost) noexcept {
    setup(callConv, TypeId::kVoid, _builderArgList, 0);
  }

  // --------------------------------------------------------------------------
  // [Accessors]
  // --------------------------------------------------------------------------

  ASMJIT_INLINE void setCallConv(uint32_t callConv) noexcept {
    ASMJIT_ASSERT(callConv <= 0xFF);
    _callConv = static_cast<uint8_t>(callConv);
  }

  //! Set the return type to `retType`.
  ASMJIT_INLINE void setRet(uint32_t retType) noexcept {
    _ret = retType;
  }
  //! Set the return type based on `T`.
  template<typename T>
  ASMJIT_INLINE void setRetT() noexcept { setRet(TypeIdOf<T>::kTypeId); }

  //! Set the argument at index `i` to the `type`
  ASMJIT_INLINE void setArg(uint32_t i, uint32_t type) noexcept {
    ASMJIT_ASSERT(i < _argCount);
    _builderArgList[i] = type;
  }
  //! Set the argument at index `i` to the type based on `T`.
  template<typename T>
  ASMJIT_INLINE void setArgT(uint32_t i) noexcept { setArg(i, TypeIdOf<T>::kTypeId); }

  //! Append an argument of `type` to the function prototype.
  ASMJIT_INLINE void addArg(uint32_t type) noexcept {
    ASMJIT_ASSERT(_argCount < kFuncArgCount);
    _builderArgList[_argCount++] = static_cast<uint8_t>(type);
  }
  //! Append an argument of type based on `T` to the function prototype.
  template<typename T>
  ASMJIT_INLINE void addArgT() noexcept { addArg(TypeIdOf<T>::kTypeId); }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  uint8_t _builderArgList[kFuncArgCount];
};

//! \internal
#define T(TYPE) TypeIdOf<TYPE>::kTypeId

//! Function signature template (no arguments).
template<typename RET>
struct FuncSignature0 : public FuncSignature {
  ASMJIT_INLINE FuncSignature0(uint32_t callConv = CallConv::kIdHost) noexcept {
    setup(callConv, T(RET), nullptr, 0);
  }
};

//! Function signature template (1 argument).
template<typename RET, typename A0>
struct FuncSignature1 : public FuncSignature {
  ASMJIT_INLINE FuncSignature1(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (2 arguments).
template<typename RET, typename A0, typename A1>
struct FuncSignature2 : public FuncSignature {
  ASMJIT_INLINE FuncSignature2(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (3 arguments).
template<typename RET, typename A0, typename A1, typename A2>
struct FuncSignature3 : public FuncSignature {
  ASMJIT_INLINE FuncSignature3(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (4 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3>
struct FuncSignature4 : public FuncSignature {
  ASMJIT_INLINE FuncSignature4(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (5 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4>
struct FuncSignature5 : public FuncSignature {
  ASMJIT_INLINE FuncSignature5(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (6 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5>
struct FuncSignature6 : public FuncSignature {
  ASMJIT_INLINE FuncSignature6(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (7 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct FuncSignature7 : public FuncSignature {
  ASMJIT_INLINE FuncSignature7(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (8 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct FuncSignature8 : public FuncSignature {
  ASMJIT_INLINE FuncSignature8(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6), T(A7) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (9 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct FuncSignature9 : public FuncSignature {
  ASMJIT_INLINE FuncSignature9(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6), T(A7), T(A8) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};

//! Function signature template (10 arguments).
template<typename RET, typename A0, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct FuncSignature10 : public FuncSignature {
  ASMJIT_INLINE FuncSignature10(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { T(A0), T(A1), T(A2), T(A3), T(A4), T(A5), T(A6), T(A7), T(A8), T(A9) };
    setup(callConv, T(RET), args, ASMJIT_ARRAY_SIZE(args));
  }
};
#undef T

#if ASMJIT_CC_HAS_VARIADIC_TEMPLATES
//! Function signature template.
template<typename RET, typename... ARGS>
struct FuncSignatureT : public FuncSignature {
  ASMJIT_INLINE FuncSignatureT(uint32_t callConv = CallConv::kIdHost) noexcept {
    static const uint8_t args[] = { (TypeIdOf<ARGS>::kTypeId)... };
    setup(callConv, TypeIdOf<RET>::kTypeId, args, ASMJIT_ARRAY_SIZE(args));
  }
};
#endif // ASMJIT_CC_HAS_VARIADIC_TEMPLATES

// ============================================================================
// [asmjit::FuncDecl]
// ============================================================================

//! Function declaration.
struct FuncDecl {
  // --------------------------------------------------------------------------
  // [Init / Reset]
  // --------------------------------------------------------------------------

  //! Initialize this `FuncDecl` to the given signature.
  ASMJIT_API Error init(const FuncSignature& sign);

  ASMJIT_INLINE void reset() noexcept { ::memset(this, 0, sizeof(*this)); }

  // --------------------------------------------------------------------------
  // [Accessors - Calling Convention]
  // --------------------------------------------------------------------------

  //! Get the function's calling convention, see `CallConv`.
  ASMJIT_INLINE const CallConv& getCallConv() const noexcept { return _callConv; }
  ASMJIT_INLINE bool hasFlag(uint32_t ccFlag) const noexcept { return _callConv.hasFlag(ccFlag); }

  // --------------------------------------------------------------------------
  // [Accessors - Arguments and Return]
  // --------------------------------------------------------------------------

  //! Get the number of function arguments.
  ASMJIT_INLINE uint32_t getArgCount() const noexcept { return _argCount; }
  //! Get count of function return values.
  ASMJIT_INLINE uint32_t getRetCount() const noexcept { return _retCount; }

  //! Get function arguments array.
  ASMJIT_INLINE FuncInOut* getArgs() noexcept { return _args; }
  //! Get function arguments array (const).
  ASMJIT_INLINE const FuncInOut* getArgs() const noexcept { return _args; }

  //! Get function argument at index `index`.
  ASMJIT_INLINE FuncInOut& getArg(size_t index) noexcept {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _args[index];
  }

  //! Get function argument at index `index`.
  ASMJIT_INLINE const FuncInOut& getArg(size_t index) const noexcept {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    return _args[index];
  }

  ASMJIT_INLINE void resetArg(size_t index) noexcept {
    ASMJIT_ASSERT(index < kFuncArgCountLoHi);
    _args[index].reset();
  }

  //! Get whether the function has a return value.
  ASMJIT_INLINE bool hasRet() const noexcept { return _retCount != 0; }
  //! Get function return value.
  ASMJIT_INLINE FuncInOut& getRet(uint32_t index = kFuncRetLo) noexcept { return _rets[index]; }
  //! Get function return value.
  ASMJIT_INLINE const FuncInOut& getRet(uint32_t index = kFuncRetLo) const noexcept { return _rets[index]; }

  //! Get stack size needed for function arguments passed on the stack.
  ASMJIT_INLINE uint32_t getArgStackSize() const noexcept { return _argStackSize; }

  ASMJIT_INLINE uint32_t getUsedMask(uint32_t kind) const noexcept {
    ASMJIT_ASSERT(kind < CallConv::kNumRegKinds);
    return _usedMask[kind];
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  CallConv _callConv;                    //!< Calling convention.

  uint8_t _argCount;                     //!< Number of arguments.
  uint8_t _retCount;                     //!< Number of return values.

  uint32_t _usedMask[CallConv::kNumRegKinds];
  uint32_t _argStackSize;                //!< Stack arguments' size (aligned).

  //! Function arguments (LO & HI) mapped to physical registers and stack.
  FuncInOut _args[kFuncArgCountLoHi];
  //! Function return value(s).
  FuncInOut _rets[2];
};

//! \}

} // asmjit namespace

// [Api-End]
#include "../apiend.h"

// [Guard]
#endif // _ASMJIT_BASE_FUNC_H
