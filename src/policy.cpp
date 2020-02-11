#include <functional>
#include <iomanip>
#include <sstream>
#include <vector>
#define KDEXT_64BIT
#include <windows.h>
#include <wdbgexts.h>

#include "common.h"
#include "peimage.h"

const int gPointerSize = 8;

class SandboxPolicies {
  class OpCode {
    static const size_t kArgumentCount = 4;

    address_t base_;
    uint32_t id_, options_;
    int16_t param_;
    address_t args_[kArgumentCount];

  public:
    enum OpcodeID {
      OP_ALWAYS_FALSE,        // Evaluates to false (EVAL_FALSE).
      OP_ALWAYS_TRUE,         // Evaluates to true (EVAL_TRUE).
      OP_NUMBER_MATCH,        // Match a 32-bit integer as n == a.
      OP_NUMBER_MATCH_RANGE,  // Match a 32-bit integer as a <= n <= b.
      OP_NUMBER_AND_MATCH,    // Match using bitwise AND; as in: n & a != 0.
      OP_WSTRING_MATCH,       // Match a string for equality.
      OP_ACTION,              // Evaluates to an action opcode.

      OP_LAST,
    };

    static const char *GetName(uint32_t id) {
      static const char *Names[] = {
        "OP_ALWAYS_FALSE",
        "OP_ALWAYS_TRUE",
        "OP_NUMBER_MATCH",
        "OP_NUMBER_MATCH_RANGE",
        "OP_NUMBER_AND_MATCH",
        "OP_WSTRING_MATCH",
        "OP_ACTION",
        "N/A",
      };
      return Names[min(id, OP_LAST)];
    }

    OpCode(address_t base) : base_(base), id_{}, options_{}, param_{} {
      static const auto offsetId =
        get_field_offset("chrome_exe!sandbox::PolicyOpcode", "opcode_id");
      static const auto offsetParam =
        get_field_offset("chrome_exe!sandbox::PolicyOpcode", "parameter_");
      static const auto offsetOpt =
        get_field_offset("chrome_exe!sandbox::PolicyOpcode", "options_");
      static const auto offsetArgs =
        get_field_offset("chrome_exe!sandbox::PolicyOpcode", "arguments_");

      id_ = load_data<uint32_t>(base + offsetId);
      options_ = load_data<uint32_t>(base + offsetOpt);
      param_ = load_data<int16_t>(base + offsetParam);
      for (size_t i = 0; i < kArgumentCount; ++i) {
        args_[i] = load_pointer(base + offsetArgs + gPointerSize * i);
      }
    }

    static std::string LoadString(address_t addr, int32_t cch) {
      std::string ret;
      if (auto wbuf = new wchar_t[cch + 1]) {
        ULONG cb = 0;
        ReadMemory(addr, wbuf, cch * sizeof(wchar_t), &cb);
        wbuf[cch] = 0;

        if (auto buf = new char[cch + 1]) {
          int len = WideCharToMultiByte(
            CP_OEMCP, 0, wbuf, cch, buf, cch, nullptr, nullptr);
          buf[len] = 0;
          ret = buf;
        }

        delete [] wbuf;
      }
      return ret;
    }

    static std::string OpcodeOption(int val) {
      constexpr uint32_t kPolNegateEval = 1;
      constexpr uint32_t kPolClearContext = 2;
      constexpr uint32_t kPolUseOREval = 4;

      std::string s;
      if (val & kPolNegateEval) {
        s += "kPolNegateEval";
        val &= ~kPolNegateEval;
        if (val) s += '|';
      }
      if (val & kPolClearContext) {
        s += "kPolClearContext";
        val &= ~kPolClearContext;
        if (val) s += '|';
      }
      if (val & kPolUseOREval) {
        s += "kPolUseOREval";
        val &= ~kPolNegateEval;
        if (val) s += '|';
      }
      if (val || s.size() == 0) {
        s += std::to_string(val);
      }
      return s;
    }

    void Dump(std::ostream &s) const {
      s << GetName(id_) << " arg" << param_ << ' ' << OpcodeOption(options_);

      if (id_ == OP_WSTRING_MATCH) {
        constexpr int kSeekToEnd = 0xfffff;
        static const char *OptionStr[] = {
          "CASE_SENSITIVE",
          "",
          "EXACT_LENGTH|CASE_SENSITIVE",
          "EXACT_LENGTH",
        };

        auto match_str =
          LoadString(base_ + args_[0], static_cast<int32_t>(args_[1]));
        auto match_len = static_cast<int32_t>(args_[2]);
        auto match_opts = static_cast<uint32_t>(args_[3]);

        s << std::endl
          << "  match_str      = " << match_str << std::endl
          << "  match_len      = " << static_cast<int32_t>(args_[1])
          << std::endl
          << "  start_position = ";
        if (match_len == kSeekToEnd)
          s << "kSeekToEnd" << std::endl;
        else
          s << match_len << std::endl;
        s << "  match_opts     = " << match_opts;
        if (match_opts < 4) s << " " << OptionStr[match_opts];
        s << std::endl;
      }
      else {
        s << " ["
          << static_cast<uint32_t>(args_[0])
          << ' ' << static_cast<uint32_t>(args_[1])
          << ' ' << static_cast<uint32_t>(args_[2])
          << ' ' << static_cast<uint32_t>(args_[3])
          << ']' << std::endl;
      }
    }
  };

  static const char *GetName(uint32_t tag) {
    static const char *Names[] = {
      "UNUSED",
      "PING1",
      "PING2",
      "NTCREATEFILE",
      "NTOPENFILE",
      "NTQUERYATTRIBUTESFILE",
      "NTQUERYFULLATTRIBUTESFILE",
      "NTSETINFO_RENAME",
      "CREATENAMEDPIPEW",
      "NTOPENTHREAD",
      "NTOPENPROCESS",
      "NTOPENPROCESSTOKEN",
      "NTOPENPROCESSTOKENEX",
      "CREATEPROCESSW",
      "CREATEEVENT",
      "OPENEVENT",
      "NTCREATEKEY",
      "NTOPENKEY",
      "GDI_GDIDLLINITIALIZE",
      "GDI_GETSTOCKOBJECT",
      "USER_REGISTERCLASSW",
      "CREATETHREAD",
      "USER_ENUMDISPLAYMONITORS",
      "USER_ENUMDISPLAYDEVICES",
      "USER_GETMONITORINFO",
      "GDI_CREATEOPMPROTECTEDOUTPUTS",
      "GDI_GETCERTIFICATE",
      "GDI_GETCERTIFICATESIZE",
      "GDI_DESTROYOPMPROTECTEDOUTPUT",
      "GDI_CONFIGUREOPMPROTECTEDOUTPUT",
      "GDI_GETOPMINFORMATION",
      "GDI_GETOPMRANDOMNUMBER",
      "GDI_GETSUGGESTEDOPMPROTECTEDOUTPUTARRAYSIZE",
      "GDI_SETOPMSIGNINGKEYANDSEQUENCENUMBERS",
      "NTCREATESECTION",
      "LAST",
    };
    return Names[tag];
  }


  class Policy {
    uint32_t num_opcodes_;
    address_t policies_;
    uint32_t policy_size_;

  public:
    Policy(address_t base) : num_opcodes_(0) {
      static const auto opcodesField =
        get_field_info("chrome_exe!sandbox::PolicyBuffer", "opcodes");

      num_opcodes_ = load_data<uint32_t>(base);
      policies_ = base + opcodesField.FieldOffset;
      policy_size_ = opcodesField.size;
    }

    operator bool() const {
      return num_opcodes_ > 0;
    }

    void Dump(std::ostream &s) const {
      for (uint32_t i = 0; i < num_opcodes_; ++i) {
        address_t opcode_base = policies_ + i * policy_size_;
        OpCode opcode(opcode_base);
        s << '[' << i << "] @ " <<address_string(opcode_base) << ' ';
        opcode.Dump(s);
      }
    }
  };

  address_t base_;
  uint32_t num_items_;
  uint32_t data_size_;

public:
  SandboxPolicies(address_t addr) : base_(addr), num_items_{}, data_size_{} {
    static const auto offsetDataSize =
      get_field_offset("chrome_exe!sandbox::PolicyGlobal", "data_size");
    num_items_ = offsetDataSize / gPointerSize;
    data_size_ = load_data<uint32_t>(base_ + offsetDataSize);
  }

  operator bool() const {
    return !!base_;
  }

  void Dump(uint32_t index) const {
    if (index >= num_items_) return;

    auto policy_buffer_start =
      base_ + load_data<uint32_t>(base_ + index * gPointerSize);

    Policy policy(policy_buffer_start);
    if (!policy) return;

    std::stringstream s;
    policy.Dump(s);
    dprintf("\nPolicy.%s\n%s",
            GetName(index),
            s.str().c_str());
  }

  void DumpAll() const {
    for (uint32_t i = 0; i < num_items_; ++i) {
      Dump(i);
    }
  }
};

DECLARE_API(pol) {
  static address_t policy_addr =
    GetExpression("chrome_exe!g_shared_policy_memory");

  auto policy_base = load_pointer(policy_addr);

  address_string s1(policy_addr), s2(policy_base);
  dprintf("chrome_exe!g_shared_policy_memory = *%s = %s\n",
          s1, s2);

  if (!policy_base) return;

  SandboxPolicies policies(policy_base);
  const auto vargs = get_args(args);
  if (vargs.size() == 0)
    policies.DumpAll();
  else
    policies.Dump(static_cast<uint32_t>(GetExpression(vargs[0].c_str())));
}
