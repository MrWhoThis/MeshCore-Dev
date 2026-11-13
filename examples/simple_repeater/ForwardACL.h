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
#define FWD_ACL_DEFAULTS_FILE "/fwd_acl_def"   // fingerprint of the last applied FWD_ACL_DEFAULTS
#define FWD_ACL_LIST_PAGE     4      // entries per 'acl list' reply (fits the 160 byte CLI reply buffer)
#define FWD_ACL_MAX_HOP_LEN   3      // path hashes are 1..3 bytes (size 4 is reserved)

#define ACL_FLAG_SRC_ANY      0x01   // src is wildcard (always set for ACL_KIND_GRP)
#define ACL_FLAG_DST_ANY      0x02   // dst (or channel, for ACL_KIND_GRP) is wildcard
#define ACL_FLAG_HOP_ZERO     0x04   // hop '0': flood packet heard straight from its originator (empty path)

#define ACL_KIND_DM           0      // REQ / RESPONSE / TXT_MSG / PATH: match on dst/src hashes
#define ACL_KIND_GRP          1      // GRP_TXT / GRP_DATA: 'dst' holds the channel hash, 'src' unused

struct AclEntry {
  uint8_t hop[FWD_ACL_MAX_HOP_LEN];
  uint8_t hop_len;   // 0 = wildcard (or zero-hop, see ACL_FLAG_HOP_ZERO), 1..3 = prefix of first path hash
  uint8_t kind;      // ACL_KIND_*
  uint8_t src;
  uint8_t dst;       // dest hash, or channel hash for ACL_KIND_GRP
  uint8_t flags;     // ACL_FLAG_*
};

// Pure match function: true if every non-wildcard field of 'e' matches 'pkt'.
bool aclMatches(const AclEntry& e, const mesh::Packet* pkt);

class ForwardACL {
  AclEntry _entries[FWD_ACL_MAX_ENTRIES];
  uint32_t _hits[FWD_ACL_MAX_ENTRIES];   // RAM only, never persisted (avoids flash wear)
  uint8_t  _count;
  bool     _enabled;

public:
  // serialised format: magic(4) | version(1) | enabled(1) | count(1) | count * entry
  //   v2 entry (8): hop[3] | hop_len | kind | src | dst | flags   (written)
  //   v1 entry (6): hop[2] | hop_len | src | dst | flags          (read only, loaded as ACL_KIND_DM)
  static const size_t HEADER_SIZE = 7;
  static const size_t ENTRY_SIZE = 8;
  static const size_t ENTRY_SIZE_V1 = 6;
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

  // Seeds rules from a build-time spec like "c8/08/7d,c8f2/*/11,0/#11" (separators: ',' ';' or space).
  // DM rules are "hop/src/dst"; group rules are "hop/#chan" (same form 'acl list' prints).
  // Replaces any existing rules and enables the ACL. Returns number of rules added, or -1 on a
  // parse/add error (in which case the ACL is left empty and disabled).
  int loadDefaults(const char* spec);

  // FNV-1a fingerprint of a defaults spec, used to detect a firmware with a different rule list.
  static uint32_t specHash(const char* spec);

  static bool parseEntry(const char* hop, const char* src, const char* dst, AclEntry& out);
  static bool parseGroupEntry(const char* hop, const char* chan, AclEntry& out);
  // e.g. "c8/08/7d", "c8f2a1/*/7d", "0/08/*", "grp c8/#11", "grp 0/#*". 'dest' needs FORMAT_MAX bytes.
  static const size_t FORMAT_MAX = 20;
  static void formatEntry(const AclEntry& e, char* dest);
};
