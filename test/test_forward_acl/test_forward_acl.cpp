#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include <vector>
#include "../../examples/simple_repeater/ForwardACL.h"

using namespace mesh;

// captured packet: flood TXT_MSG, 6 hops of 1-byte hashes (c8 bc d3 0d 12 ab), dest 7d, src 08
static const char* CAPTURED =
  "0906c8bcd30d12ab7d080b4ae3da7891cbe28d5b67e2e746eeea42bb47e3d02e5a529918ca3e01e702890bd5";

static Packet fromHex(const char* hex) {
  uint8_t raw[256];
  size_t n = strlen(hex) / 2;
  for (size_t i = 0; i < n; i++) {
    unsigned int b;
    sscanf(&hex[i * 2], "%2x", &b);
    raw[i] = (uint8_t)b;
  }
  Packet p;
  EXPECT_TRUE(p.readFrom(raw, (uint8_t)n));
  return p;
}

static AclEntry rule(const char* hop, const char* src, const char* dst) {
  AclEntry e;
  EXPECT_TRUE(ForwardACL::parseEntry(hop, src, dst, e)) << hop << " " << src << " " << dst;
  return e;
}

static bool blockedBy(const char* hop, const char* src, const char* dst, const Packet& p) {
  ForwardACL acl;
  std::string cmd = std::string(" add ") + hop + " " + src + " " + dst;
  char reply[160];
  EXPECT_TRUE(acl.handleCommand(cmd.c_str(), reply)) << reply;
  acl.setEnabled(true);
  return acl.shouldBlock(&p);
}

TEST(ForwardACL, CapturedPacketDecodes) {
  Packet p = fromHex(CAPTURED);
  EXPECT_EQ(p.getRouteType(), ROUTE_TYPE_FLOOD);
  EXPECT_EQ(p.getPayloadType(), PAYLOAD_TYPE_TXT_MSG);
  EXPECT_EQ(p.getPathHashCount(), 6);
  EXPECT_EQ(p.getPathHashSize(), 1);
  EXPECT_EQ(p.path[0], 0xc8);
  EXPECT_EQ(p.payload[0], 0x7d);
  EXPECT_EQ(p.payload[1], 0x08);
}

TEST(ForwardACL, CapturedPacketBlocked) {
  Packet p = fromHex(CAPTURED);
  EXPECT_TRUE(blockedBy("c8", "08", "7d", p));
  EXPECT_TRUE(blockedBy("C8", "*", "*", p));
  EXPECT_TRUE(blockedBy("*", "08", "*", p));
  EXPECT_TRUE(blockedBy("c8f2", "08", "7d", p));   // 2-byte rule vs 1-byte path: first byte only
}

TEST(ForwardACL, CapturedPacketNotBlocked) {
  Packet p = fromHex(CAPTURED);
  EXPECT_FALSE(blockedBy("bc", "08", "7d", p));    // bc is the 2nd hop, not the first
  EXPECT_FALSE(blockedBy("c8", "09", "*", p));
}

TEST(ForwardACL, DisabledAllowsEverything) {
  Packet p = fromHex(CAPTURED);
  ForwardACL acl;
  char reply[160];
  acl.handleCommand("add c8 08 7d", reply);
  EXPECT_FALSE(acl.shouldBlock(&p));
  acl.handleCommand("on", reply);
  EXPECT_TRUE(acl.shouldBlock(&p));
  acl.handleCommand("off", reply);
  EXPECT_FALSE(acl.shouldBlock(&p));
  EXPECT_EQ(acl.getCount(), 1);   // entries kept when off
}

TEST(ForwardACL, DirectPacketIgnoresHopRules) {
  Packet p = fromHex(CAPTURED);
  p.header = (p.header & ~PH_ROUTE_MASK) | ROUTE_TYPE_DIRECT;
  EXPECT_FALSE(blockedBy("c8", "08", "7d", p));
  EXPECT_TRUE(blockedBy("*", "08", "7d", p));

  p.header = (p.header & ~PH_ROUTE_MASK) | ROUTE_TYPE_TRANSPORT_DIRECT;
  EXPECT_FALSE(blockedBy("c8", "08", "7d", p));
  EXPECT_TRUE(blockedBy("*", "08", "7d", p));
}

TEST(ForwardACL, TransportFloodUsesPathAccessors) {
  // same packet as TRANSPORT_FLOOD: 4 transport-code bytes precede the path length on the wire
  std::string hex = std::string("08") + "11223344" + (CAPTURED + 2);
  Packet p = fromHex(hex.c_str());
  ASSERT_EQ(p.getRouteType(), ROUTE_TYPE_TRANSPORT_FLOOD);
  EXPECT_EQ(p.path[0], 0xc8);
  EXPECT_TRUE(blockedBy("c8", "08", "7d", p));
  EXPECT_FALSE(blockedBy("11", "08", "7d", p));
}

TEST(ForwardACL, TwoByteHashFlood) {
  Packet p = fromHex(CAPTURED);
  // re-encode path as 2 hops of 2-byte hashes: c8f2, bcd3
  p.setPathHashSizeAndCount(2, 2);
  p.path[0] = 0xc8; p.path[1] = 0xf2; p.path[2] = 0xbc; p.path[3] = 0xd3;
  EXPECT_TRUE(blockedBy("c8", "08", "7d", p));     // 1-byte rule prefix-matches
  EXPECT_TRUE(blockedBy("c8f2", "08", "7d", p));   // exact
  EXPECT_FALSE(blockedBy("c8f3", "08", "7d", p));
  EXPECT_FALSE(blockedBy("bcd3", "08", "7d", p));
}

TEST(ForwardACL, ZeroHopFlood) {
  Packet p = fromHex(CAPTURED);
  p.setPathHashSizeAndCount(1, 0);
  EXPECT_FALSE(blockedBy("c8", "08", "7d", p));
  EXPECT_TRUE(blockedBy("*", "08", "7d", p));
}

TEST(ForwardACL, AdvertNeverBlocked) {
  Packet p = fromHex(CAPTURED);
  p.header = ROUTE_TYPE_FLOOD | (PAYLOAD_TYPE_ADVERT << PH_TYPE_SHIFT);
  EXPECT_FALSE(blockedBy("c8", "*", "*", p));
  EXPECT_FALSE(blockedBy("*", "08", "*", p));
  p.header = ROUTE_TYPE_FLOOD | (PAYLOAD_TYPE_GRP_TXT << PH_TYPE_SHIFT);
  EXPECT_FALSE(blockedBy("c8", "*", "*", p));
  p.header = ROUTE_TYPE_FLOOD | (PAYLOAD_TYPE_ACK << PH_TYPE_SHIFT);
  EXPECT_FALSE(blockedBy("c8", "*", "*", p));
}

TEST(ForwardACL, OtherCoveredTypesAndShortPayload) {
  Packet p = fromHex(CAPTURED);
  for (uint8_t t : { PAYLOAD_TYPE_REQ, PAYLOAD_TYPE_RESPONSE, PAYLOAD_TYPE_PATH }) {
    p.header = ROUTE_TYPE_FLOOD | (t << PH_TYPE_SHIFT);
    EXPECT_TRUE(blockedBy("c8", "08", "7d", p)) << (int)t;
  }
  p.payload_len = 1;
  EXPECT_FALSE(blockedBy("*", "*", "7d", p));
}

TEST(ForwardACL, HitCounters) {
  Packet p = fromHex(CAPTURED);
  ForwardACL acl;
  char reply[160];
  acl.handleCommand("add bc * *", reply);
  acl.handleCommand("add c8 * *", reply);
  acl.handleCommand("on", reply);
  EXPECT_TRUE(acl.shouldBlock(&p));
  EXPECT_TRUE(acl.shouldBlock(&p));
  EXPECT_EQ(acl.getHits(0), 0u);
  EXPECT_EQ(acl.getHits(1), 2u);
  acl.handleCommand("status", reply);
  EXPECT_STREQ(reply, "acl on, 2/32 entries, 2 hits");
  EXPECT_FALSE(acl.handleCommand("reset", reply));
  EXPECT_EQ(acl.getTotalHits(), 0u);
}

TEST(ForwardACL, CliErrors) {
  ForwardACL acl;
  char reply[160];
  EXPECT_FALSE(acl.handleCommand("add zz 08 7d", reply)); EXPECT_STREQ(reply, "ERR: bad hex");
  EXPECT_FALSE(acl.handleCommand("add c 08 7d", reply)); EXPECT_STREQ(reply, "ERR: bad hex");
  EXPECT_FALSE(acl.handleCommand("add c8f2a1b3 08 7d", reply)); EXPECT_STREQ(reply, "ERR: bad hex");
  EXPECT_FALSE(acl.handleCommand("add 00f 08 7d", reply)); EXPECT_STREQ(reply, "ERR: bad hex");
  EXPECT_FALSE(acl.handleCommand("add c8 008 7d", reply)); EXPECT_STREQ(reply, "ERR: bad hex");
  EXPECT_FALSE(acl.handleCommand("add * * *", reply)); EXPECT_STREQ(reply, "ERR: all-wildcard rule");
  EXPECT_TRUE(acl.handleCommand("add C8 08 7D", reply)); EXPECT_STREQ(reply, "OK: 1");
  EXPECT_FALSE(acl.handleCommand("add c8 08 7d", reply)); EXPECT_STREQ(reply, "ERR: duplicate");
  EXPECT_FALSE(acl.handleCommand("del 2", reply)); EXPECT_STREQ(reply, "ERR: no such entry");
  EXPECT_FALSE(acl.handleCommand("del 0", reply)); EXPECT_STREQ(reply, "ERR: no such entry");
  EXPECT_FALSE(acl.handleCommand("del x", reply)); EXPECT_STREQ(reply, "ERR: no such entry");
  EXPECT_FALSE(acl.handleCommand("bogus", reply)); EXPECT_STREQ(reply, "ERR: unknown acl command");

  for (int i = 1; i < FWD_ACL_MAX_ENTRIES; i++) {
    char cmd[32];
    sprintf(cmd, "add * %02x *", i);
    EXPECT_TRUE(acl.handleCommand(cmd, reply)) << reply;
  }
  EXPECT_FALSE(acl.handleCommand("add * ff ff", reply)); EXPECT_STREQ(reply, "ERR: full");
}

TEST(ForwardACL, DelCompactsAndList) {
  ForwardACL acl;
  char reply[160];
  acl.handleCommand("add c8 08 7d", reply);
  acl.handleCommand("add c8f2 * 7d", reply);
  acl.handleCommand("add * 09 *", reply);
  acl.handleCommand("list", reply);
  EXPECT_STREQ(reply, "1:c8/08/7d h=0\n2:c8f2/*/7d h=0\n3:*/09/* h=0");
  EXPECT_TRUE(acl.handleCommand("del 2", reply));
  acl.handleCommand("list", reply);
  EXPECT_STREQ(reply, "1:c8/08/7d h=0\n2:*/09/* h=0");
  EXPECT_TRUE(acl.handleCommand("clear", reply));
  acl.handleCommand("list", reply);
  EXPECT_STREQ(reply, "(empty)");
}

TEST(ForwardACL, ListPaginationFitsReplyBuffer) {
  ForwardACL acl;
  char reply[200];
  for (int i = 0; i < FWD_ACL_MAX_ENTRIES; i++) {
    char cmd[32];
    // worst-case widths: 3-byte hops, alternating DM and group rules
    if (i & 1) sprintf(cmd, "add grp %02x%02x%02x %02x", i, i, i, i);
    else sprintf(cmd, "add %02x%02x%02x %02x %02x", i, i, i, i, i);
    EXPECT_TRUE(acl.handleCommand(cmd, reply)) << reply;
  }
  int pages = (FWD_ACL_MAX_ENTRIES + FWD_ACL_LIST_PAGE - 1) / FWD_ACL_LIST_PAGE;
  for (int pg = 1; pg <= pages; pg++) {
    char cmd[16];
    sprintf(cmd, "list %d", pg);
    acl.handleCommand(cmd, reply);
    // +3 for optional 'XX|' companion prefix, +9 per line for "h=0" growing to "h=4294967295"
    EXPECT_LT(strlen(reply) + 3 + 9 * FWD_ACL_LIST_PAGE, 160u) << reply;
  }
  acl.handleCommand("list 1", reply);
  char footer[24];
  sprintf(footer, "(page 1/%d)", pages);
  EXPECT_NE(strstr(reply, footer), nullptr) << reply;
  EXPECT_FALSE(acl.handleCommand("list 99", reply));
  EXPECT_STREQ(reply, "ERR: no such page");
}

TEST(ForwardACL, SerialiseRoundTrip) {
  ForwardACL acl;
  char reply[160];
  acl.handleCommand("add c8 08 7d", reply);
  acl.handleCommand("add c8f2 * 7d", reply);
  acl.handleCommand("on", reply);
  uint8_t buf[ForwardACL::MAX_SERIALISED_SIZE];
  size_t len = acl.serialise(buf, sizeof(buf));
  EXPECT_EQ(len, ForwardACL::HEADER_SIZE + 2 * ForwardACL::ENTRY_SIZE);

  ForwardACL b;
  EXPECT_TRUE(b.deserialise(buf, len));
  EXPECT_TRUE(b.isEnabled());
  b.handleCommand("list", reply);
  EXPECT_STREQ(reply, "1:c8/08/7d h=0\n2:c8f2/*/7d h=0");
}

TEST(ForwardACL, CorruptFileGivesEmptyDisabled) {
  ForwardACL acl;
  char reply[160];
  acl.handleCommand("add c8 08 7d", reply);
  acl.handleCommand("on", reply);
  uint8_t good[ForwardACL::MAX_SERIALISED_SIZE];
  size_t len = acl.serialise(good, sizeof(good));

  auto check_bad = [&](uint8_t* buf, size_t n) {
    ForwardACL b;
    b.setEnabled(true);
    EXPECT_FALSE(b.deserialise(buf, n));
    EXPECT_FALSE(b.isEnabled());
    EXPECT_EQ(b.getCount(), 0);
  };
  uint8_t buf[ForwardACL::MAX_SERIALISED_SIZE + 8];
  memcpy(buf, good, len); buf[0] ^= 1;   check_bad(buf, len);       // magic
  memcpy(buf, good, len); buf[4] = 99;   check_bad(buf, len);       // version
  memcpy(buf, good, len);                check_bad(buf, len - 1);   // truncated
  memcpy(buf, good, len); buf[len] = 0;  check_bad(buf, len + 1);   // trailing junk
  memcpy(buf, good, len); buf[6] = 200;  check_bad(buf, len);       // count too big
  // v2 entry at offset 7: hop[3] | hop_len(10) | kind(11) | src(12) | dst(13) | flags(14)
  memcpy(buf, good, len); buf[10] = 7;   check_bad(buf, len);       // bad hop_len
  memcpy(buf, good, len); buf[10] = 0; buf[14] = 3; check_bad(buf, len);  // all-wildcard entry
  memcpy(buf, good, len); buf[11] = 2;   check_bad(buf, len);       // unknown kind
  memcpy(buf, good, len); buf[11] = ACL_KIND_GRP; check_bad(buf, len);   // grp rule with a src
  memcpy(buf, good, len); buf[14] = 0x80; check_bad(buf, len);      // unknown flag
  memcpy(buf, good, len); buf[14] = ACL_FLAG_HOP_ZERO; check_bad(buf, len);  // zero-hop with hop bytes
  check_bad(buf, 0);
  check_bad(nullptr, 0);
}

TEST(ForwardACL, CliEdgeCases) {
  ForwardACL acl;
  char reply[160];
  EXPECT_FALSE(acl.handleCommand("list 1", reply)); EXPECT_STREQ(reply, "(empty)");
  EXPECT_FALSE(acl.handleCommand("list 0", reply)); EXPECT_STREQ(reply, "ERR: no such page");
  EXPECT_FALSE(acl.handleCommand("list -1", reply)); EXPECT_STREQ(reply, "ERR: no such page");
  EXPECT_FALSE(acl.handleCommand("list 2", reply)); EXPECT_STREQ(reply, "ERR: no such page");

  EXPECT_TRUE(acl.handleCommand("  add  c8 08 7d  ", reply)); EXPECT_STREQ(reply, "OK: 1");

  // over-long input is rejected, not truncated into a valid command
  std::string longcmd = std::string("add * 03 *") + std::string(60, ' ') + "junk";
  EXPECT_FALSE(acl.handleCommand(longcmd.c_str(), reply)); EXPECT_STREQ(reply, "ERR: command too long");
  EXPECT_EQ(acl.getCount(), 1);

  // index that would wrap to 1 when narrowed to int on 64-bit hosts
  EXPECT_FALSE(acl.handleCommand("del 4294967297", reply)); EXPECT_STREQ(reply, "ERR: no such entry");
  EXPECT_FALSE(acl.handleCommand("del -1", reply)); EXPECT_STREQ(reply, "ERR: no such entry");
  EXPECT_EQ(acl.getCount(), 1);
}

TEST(ForwardACL, HitsFollowEntriesOnDelAndClear) {
  Packet p = fromHex(CAPTURED);
  ForwardACL acl;
  char reply[160];
  acl.handleCommand("add bc * *", reply);
  acl.handleCommand("add c8 * *", reply);
  acl.handleCommand("on", reply);
  acl.shouldBlock(&p);
  acl.handleCommand("del 1", reply);
  EXPECT_EQ(acl.getHits(0), 1u);   // hit counter moved with its rule
  acl.handleCommand("clear", reply);
  acl.handleCommand("add * 08 *", reply);
  EXPECT_EQ(acl.getHits(0), 0u);   // new rule starts at zero
}

TEST(ForwardACL, WildcardHopMatchesZeroHopDirect) {
  Packet p = fromHex(CAPTURED);
  p.header = (p.header & ~PH_ROUTE_MASK) | ROUTE_TYPE_DIRECT;
  p.setPathHashSizeAndCount(1, 0);
  EXPECT_TRUE(blockedBy("*", "08", "7d", p));
  EXPECT_FALSE(blockedBy("c8", "08", "7d", p));
}

// ---- group rules (GRP_TXT / GRP_DATA), 3-byte hops, zero-hop ----

// the captured packet re-typed as a group message: payload[0] (0x7d) is then the channel hash
static Packet groupPacket(uint8_t type = PAYLOAD_TYPE_GRP_TXT) {
  Packet p = fromHex(CAPTURED);
  p.header = (p.header & ~(PH_TYPE_MASK << PH_TYPE_SHIFT)) | (type << PH_TYPE_SHIFT);
  return p;
}

static bool blockedByGrp(const char* hop, const char* chan, const Packet& p) {
  ForwardACL acl;
  std::string cmd = std::string("add grp ") + hop + " " + chan;
  char reply[160];
  EXPECT_TRUE(acl.handleCommand(cmd.c_str(), reply)) << reply;
  acl.setEnabled(true);
  return acl.shouldBlock(&p);
}

TEST(ForwardACLGroup, ChannelAndHopMatch) {
  for (uint8_t t : { PAYLOAD_TYPE_GRP_TXT, PAYLOAD_TYPE_GRP_DATA }) {
    Packet p = groupPacket(t);
    ASSERT_EQ(p.getPayloadType(), t);
    EXPECT_TRUE(blockedByGrp("c8", "7d", p)) << (int)t;
    EXPECT_TRUE(blockedByGrp("*", "7d", p)) << (int)t;
    EXPECT_TRUE(blockedByGrp("c8", "*", p)) << (int)t;
    EXPECT_TRUE(blockedByGrp("C8", "7D", p)) << (int)t;
    EXPECT_FALSE(blockedByGrp("bc", "7d", p)) << (int)t;   // 2nd hop, not the first
    EXPECT_FALSE(blockedByGrp("c8", "11", p)) << (int)t;   // other channel
    EXPECT_FALSE(blockedByGrp("0", "7d", p)) << (int)t;    // packet has been relayed
  }
}

TEST(ForwardACLGroup, KindsDoNotCrossMatch) {
  Packet dm = fromHex(CAPTURED);
  EXPECT_FALSE(blockedByGrp("c8", "7d", dm));
  EXPECT_FALSE(blockedByGrp("*", "7d", dm));
  Packet grp = groupPacket();
  EXPECT_FALSE(blockedBy("c8", "*", "*", grp));
  EXPECT_FALSE(blockedBy("*", "*", "7d", grp));
  // other payload types are never matched by group rules
  for (uint8_t t : { PAYLOAD_TYPE_ADVERT, PAYLOAD_TYPE_ACK, PAYLOAD_TYPE_ANON_REQ, PAYLOAD_TYPE_TRACE }) {
    Packet p = groupPacket(t);
    EXPECT_FALSE(blockedByGrp("c8", "*", p)) << (int)t;
  }
}

TEST(ForwardACLGroup, VersionAndShortPayload) {
  Packet p = groupPacket();
  p.payload_len = 0;
  EXPECT_FALSE(blockedByGrp("c8", "*", p));
  p = groupPacket();
  p.header |= (PAYLOAD_VER_2 << PH_VER_SHIFT);
  EXPECT_FALSE(blockedByGrp("c8", "7d", p));
}

TEST(ForwardACLGroup, DirectIgnoresHopRules) {
  Packet p = groupPacket();
  p.header = (p.header & ~PH_ROUTE_MASK) | ROUTE_TYPE_DIRECT;
  EXPECT_FALSE(blockedByGrp("c8", "7d", p));
  EXPECT_TRUE(blockedByGrp("*", "7d", p));
}

static void setPath3(Packet& p) {
  // 2 hops of 3-byte hashes: c8f2a1, bcd3e4
  p.setPathHashSizeAndCount(3, 2);
  const uint8_t path[] = { 0xc8, 0xf2, 0xa1, 0xbc, 0xd3, 0xe4 };
  memcpy(p.path, path, sizeof(path));
}

TEST(ForwardACLHop, ThreeByteHashFlood) {
  Packet dm = fromHex(CAPTURED);
  setPath3(dm);
  Packet grp = groupPacket();
  setPath3(grp);
  for (const char* hop : { "c8", "c8f2", "c8f2a1", "C8F2A1" }) {
    EXPECT_TRUE(blockedBy(hop, "08", "7d", dm)) << hop;
    EXPECT_TRUE(blockedByGrp(hop, "7d", grp)) << hop;
  }
  for (const char* hop : { "c9", "c8f3", "c8f2a2", "bcd3e4", "bc" }) {
    EXPECT_FALSE(blockedBy(hop, "08", "7d", dm)) << hop;
    EXPECT_FALSE(blockedByGrp(hop, "7d", grp)) << hop;
  }
}

TEST(ForwardACLHop, LongRuleCannotBeEvadedBySmallerHashSize) {
  // the originator picks the hash size; a 3-byte rule still matches on the overlapping prefix
  Packet p = groupPacket();                       // 1-byte path: c8 bc ...
  EXPECT_TRUE(blockedByGrp("c8f2a1", "7d", p));
  EXPECT_FALSE(blockedByGrp("c9f2a1", "7d", p));

  p.setPathHashSizeAndCount(2, 1);                // 2-byte path: c8f2
  p.path[0] = 0xc8; p.path[1] = 0xf2;
  EXPECT_TRUE(blockedByGrp("c8f2a1", "7d", p));
  EXPECT_FALSE(blockedByGrp("c8f3a1", "7d", p));

  Packet dm = fromHex(CAPTURED);                  // same for DM rules
  EXPECT_TRUE(blockedBy("c8f2a1", "08", "7d", dm));
  dm.setPathHashSizeAndCount(2, 1);
  dm.path[0] = 0xc8; dm.path[1] = 0xf2;
  EXPECT_TRUE(blockedBy("c8f2a1", "08", "7d", dm));
  EXPECT_FALSE(blockedBy("c8f3a1", "08", "7d", dm));
}

TEST(ForwardACLHop, ZeroHopMatchesOnlyUnrelayedFlood) {
  for (uint8_t sz : { 1, 2, 3 }) {
    Packet grp = groupPacket();
    grp.setPathHashSizeAndCount(sz, 0);
    EXPECT_TRUE(blockedByGrp("0", "7d", grp)) << (int)sz;
    EXPECT_TRUE(blockedByGrp("0", "*", grp)) << (int)sz;
    EXPECT_FALSE(blockedByGrp("0", "11", grp)) << (int)sz;
    EXPECT_FALSE(blockedByGrp("c8", "7d", grp)) << (int)sz;   // no first hop to match

    Packet dm = fromHex(CAPTURED);
    dm.setPathHashSizeAndCount(sz, 0);
    EXPECT_TRUE(blockedBy("0", "08", "7d", dm)) << (int)sz;
    EXPECT_TRUE(blockedBy("0", "*", "*", dm)) << (int)sz;
    EXPECT_FALSE(blockedBy("0", "09", "*", dm)) << (int)sz;
  }
  // relayed flood: zero-hop rule doesn't apply
  EXPECT_FALSE(blockedByGrp("0", "7d", groupPacket()));
  EXPECT_FALSE(blockedBy("0", "08", "7d", fromHex(CAPTURED)));

  // direct packets never match zero-hop rules
  Packet d = groupPacket();
  d.header = (d.header & ~PH_ROUTE_MASK) | ROUTE_TYPE_DIRECT;
  d.setPathHashSizeAndCount(1, 0);
  EXPECT_FALSE(blockedByGrp("0", "7d", d));

  // transport flood counts as flood
  std::string hex = std::string("08") + "11223344" + (CAPTURED + 2);
  Packet tf = fromHex(hex.c_str());
  tf.setPathHashSizeAndCount(1, 0);
  EXPECT_TRUE(blockedBy("0", "08", "7d", tf));
}

TEST(ForwardACLCli, GroupCommands) {
  ForwardACL acl;
  char reply[160];
  EXPECT_FALSE(acl.handleCommand("add grp c8", reply)); EXPECT_STREQ(reply, "ERR: usage acl add grp <hop> <chan>");
  EXPECT_FALSE(acl.handleCommand("add grp c8 7d 01", reply)); EXPECT_STREQ(reply, "ERR: usage acl add grp <hop> <chan>");
  EXPECT_FALSE(acl.handleCommand("add grp", reply)); EXPECT_STREQ(reply, "ERR: usage acl add grp <hop> <chan>");
  EXPECT_FALSE(acl.handleCommand("add c8 7d", reply)); EXPECT_STREQ(reply, "ERR: usage acl add <hop> <src> <dst> | add grp <hop> <chan>");
  EXPECT_FALSE(acl.handleCommand("add grp zz 7d", reply)); EXPECT_STREQ(reply, "ERR: bad hex");
  EXPECT_FALSE(acl.handleCommand("add grp c8 7dd", reply)); EXPECT_STREQ(reply, "ERR: bad hex");
  EXPECT_FALSE(acl.handleCommand("add grp c8f2a1b3 7d", reply)); EXPECT_STREQ(reply, "ERR: bad hex");
  EXPECT_FALSE(acl.handleCommand("add grp 00 7", reply)); EXPECT_STREQ(reply, "ERR: bad hex");
  EXPECT_FALSE(acl.handleCommand("add grp * *", reply)); EXPECT_STREQ(reply, "ERR: all-wildcard rule");
  EXPECT_EQ(acl.getCount(), 0);

  EXPECT_TRUE(acl.handleCommand("add grp c8 7d", reply)); EXPECT_STREQ(reply, "OK: 1");
  EXPECT_FALSE(acl.handleCommand("add grp C8 7D", reply)); EXPECT_STREQ(reply, "ERR: duplicate");
  EXPECT_TRUE(acl.handleCommand("add c8 * 7d", reply)); EXPECT_STREQ(reply, "OK: 2");   // DM rule, not a dup
  EXPECT_TRUE(acl.handleCommand("add 0 * *", reply)); EXPECT_STREQ(reply, "OK: 3");
  EXPECT_TRUE(acl.handleCommand("add grp 0 *", reply)); EXPECT_STREQ(reply, "OK: 4");
  EXPECT_FALSE(acl.handleCommand("add grp 0 *", reply)); EXPECT_STREQ(reply, "ERR: duplicate");
  EXPECT_TRUE(acl.handleCommand("add grp 00 *", reply)); EXPECT_STREQ(reply, "OK: 5");  // hop hash 00 != zero-hop
  EXPECT_TRUE(acl.handleCommand("add c8f2a1 08 *", reply)); EXPECT_STREQ(reply, "OK: 6");

  acl.handleCommand("list", reply);
  EXPECT_STREQ(reply, "1:grp c8/#7d h=0\n2:c8/*/7d h=0\n3:0/*/* h=0\n4:grp 0/#* h=0\n(page 1/2)");
  acl.handleCommand("list 2", reply);
  EXPECT_STREQ(reply, "5:grp 00/#* h=0\n6:c8f2a1/08/* h=0\n(page 2/2)");
}

TEST(ForwardACLFile, RoundTripV2) {
  ForwardACL acl;
  char reply[160];
  acl.handleCommand("add grp c8f2a1 7d", reply);
  acl.handleCommand("add grp 0 *", reply);
  acl.handleCommand("add 0 08 *", reply);
  acl.handleCommand("add c8f2a1 * 7d", reply);
  uint8_t buf[ForwardACL::MAX_SERIALISED_SIZE];
  size_t len = acl.serialise(buf, sizeof(buf));
  EXPECT_EQ(len, ForwardACL::HEADER_SIZE + 4 * ForwardACL::ENTRY_SIZE);
  EXPECT_EQ(buf[4], 2);   // file version

  ForwardACL b;
  ASSERT_TRUE(b.deserialise(buf, len));
  EXPECT_FALSE(b.isEnabled());
  b.handleCommand("list", reply);
  EXPECT_STREQ(reply, "1:grp c8f2a1/#7d h=0\n2:grp 0/#* h=0\n3:0/08/* h=0\n4:c8f2a1/*/7d h=0");
}

TEST(ForwardACLFile, LoadsV1File) {
  // v1: magic | ver=1 | enabled | count | entries of hop[2] hop_len src dst flags
  const uint8_t v1[] = {
    'F', 'A', 'C', 'L', 1, 1, 3,
    0xc8, 0x00, 1, 0x08, 0x7d, 0,
    0xc8, 0xf2, 2, 0x00, 0x7d, ACL_FLAG_SRC_ANY,
    0x00, 0x00, 0, 0x09, 0x00, ACL_FLAG_DST_ANY,
  };
  ForwardACL acl;
  ASSERT_TRUE(acl.deserialise(v1, sizeof(v1)));
  EXPECT_TRUE(acl.isEnabled());
  EXPECT_EQ(acl.getCount(), 3);
  char reply[160];
  acl.handleCommand("list", reply);
  EXPECT_STREQ(reply, "1:c8/08/7d h=0\n2:c8f2/*/7d h=0\n3:*/09/* h=0");
  for (int i = 0; i < acl.getCount(); i++) EXPECT_EQ(acl.getEntry(i)->kind, ACL_KIND_DM);

  // migrated rules still only hit DM packets
  Packet dm = fromHex(CAPTURED);
  EXPECT_TRUE(acl.shouldBlock(&dm));
  Packet grp = groupPacket();
  EXPECT_FALSE(acl.shouldBlock(&grp));

  // and are re-saved as v2
  uint8_t buf[ForwardACL::MAX_SERIALISED_SIZE];
  size_t len = acl.serialise(buf, sizeof(buf));
  EXPECT_EQ(buf[4], 2);
  EXPECT_EQ(len, ForwardACL::HEADER_SIZE + 3 * ForwardACL::ENTRY_SIZE);
  ForwardACL b;
  ASSERT_TRUE(b.deserialise(buf, len));
  b.handleCommand("list", reply);
  EXPECT_STREQ(reply, "1:c8/08/7d h=0\n2:c8f2/*/7d h=0\n3:*/09/* h=0");
}

TEST(ForwardACLFile, RejectsBadV1) {
  auto bad = [](std::initializer_list<uint8_t> bytes) {
    std::vector<uint8_t> v(bytes);
    ForwardACL acl;
    EXPECT_FALSE(acl.deserialise(v.data(), v.size()));
    EXPECT_EQ(acl.getCount(), 0);
    EXPECT_FALSE(acl.isEnabled());
  };
  bad({ 'F', 'A', 'C', 'L', 1, 1, 1, 0xc8, 0xf2, 3, 0x08, 0x7d, 0 });                  // 3-byte hop didn't exist in v1
  bad({ 'F', 'A', 'C', 'L', 1, 1, 1, 0x00, 0x00, 0, 0x08, 0x7d, ACL_FLAG_HOP_ZERO });  // nor did zero-hop
  bad({ 'F', 'A', 'C', 'L', 1, 1, 1, 0xc8, 0x00, 1, 0x08, 0x7d });                     // truncated entry
  bad({ 'F', 'A', 'C', 'L', 1, 1, 1, 0xc8, 0x00, 1, 0x08, 0x7d, 0, 0, 0 });            // v2-sized entry
}

TEST(ForwardACLGroup, MoreCoverage) {
  // upper-case keyword
  {
    ForwardACL acl;
    char reply[160];
    EXPECT_TRUE(acl.handleCommand("add GRP c8 7d", reply)); EXPECT_STREQ(reply, "OK: 1");
    acl.handleCommand("list", reply);
    EXPECT_STREQ(reply, "1:grp c8/#7d h=0");
  }
  // GRP_DATA with 3-byte and zero-hop paths
  Packet p = groupPacket(PAYLOAD_TYPE_GRP_DATA);
  setPath3(p);
  EXPECT_TRUE(blockedByGrp("c8f2a1", "7d", p));
  EXPECT_FALSE(blockedByGrp("c8f2a2", "7d", p));
  p.setPathHashSizeAndCount(2, 0);
  EXPECT_TRUE(blockedByGrp("0", "7d", p));
  // group rules on TRANSPORT_FLOOD
  std::string hex = std::string("08") + "11223344" + (CAPTURED + 2);
  Packet tf = fromHex(hex.c_str());
  tf.header = (tf.header & ~(PH_TYPE_MASK << PH_TYPE_SHIFT)) | (PAYLOAD_TYPE_GRP_TXT << PH_TYPE_SHIFT);
  ASSERT_EQ(tf.getRouteType(), ROUTE_TYPE_TRANSPORT_FLOOD);
  EXPECT_TRUE(blockedByGrp("c8", "7d", tf));
  EXPECT_FALSE(blockedByGrp("0", "7d", tf));
  tf.setPathHashSizeAndCount(1, 0);
  EXPECT_TRUE(blockedByGrp("0", "7d", tf));
  // min_len boundary: 1-byte payload is enough for a channel rule
  Packet one = groupPacket();
  one.payload_len = 1;
  EXPECT_TRUE(blockedByGrp("c8", "7d", one));
}

TEST(ForwardACLFile, UnusedBytesCanonicalised) {
  // v2 grp entry with junk in hop[1..2] (hop_len 1), src and dst (DST_ANY)
  const uint8_t v2[] = {
    'F', 'A', 'C', 'L', 2, 0, 1,
    0xc8, 0xaa, 0xbb, 1, ACL_KIND_GRP, 0x55, 0x66, ACL_FLAG_SRC_ANY | ACL_FLAG_DST_ANY,
  };
  ForwardACL acl;
  ASSERT_TRUE(acl.deserialise(v2, sizeof(v2)));
  const AclEntry* e = acl.getEntry(0);
  EXPECT_EQ(e->hop[1], 0); EXPECT_EQ(e->hop[2], 0); EXPECT_EQ(e->src, 0); EXPECT_EQ(e->dst, 0);
  uint8_t buf[ForwardACL::MAX_SERIALISED_SIZE];
  size_t len = acl.serialise(buf, sizeof(buf));
  const uint8_t expect[] = {
    'F', 'A', 'C', 'L', 2, 0, 1,
    0xc8, 0x00, 0x00, 1, ACL_KIND_GRP, 0x00, 0x00, ACL_FLAG_SRC_ANY | ACL_FLAG_DST_ANY,
  };
  ASSERT_EQ(len, sizeof(expect));
  EXPECT_EQ(memcmp(buf, expect, len), 0);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
