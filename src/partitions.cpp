#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <vector>
#include <memory>
#define KDEXT_64BIT
#include <wdbgexts.h>
#include "common.h"

namespace base {

constexpr size_t kBitsPerSizeT = 64;
constexpr size_t kGenericMinBucketedOrder = 4;
constexpr size_t kGenericMaxBucketedOrder = 20;
constexpr size_t kGenericNumBucketedOrders =
    (kGenericMaxBucketedOrder - kGenericMinBucketedOrder) + 1;
constexpr size_t kGenericNumBucketsPerOrder = 8;
constexpr size_t kGenericSmallestBucket = 1 << (kGenericMinBucketedOrder - 1);
constexpr size_t kSystemPageSize = 4096;
constexpr size_t kPartitionPageShift = 14;
constexpr size_t kPageMetadataShift = 5;
constexpr size_t kSuperPageShift = 21;
constexpr size_t kSuperPageSize = 1 << kSuperPageShift;
constexpr size_t kSuperPageOffsetMask = kSuperPageSize - 1;
constexpr size_t kSuperPageBaseMask = ~kSuperPageOffsetMask;

class Global : Object {
  static COREADDR g_sentinel_page;
  static COREADDR g_sentinel_bucket;
public:
  static COREADDR get_sentinel_page() {
    if (!g_sentinel_page)
      g_sentinel_page = get_expression("!g_sentinel_page");
    return g_sentinel_page;
  }
  static COREADDR get_sentinel_bucket() {
    if (!g_sentinel_bucket)
      g_sentinel_bucket = get_expression("!g_sentinel_bucket");
    return g_sentinel_bucket;
  }
};

COREADDR Global::g_sentinel_page = 0;
COREADDR Global::g_sentinel_bucket = 0;

class PartitionSuperPageExtentEntry : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

public:
  COREADDR super_page_base;
  COREADDR super_pages_end;
  COREADDR next;

  PartitionSuperPageExtentEntry()
    : super_page_base(0),
      super_pages_end(0),
      next(0)
  {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!base::PartitionSuperPageExtentEntry";

      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("super_page_base");
      LOAD_FIELD_OFFSET("super_pages_end");
      LOAD_FIELD_OFFSET("next");
    }

    addr_ = addr;
    if (addr) {
      char buf1[20];
      COREADDR src = 0;

      src = addr + offsets_["super_page_base"];
      LOAD_MEMBER_POINTER(super_page_base);
      src = addr + offsets_["super_pages_end"];
      LOAD_MEMBER_POINTER(super_pages_end);
      src = addr + offsets_["next"];
      LOAD_MEMBER_POINTER(next);
    }
    else {
      super_page_base = super_pages_end = next = 0;
    }
  }

  void dump(std::ostream &s) const {
    char buf1[20];
    char buf2[20];
    auto num_pages = static_cast<int>((super_pages_end - super_page_base) >> 21);
    s << ptos(super_page_base, buf1, sizeof(buf1))
      << '-'
      << ptos(super_pages_end, buf2, sizeof(buf2))
      << " (" << num_pages << ")";
  }
};

class PartitionFreelistEntry : public Object {
private:
  static COREADDR Transform(COREADDR ptr) {
    if (target().is64bit_)
      return _byteswap_uint64(ptr);
    else
      return _byteswap_ulong(static_cast<uint32_t>(ptr));
  }

  static std::map<std::string, ULONG> offsets_;

  COREADDR next;

public:
  PartitionFreelistEntry() : next(0) {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!base::PartitionFreelistEntry";

      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("next");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr) {
      src = addr + offsets_["next"];
      LOAD_MEMBER_POINTER(next);
    }
  }

  void dump(std::ostream &s) const;

  COREADDR next_transformed() const {
    return Transform(next);
  }
};

class PartitionPage : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  COREADDR freelist_head;
  COREADDR next_page;
  COREADDR bucket;
  int16_t num_allocated_slots;
  uint16_t num_unprovisioned_slots;
  uint16_t page_offset;
  int16_t empty_cache_index;

public:
  PartitionPage()
    : freelist_head(0),
      next_page(0),
      bucket(0),
      num_allocated_slots(0),
      num_unprovisioned_slots(0),
      page_offset(0),
      empty_cache_index(0)
  {}

  uint16_t get_page_offset() const { return page_offset; }

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!base::PartitionPage";

      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("freelist_head");
      LOAD_FIELD_OFFSET("next_page");
      LOAD_FIELD_OFFSET("bucket");
      LOAD_FIELD_OFFSET("num_allocated_slots");
      LOAD_FIELD_OFFSET("num_unprovisioned_slots");
      LOAD_FIELD_OFFSET("page_offset");
      LOAD_FIELD_OFFSET("empty_cache_index");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr_) {
      src = addr_ + offsets_["freelist_head"];
      LOAD_MEMBER_POINTER(freelist_head);
      src = addr_ + offsets_["next_page"];
      LOAD_MEMBER_POINTER(next_page);
      src = addr_ + offsets_["bucket"];
      LOAD_MEMBER_POINTER(bucket);
      src = addr_ + offsets_["num_allocated_slots"];
      LOAD_MEMBER_VALUE(num_allocated_slots);
      src = addr_ + offsets_["num_unprovisioned_slots"];
      LOAD_MEMBER_VALUE(num_unprovisioned_slots);
      src = addr_ + offsets_["page_offset"];
      LOAD_MEMBER_VALUE(page_offset);
      src = addr_ + offsets_["empty_cache_index"];
      LOAD_MEMBER_VALUE(empty_cache_index);
    }
    else {
      freelist_head = next_page = bucket = num_allocated_slots
        = num_unprovisioned_slots = page_offset = empty_cache_index = 0;
    }
  }

  void dump(std::ostream &s) const;
  void dump_pages_for_superpage(std::ostream &s) const;

  void dump_pages(std::ostream &s) const {
    if (addr_ == Global::get_sentinel_page()) {
      s << "g_sentinel_page\n";
    }
    else {
      char buf1[20];
      PartitionPage pp;
      for (COREADDR p = addr_; p; p = pp.next_page) {
        pp.load(p);
        if (p != addr_) s << "                                 ";
        s << ptos(p, buf1, sizeof(buf1))
          << ' ' << pp.num_allocated_slots
          << (num_unprovisioned_slots > 0 ? "!\n" : "\n");
      }
    }
  }

  void dump_pages_for_bucket(std::ostream &s) const {
    if (addr_ == Global::get_sentinel_page()) {
      s << "g_sentinel_page\n";
    }
    else {
      char buf1[20];
      char buf2[20];
      PartitionPage pp;
      for (COREADDR p = addr_; p; p = pp.next_page) {
        COREADDR super_page_ptr = p & kSuperPageBaseMask;
        uint32_t partition_page_index = static_cast<uint32_t>(
          (p - super_page_ptr - kSystemPageSize) >> kPageMetadataShift);
        pp.load(p);
        s << ptos(super_page_ptr, buf1, sizeof(buf1))
          << " #" << std::setiosflags(std::ios::left) << std::setw(5)
          << partition_page_index
          << ptos(p, buf2, sizeof(buf2))
          << ' ' << pp.num_allocated_slots << std::endl;
      }
    }
  }
};

struct SuperPage {
  COREADDR base_ = 0;

  void load(COREADDR addr) {
    base_ = addr & kSuperPageBaseMask;
  }

  void dump(std::ostream &s) const {
    char buf1[20];
    constexpr COREADDR ppage_size = 1 << kPartitionPageShift;
    const COREADDR metadata_base = base_ + kSystemPageSize;
    int index = 0;
    for (COREADDR p = base_ + ppage_size;
         p < base_ + kSuperPageSize;
         p += ppage_size) {

      PartitionPage metadata;
      metadata.load(metadata_base + ((++index) << kPageMetadataShift));

      s << std::dec << std::setfill(' ') << std::setw(3) << index
        << ' ' << ptos(p, buf1, sizeof(buf1));

      ULONG dummy, cb;
      if (!ReadMemory(p, &dummy, sizeof(dummy), &cb))
        s << " x ";
      else
        s << "   ";

      metadata.dump_pages_for_superpage(s);
      s << std::endl;
    }
  }
};

void PartitionFreelistEntry::dump(std::ostream &s) const {
  COREADDR super_page_ptr = addr_ & kSuperPageBaseMask;
  uint32_t partition_page_index
    = static_cast<uint32_t>((addr_ & kSuperPageOffsetMask) >> kPartitionPageShift);
  COREADDR page = super_page_ptr
                  + kSystemPageSize
                  + (partition_page_index << kPageMetadataShift);
  PartitionPage pp;
  pp.load(page);
  uint32_t delta = pp.get_page_offset() << kPageMetadataShift;
  char buf1[20];
  s << "SuperPage " << ptos(super_page_ptr, buf1, sizeof(buf1))
    << " #" << partition_page_index;
  if (pp.get_page_offset())
    s << '-' <<  pp.get_page_offset();
  s << "\nbase::PartitionPage " << ptos(page - delta, buf1, sizeof(buf1))
    << std::endl;
}

class PartitionBucket : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  PartitionPage active_pages_head;
  uint32_t slot_size;
  uint8_t num_system_pages_per_slot_span;

public:
  PartitionBucket()
    : slot_size(0),
      num_system_pages_per_slot_span(0) {}

  bool is_valid() const {
    return slot_size % kGenericSmallestBucket == 0;
  }

  size_t get_bytes_per_span() const {
    return num_system_pages_per_slot_span * kSystemPageSize;
  }
  uint16_t get_slots_per_span() const {
    return static_cast<uint16_t>(get_bytes_per_span() / slot_size);
  }
  uint32_t get_slot_size() const {
    return slot_size;
  }

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!base::PartitionBucket";

      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("active_pages_head");
      LOAD_FIELD_OFFSET("slot_size");
      LOAD_FIELD_OFFSET("num_system_pages_per_slot_span");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr) {
      src = addr_ + offsets_["active_pages_head"];
      LOAD_MEMBER_POINTER(addr);
      active_pages_head.load(addr);
      src = addr_ + offsets_["slot_size"];
      LOAD_MEMBER_VALUE(slot_size);
      src = addr_ + offsets_["num_system_pages_per_slot_span"];
      LOAD_MEMBER_VALUE(num_system_pages_per_slot_span);
    }
    else {
      active_pages_head.load(0);
      slot_size = num_system_pages_per_slot_span = 0;
    }
  }

  void dump_pages(std::ostream &s) const {
    s << std::hex << std::setfill(' ') << std::setw(5) << slot_size
      << std::dec << std::setfill(' ') << std::setw(4)
      << static_cast<uint32_t>(num_system_pages_per_slot_span)
      << ' ';
    active_pages_head.dump_pages(s);
  }

  void dump(std::ostream &s) const {
    s << "slot_size (bytes):              0x" << std::hex << slot_size << std::endl
      << "num_system_pages_per_slot_span: 0n" << std::dec
      << static_cast<uint32_t>(num_system_pages_per_slot_span) << std::endl
      << "total slots per span:           0n" << get_slots_per_span() << std::endl
      << "active pages / num_allocated_slots:" << std::endl;
    active_pages_head.dump_pages_for_bucket(s);
  }
};

void PartitionPage::dump(std::ostream &s) const {
  if (addr_ == Global::get_sentinel_page()) {
    s << "g_sentinel_page\n";
    return;
  }

  char buf1[20];
  if (page_offset) {
    COREADDR meta = addr() - (page_offset << kPageMetadataShift);
    s << "Resolving page_offset=" << page_offset
      << " -> " << ptos(meta, buf1, sizeof(buf1))
      << std::endl;
    PartitionPage ppage;
    ppage.load(meta);
    ppage.dump(s);
  }
  else {
    PartitionBucket b;
    b.load(bucket);
    s << "bucket:                  " << ptos(bucket, buf1, sizeof(buf1))
      << " (total slots=" << b.get_slots_per_span() << ')' << std::endl
      << "num_allocated_slots:     " << num_allocated_slots << std::endl
      << "num_unprovisioned_slots: " << num_unprovisioned_slots << std::endl
      << "free slots (";

    std::stringstream ss;
    PartitionFreelistEntry next_entry;
    int i = 0;
    for (COREADDR p = freelist_head;
         p;
         next_entry.load(p), p = next_entry.next_transformed(), ++i) {
      if (i & 3) ss << ' ';
      ss << ptos(p, buf1, sizeof(buf1));
      if ((i & 3) == 3) ss << std::endl;
    }
    s << i << "):" << std::endl << ss.str() << std::endl;
  }
}

void PartitionPage::dump_pages_for_superpage(std::ostream &s) const {
  char buf1[20];
  char buf2[20];
  s << ptos(addr(), buf1, sizeof(buf1))
    << std::dec << std::setfill(' ') << std::setw(6) << num_allocated_slots
    << std::setw(6) << num_unprovisioned_slots
    << std::setw(2) << page_offset
    << std::setw(3) << empty_cache_index;
  if (bucket) {
    PartitionBucket b;
    b.load(bucket);
    s << ' ' << ptos(bucket, buf2, sizeof(buf2))
      << " (slot=0x" << std::hex << b.get_slot_size() << ')';
  }
}

class PartitionRootBase : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

protected:
  COREADDR first_extent;

public:
  PartitionRootBase() {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!base::PartitionRootBase";

      ULONG offset = 0;
      const char *field_name = nullptr;
      LOAD_FIELD_OFFSET("first_extent");
    }

    char buf1[20];
    COREADDR src = 0;

    addr_ = addr;
    if (addr) {
      src = addr_ + offsets_["first_extent"];
      LOAD_MEMBER_POINTER(first_extent);
    }
    else {
      first_extent = 0;
    }
  }

  virtual void dump(std::ostream &s) const {
    if (!first_extent)
      s << "no extents\n";
    else {
      s << "extent(s): ";
      PartitionSuperPageExtentEntry extent;
      for (auto p = first_extent; p; p = extent.next) {
        extent.load(p);
        if (p != first_extent) s << "          ";
        extent.dump(s);
        s << std::endl;
      }
    }
  }
};

class PartitionRoot : public PartitionRootBase {};

class PartitionRootGeneric : public PartitionRootBase {
private:
  static std::map<std::string, ULONG> offsets_;
  static int bucket_size_;
  static int bucket_count_;
  static int lookup_count_;

public:
  PartitionRootGeneric() {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!base::PartitionRootGeneric";

      FIELD_INFO field{};
      field = GetFieldInfo(type.c_str(), "buckets[0]");
      bucket_size_ = field.size;
      field = GetFieldInfo(type.c_str(), "buckets");
      offsets_["buckets"] = field.FieldOffset;
      bucket_count_ = field.size / bucket_size_;
      if (bucket_count_ != kGenericNumBucketedOrders * kGenericNumBucketsPerOrder) {
        dprintf("Bucket size mismatch.  Update the constants accordingly.\n");
        return;
      }

      const int pointer_size = target().is64bit_ ? 8 : 4;
      field = GetFieldInfo(type.c_str(), "bucket_lookups");
      lookup_count_ = field.size / pointer_size;
      offsets_["bucket_lookups"] = field.FieldOffset;
    }

    PartitionRootBase::load(addr);
  }

  void dump_bucket_lookup_table(std::ostream &s) const {
    auto lookup_head = addr_ + offsets_["bucket_lookups"];
    auto bucket_head = addr_ + offsets_["buckets"];

    const int pointer_size = target().is64bit_ ? 8 : 4;
    int order = 0;
    for (int i = 0;
         i < kGenericNumBucketsPerOrder * (kGenericMaxBucketedOrder + 1);
         ++i) {
      if (i % kGenericNumBucketsPerOrder == 0)
        s << std::setfill(' ') << std::setw(4) << order << ' ';

      COREADDR item = deref(lookup_head + pointer_size * i);
      s << std::setfill(' ') << std::setw(5)
        << ((item - bucket_head) / bucket_size_);

      if ((i + 1) % kGenericNumBucketsPerOrder == 0) {
        s << std::endl;
        ++order;
      }
    }
    s << std::endl;
  }

  void dump_buckets(std::ostream &s) const {
    auto bucket_head = addr_ + offsets_["buckets"];
    for (int i = 0; i < bucket_count_; ++i) {
      PartitionBucket bucket;
      bucket.load(bucket_head + bucket_size_ * i);
      if (bucket.is_valid()) {
        char buf1[20];
        s << std::dec << std::setfill(' ') << std::setw(4) << i
          << ' ' << ptos(bucket.addr(), buf1, sizeof(buf1))
          << ' ';
        bucket.dump_pages(s);
      }
    }
    s << std::endl;
  }
};

class PartitionAllocatorGeneric : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  PartitionRootGeneric partition_root_;

public:
  static const char type_name[];

  PartitionAllocatorGeneric() {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!base::PartitionAllocatorGeneric";

      ULONG offset = 0;
      const char *field_name = nullptr;

      LOAD_FIELD_OFFSET("partition_root_");
    }

    addr_ = addr;
    if (addr) {
      partition_root_.load(addr + offsets_["partition_root_"]);
    }
  }

  void dump(std::ostream &s) const {
    partition_root_.dump(s);
  }

  void dump_buckets(std::ostream &s) const {
    //partition_root_.dump_bucket_lookup_table(s);
    partition_root_.dump_buckets(s);
  }
};

class SizeSpecificPartitionAllocator1024 : public Object {
private:
  static std::map<std::string, ULONG> offsets_;

  static int bucket_size_;
  static int bucket_count_;

  PartitionRoot partition_root_;

public:
  static const char type_name[];

  SizeSpecificPartitionAllocator1024() {}

  virtual void load(COREADDR addr) {
    if (offsets_.size() == 0) {
      std::string type = target().engine();
      type += "!base::SizeSpecificPartitionAllocator<1024>";

      FIELD_INFO field{};
      field = GetFieldInfo(type.c_str(), "actual_buckets_[0]");
      bucket_size_ = field.size;
      field = GetFieldInfo(type.c_str(), "actual_buckets_");
      offsets_["actual_buckets_"] = field.FieldOffset;
      bucket_count_ = field.size / bucket_size_;

      ULONG offset = 0;
      const char *field_name = nullptr;

      LOAD_FIELD_OFFSET("partition_root_");
    }

    addr_ = addr;
    if (addr) {
      partition_root_.load(addr + offsets_["partition_root_"]);
    }
  }

  void dump(std::ostream &s) const {
    partition_root_.dump(s);
  }

  void dump_buckets(std::ostream &s) const {
    auto bucket_head = addr_ + offsets_["actual_buckets_"];
    for (int i = 0; i < bucket_count_; ++i) {
      PartitionBucket bucket;
      bucket.load(bucket_head + bucket_size_ * i);
      if (bucket.is_valid()) {
        char buf1[20];
        s << std::dec << std::setfill(' ') << std::setw(4) << i
          << ' ' << ptos(bucket.addr(), buf1, sizeof(buf1))
          << ' ';
        bucket.dump_pages(s);
      }
    }
    s << std::endl;
  }
};

int PartitionRootGeneric::bucket_size_ = 0;
int PartitionRootGeneric::bucket_count_ = 0;
int PartitionRootGeneric::lookup_count_ = 0;
int SizeSpecificPartitionAllocator1024::bucket_size_ = 0;
int SizeSpecificPartitionAllocator1024::bucket_count_ = 0;
std::map<std::string, ULONG> PartitionSuperPageExtentEntry::offsets_;
std::map<std::string, ULONG> PartitionFreelistEntry::offsets_;
std::map<std::string, ULONG> PartitionPage::offsets_;
std::map<std::string, ULONG> PartitionBucket::offsets_;
std::map<std::string, ULONG> PartitionRootBase::offsets_;
std::map<std::string, ULONG> PartitionRootGeneric::offsets_;
std::map<std::string, ULONG> PartitionAllocatorGeneric::offsets_;
std::map<std::string, ULONG> SizeSpecificPartitionAllocator1024::offsets_;
const char PartitionAllocatorGeneric::type_name[]
  = "base::PartitionAllocatorGeneric";
const char SizeSpecificPartitionAllocator1024::type_name[]
  = "base::SizeSpecificPartitionAllocator<1024>";

}

namespace WTF {
  namespace Partitions {
    COREADDR fast_malloc_allocator_ = 0;
    COREADDR array_buffer_allocator_ = 0;
    COREADDR buffer_allocator_ = 0;
    COREADDR layout_allocator_ = 0;
  }
}

template <typename ALLOC_TYPE>
void dump_allocator(PCSTR args,
                    COREADDR &address_store,
                    const std::string &bang_and_symbol) {
  Object global_initializer;

  bool use_args = false;
  bool dump_buckets = false;
  COREADDR allocator_addr = 0;

  char args_copy[1024];
  if (args && strcpy_s(args_copy, sizeof(args_copy), args) == 0) {
    char *token = strtok(args_copy, " ");
    while(token) {
      if (strcmp(token, "-buckets") == 0)
        dump_buckets = true;
      else {
        allocator_addr = GetExpression(token);
        use_args = true;
      }
      token = strtok(nullptr, " ");
    }
  }

  if (!use_args) {
    if (!address_store)
      address_store = Object::get_expression(bang_and_symbol.c_str());
    allocator_addr = Object::deref(address_store);
  }

  char buf1[20];
  char buf2[20];
  std::stringstream s;
  s << bang_and_symbol.substr(1)
    << ' ' << ptos(address_store, buf1, sizeof(buf1))
    << " -> " << ALLOC_TYPE::type_name <<' '
    << ptos(allocator_addr, buf2, sizeof(buf2))
    << std::endl;

  ALLOC_TYPE allocator;
  allocator.load(allocator_addr);
  allocator.dump(s);
  if (dump_buckets) allocator.dump_buckets(s);

  dprintf(s.str().c_str());
}

DECLARE_API(fast_malloc_allocator) {
  dump_allocator<base::PartitionAllocatorGeneric>(
    args,
    WTF::Partitions::fast_malloc_allocator_,
    std::string("!WTF::Partitions::fast_malloc_allocator_"));
}

DECLARE_API(array_buffer_allocator) {
  dump_allocator<base::PartitionAllocatorGeneric>(
    args,
    WTF::Partitions::array_buffer_allocator_,
    std::string("!WTF::Partitions::array_buffer_allocator_"));
}

DECLARE_API(buffer_allocator) {
  dump_allocator<base::PartitionAllocatorGeneric>(
    args,
    WTF::Partitions::buffer_allocator_,
    std::string("!WTF::Partitions::buffer_allocator_"));
}

DECLARE_API(layout_allocator) {
  dump_allocator<base::SizeSpecificPartitionAllocator1024>(
    args,
    WTF::Partitions::layout_allocator_,
    std::string("!WTF::Partitions::layout_allocator_"));
}

template <typename T>
void dump_arg(PCSTR args) {
  const char delim[] = " ";
  char args_copy[1024];
  if (args && strcpy_s(args_copy, sizeof(args_copy), args) == 0) {
    char *next_token = nullptr;
    if (auto token = strtok_s(args_copy, delim, &next_token)) {
      T t;
      t.load(GetExpression(token));
      std::stringstream s;
      t.dump(s);
      dprintf(s.str().c_str());
    }
  }
}

DECLARE_API(page) {
  dump_arg<base::PartitionPage>(args);
}

DECLARE_API(spage) {
  Object global_initializer;
  dump_arg<base::SuperPage>(args);
}

DECLARE_API(slot) {
  dump_arg<base::PartitionFreelistEntry>(args);
}

DECLARE_API(bucket) {
  dump_arg<base::PartitionBucket>(args);
}
