#include "ForwardACL.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const uint8_t FWD_ACL_MAGIC[4] = { 'F', 'A', 'C', 'L' };
#define FWD_ACL_FILE_VERSION  1

static bool isWildcardAll(const AclEntry& e) {
  return e.hop_len == 0 && (e.flags & ACL_FLAG_SRC_ANY) && (e.flags & ACL_FLAG_DST_ANY);
}

static bool isValidEntry(const AclEntry& e) {
  if (e.hop_len > 2) return false;
  if (e.flags & ~(ACL_FLAG_SRC_ANY | ACL_FLAG_DST_ANY)) return false;
  return !isWildcardAll(e);
}

bool aclMatches(const AclEntry& e, const mesh::Packet* pkt) {
  // only payload types with cleartext dest/src hashes at payload[0] / payload[1]
  switch (pkt->getPayloadType()) {
    case PAYLOAD_TYPE_REQ:
    case PAYLOAD_TYPE_RESPONSE:
    case PAYLOAD_TYPE_TXT_MSG:
    case PAYLOAD_TYPE_PATH:
      break;
    default:
      return false;
  }
  if (pkt->getPayloadVer() != PAYLOAD_VER_1) return false;   // future versions may change hash layout
  if (pkt->payload_len < 2) return false;
  if (isWildcardAll(e)) return false;   // never block everything (defensive, CLI rejects these)

  if (!(e.flags & ACL_FLAG_DST_ANY) && pkt->payload[0] != e.dst) return false;
  if (!(e.flags & ACL_FLAG_SRC_ANY) && pkt->payload[1] != e.src) return false;

  if (e.hop_len > 0) {
    // for DIRECT packets 'path' is the route ahead, not behind, so a hop rule can't apply
    if (!pkt->isRouteFlood()) return false;
    if (pkt->getPathHashCount() == 0) return false;   // straight from originator, no first hop
    uint8_t n = pkt->getPathHashSize();
    if (e.hop_len < n) n = e.hop_len;
    if (memcmp(pkt->path, e.hop, n) != 0) return false;   // prefix match on the first path entry
  }
  return true;
}

void ForwardACL::reset() {
  _count = 0;
  _enabled = false;
  memset(_entries, 0, sizeof(_entries));
  memset(_hits, 0, sizeof(_hits));
}

uint32_t ForwardACL::getTotalHits() const {
  uint32_t total = 0;
  for (int i = 0; i < _count; i++) total += _hits[i];
  return total;
}

void ForwardACL::resetHits() {
  memset(_hits, 0, sizeof(_hits));
}

int ForwardACL::findMatch(const mesh::Packet* pkt) const {
  for (int i = 0; i < _count; i++) {
    if (aclMatches(_entries[i], pkt)) return i;
  }
  return -1;
}

bool ForwardACL::shouldBlock(const mesh::Packet* pkt) {
  if (!_enabled || _count == 0) return false;
  int i = findMatch(pkt);
  if (i < 0) return false;
  _hits[i]++;
  return true;
}

static bool sameEntry(const AclEntry& a, const AclEntry& b) {
  if (a.hop_len != b.hop_len || a.flags != b.flags) return false;
  if (a.hop_len > 0 && memcmp(a.hop, b.hop, a.hop_len) != 0) return false;
  if (!(a.flags & ACL_FLAG_SRC_ANY) && a.src != b.src) return false;
  if (!(a.flags & ACL_FLAG_DST_ANY) && a.dst != b.dst) return false;
  return true;
}

int ForwardACL::add(const AclEntry& e) {
  if (!isValidEntry(e)) return ADD_ERR_INVALID;
  for (int i = 0; i < _count; i++) {
    if (sameEntry(_entries[i], e)) return ADD_ERR_DUPLICATE;
  }
  if (_count >= FWD_ACL_MAX_ENTRIES) return ADD_ERR_FULL;
  _entries[_count] = e;
  _hits[_count] = 0;
  _count++;
  return _count;
}

bool ForwardACL::remove(int idx1) {
  if (idx1 < 1 || idx1 > _count) return false;
  for (int i = idx1 - 1; i < _count - 1; i++) {
    _entries[i] = _entries[i + 1];
    _hits[i] = _hits[i + 1];
  }
  _count--;
  _hits[_count] = 0;
  return true;
}

size_t ForwardACL::serialise(uint8_t* buf, size_t buf_len) const {
  size_t len = HEADER_SIZE + ENTRY_SIZE * _count;
  if (buf_len < len) return 0;
  uint8_t* p = buf;
  memcpy(p, FWD_ACL_MAGIC, 4); p += 4;
  *p++ = FWD_ACL_FILE_VERSION;
  *p++ = _enabled ? 1 : 0;
  *p++ = _count;
  for (int i = 0; i < _count; i++) {
    const AclEntry& e = _entries[i];
    *p++ = e.hop[0];
    *p++ = e.hop[1];
    *p++ = e.hop_len;
    *p++ = e.src;
    *p++ = e.dst;
    *p++ = e.flags;
  }
  return len;
}

bool ForwardACL::deserialise(const uint8_t* buf, size_t len) {
  reset();
  if (buf == NULL || len < HEADER_SIZE) return false;
  if (memcmp(buf, FWD_ACL_MAGIC, 4) != 0) return false;
  if (buf[4] != FWD_ACL_FILE_VERSION) return false;
  uint8_t enabled = buf[5];
  uint8_t count = buf[6];
  if (enabled > 1 || count > FWD_ACL_MAX_ENTRIES) return false;
  if (len != HEADER_SIZE + ENTRY_SIZE * (size_t)count) return false;

  const uint8_t* p = &buf[HEADER_SIZE];
  for (int i = 0; i < count; i++) {
    AclEntry e;
    e.hop[0] = *p++;
    e.hop[1] = *p++;
    e.hop_len = *p++;
    e.src = *p++;
    e.dst = *p++;
    e.flags = *p++;
    if (!isValidEntry(e)) { reset(); return false; }
    _entries[i] = e;
  }
  _count = count;
  _enabled = enabled != 0;
  return true;
}

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

// parses exactly 'n' bytes (2*n hex chars, case-insensitive), nothing more
static bool parseHexExact(const char* s, uint8_t* dest, int n) {
  if ((int)strlen(s) != n * 2) return false;
  for (int i = 0; i < n; i++) {
    int hi = hexNibble(s[i * 2]), lo = hexNibble(s[i * 2 + 1]);
    if (hi < 0 || lo < 0) return false;
    dest[i] = (uint8_t)((hi << 4) | lo);
  }
  return true;
}

bool ForwardACL::parseEntry(const char* hop, const char* src, const char* dst, AclEntry& out) {
  memset(&out, 0, sizeof(out));
  if (strcmp(hop, "*") == 0) {
    out.hop_len = 0;
  } else if (parseHexExact(hop, out.hop, 1)) {
    out.hop_len = 1;
  } else if (parseHexExact(hop, out.hop, 2)) {
    out.hop_len = 2;
  } else {
    return false;
  }
  if (strcmp(src, "*") == 0) {
    out.flags |= ACL_FLAG_SRC_ANY;
  } else if (!parseHexExact(src, &out.src, 1)) {
    return false;
  }
  if (strcmp(dst, "*") == 0) {
    out.flags |= ACL_FLAG_DST_ANY;
  } else if (!parseHexExact(dst, &out.dst, 1)) {
    return false;
  }
  return true;
}

int ForwardACL::loadDefaults(const char* spec) {
  reset();
  if (spec == NULL) return -1;
  const char* p = spec;
  while (*p) {
    while (*p == ',' || *p == ';' || *p == ' ') p++;
    if (*p == 0) break;
    char tok[16];
    int n = 0;
    while (*p && *p != ',' && *p != ';' && *p != ' ') {
      if (n >= (int)sizeof(tok) - 1) { reset(); return -1; }
      tok[n++] = *p++;
    }
    tok[n] = 0;
    char* s1 = strchr(tok, '/');
    char* s2 = s1 ? strchr(s1 + 1, '/') : NULL;
    if (s2 == NULL || strchr(s2 + 1, '/') != NULL) { reset(); return -1; }
    *s1 = 0; *s2 = 0;
    AclEntry e;
    if (!parseEntry(tok, s1 + 1, s2 + 1, e) || add(e) < 0) { reset(); return -1; }
  }
  _enabled = _count > 0;
  return _count;
}

void ForwardACL::formatEntry(const AclEntry& e, char* dest) {
  char hop[5], src[3], dst[3];
  if (e.hop_len == 0) strcpy(hop, "*");
  else if (e.hop_len == 1) sprintf(hop, "%02x", e.hop[0]);
  else sprintf(hop, "%02x%02x", e.hop[0], e.hop[1]);
  if (e.flags & ACL_FLAG_SRC_ANY) strcpy(src, "*"); else sprintf(src, "%02x", e.src);
  if (e.flags & ACL_FLAG_DST_ANY) strcpy(dst, "*"); else sprintf(dst, "%02x", e.dst);
  sprintf(dest, "%s/%s/%s", hop, src, dst);
}

// splits 'buf' in place on spaces, returns number of tokens
static int tokenise(char* buf, char* argv[], int max_args) {
  int n = 0;
  char* p = buf;
  while (*p && n < max_args) {
    while (*p == ' ') p++;
    if (*p == 0) break;
    argv[n++] = p;
    while (*p && *p != ' ') p++;
    if (*p) *p++ = 0;
  }
  while (*p == ' ') p++;
  if (*p) return -1;   // too many args
  return n;
}

bool ForwardACL::handleCommand(const char* args, char* reply) {
  char buf[64];
  if (args == NULL) args = "";
  if (strlen(args) >= sizeof(buf)) {   // reject rather than silently truncate
    strcpy(reply, "ERR: command too long");
    return false;
  }
  strcpy(buf, args);
  char* argv[5];
  int argc = tokenise(buf, argv, 5);

  if (argc <= 0) {
    strcpy(reply, "ERR: usage acl add|del|list|clear|on|off|status|reset");
    return false;
  }
  const char* cmd = argv[0];

  if (strcmp(cmd, "add") == 0) {
    if (argc != 4) { strcpy(reply, "ERR: usage acl add <hop> <src> <dst>"); return false; }
    AclEntry e;
    if (!parseEntry(argv[1], argv[2], argv[3], e)) { strcpy(reply, "ERR: bad hex"); return false; }
    int r = add(e);
    if (r == ADD_ERR_INVALID) { strcpy(reply, "ERR: all-wildcard rule"); return false; }
    if (r == ADD_ERR_DUPLICATE) { strcpy(reply, "ERR: duplicate"); return false; }
    if (r == ADD_ERR_FULL) { strcpy(reply, "ERR: full"); return false; }
    sprintf(reply, "OK: %d", r);
    return true;
  }
  if (strcmp(cmd, "del") == 0) {
    if (argc != 2) { strcpy(reply, "ERR: usage acl del <n>"); return false; }
    char* end;
    long n = strtol(argv[1], &end, 10);
    if (*end != 0 || n < 1 || n > _count || !remove((int)n)) { strcpy(reply, "ERR: no such entry"); return false; }
    strcpy(reply, "OK");
    return true;
  }
  if (strcmp(cmd, "list") == 0) {
    if (argc > 2) { strcpy(reply, "ERR: usage acl list [page]"); return false; }
    int pages = (_count + FWD_ACL_LIST_PAGE - 1) / FWD_ACL_LIST_PAGE;
    long page = 1;
    if (argc == 2) {
      char* end;
      page = strtol(argv[1], &end, 10);
      if (*end != 0 || page < 1 || (page > pages && !(page == 1 && pages == 0))) {
        strcpy(reply, "ERR: no such page");
        return false;
      }
    }
    if (_count == 0) { strcpy(reply, "(empty)"); return false; }
    char* dp = reply;
    int start = (page - 1) * FWD_ACL_LIST_PAGE;
    int end = start + FWD_ACL_LIST_PAGE;
    if (end > _count) end = _count;
    for (int i = start; i < end; i++) {
      char ent[16];
      formatEntry(_entries[i], ent);
      // worst case per line: "32:c8f2/08/7d h=4294967295\n" = 27 chars
      dp += sprintf(dp, "%s%d:%s h=%lu", i > start ? "\n" : "", i + 1, ent, (unsigned long)_hits[i]);
    }
    if (pages > 1) sprintf(dp, "\n(page %ld/%d)", page, pages);
    return false;
  }
  if (argc != 1) { strcpy(reply, "ERR: unexpected args"); return false; }

  if (strcmp(cmd, "clear") == 0) {
    clear();
    strcpy(reply, "OK");
    return true;
  }
  if (strcmp(cmd, "on") == 0 || strcmp(cmd, "off") == 0) {
    setEnabled(cmd[1] == 'n');
    strcpy(reply, "OK");
    return true;
  }
  if (strcmp(cmd, "status") == 0) {
    sprintf(reply, "acl %s, %d/%d entries, %lu hits", _enabled ? "on" : "off", _count,
            FWD_ACL_MAX_ENTRIES, (unsigned long)getTotalHits());
    return false;
  }
  if (strcmp(cmd, "reset") == 0) {
    resetHits();
    strcpy(reply, "OK");
    return false;   // hit counters are RAM only
  }
  strcpy(reply, "ERR: unknown acl command");
  return false;
}
