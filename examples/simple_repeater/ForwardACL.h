#pragma once

// Forwarding ACL (blocklist) for the repeater.
//
// Rules stop this repeater from RE-TRANSMITTING matching packets. Local processing
// (e.g. this node being the destination) is never affected; only forwarding is suppressed.
//
// This file and ForwardACL.cpp only depend on Packet.h, so the matching, CLI parsing and
// (de)serialisation logic can be unit-tested natively without radio hardware or a filesystem.

#include <stdint.h>
#include <stddef.h>
#include <Packet.h>

#ifndef FWD_ACL_MAX_ENTRIES
  #define FWD_ACL_MAX_ENTRIES   32
#endif

#define FWD_ACL_FILE          "/fwd_acl"
#define FWD_ACL_LIST_PAGE     5      // entries per 'acl list' reply (fits the 160 byte CLI reply buffer)

#define ACL_FLAG_SRC_ANY      0x01   // src is wildcard
#define ACL_FLAG_DST_ANY      0x02   // dst is wildcard

struct AclEntry {
  uint8_t hop[2];
  uint8_t hop_len;   // 0 = wildcard, 1 or 2
  uint8_t src;
  uint8_t dst;
  uint8_t flags;     // bit0: src is wildcard, bit1: dst is wildcard
};

// Pure match function: true if every non-wildcard field of 'e' matches 'pkt'.
bool aclMatches(const AclEntry& e, const mesh::Packet* pkt);

class ForwardACL {
  AclEntry _entries[FWD_ACL_MAX_ENTRIES];
  uint32_t _hits[FWD_ACL_MAX_ENTRIES];   // RAM only, never persisted (avoids flash wear)
  uint8_t  _count;
  bool     _enabled;

public:
  // serialised format: magic(4) | version(1) | enabled(1) | count(1) | count * entry(6)
  static const size_t HEADER_SIZE = 7;
  static const size_t ENTRY_SIZE = 6;
  static const size_t MAX_SERIALISED_SIZE = HEADER_SIZE + ENTRY_SIZE * FWD_ACL_MAX_ENTRIES;

  ForwardACL() { reset(); }

  void reset();  // empty + disabled

  bool isEnabled() const { return _enabled; }
  void setEnabled(bool en) { _enabled = en; }
  int  getCount() const { return _count; }
  const AclEntry* getEntry(int idx) const { return (idx >= 0 && idx < _count) ? &_entries[idx] : NULL; }
  uint32_t getHits(int idx) const { return (idx >= 0 && idx < _count) ? _hits[idx] : 0; }
  uint32_t getTotalHits() const;
  void resetHits();

  // returns 0-based index of first matching rule, or -1 (ignores the enabled switch)
  int findMatch(const mesh::Packet* pkt) const;

  // true if the packet must NOT be forwarded (ACL enabled and a rule matched). Counts the hit.
  bool shouldBlock(const mesh::Packet* pkt);

  // returns new 1-based index, or one of the negative ADD_ERR_ codes
  enum { ADD_ERR_FULL = -1, ADD_ERR_DUPLICATE = -2, ADD_ERR_INVALID = -3 };
  int add(const AclEntry& e);
  bool remove(int idx1);   // 1-based, compacts the list
  void clear() { _count = 0; }

  size_t serialise(uint8_t* buf, size_t buf_len) const;   // returns bytes written, 0 if buf too small
  bool deserialise(const uint8_t* buf, size_t len);        // on any error: reset() and return false

  // 'args' is the text after "acl" (may be empty, max 63 chars). Writes a reply (< 150 bytes).
  // Returns true if persisted state changed, i.e. the caller must save.
  bool handleCommand(const char* args, char* reply);

  static bool parseEntry(const char* hop, const char* src, const char* dst, AclEntry& out);
  static void formatEntry(const AclEntry& e, char* dest);  // e.g. "c8/08/7d", "c8f2/*/7d"
};
