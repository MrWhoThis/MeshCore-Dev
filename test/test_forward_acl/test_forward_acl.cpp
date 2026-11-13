#include <gtest/gtest.h>
#include <string.h>
#include <string>
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
  EXPECT_FALSE(acl.handleCommand("add c8f2a1 08 7d", reply)); EXPECT_STREQ(reply, "ERR: bad hex");
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
    sprintf(cmd, "add %02x%02x %02x %02x", i, i, i, i);
    acl.handleCommand(cmd, reply);
  }
  int pages = (FWD_ACL_MAX_ENTRIES + FWD_ACL_LIST_PAGE - 1) / FWD_ACL_LIST_PAGE;
  for (int pg = 1; pg <= pages; pg++) {
    char cmd[16];
    sprintf(cmd, "list %d", pg);
    acl.handleCommand(cmd, reply);
    EXPECT_LT(strlen(reply) + 3, 160u) << reply;   // +3 for optional 'XX|' companion prefix
  }
  acl.handleCommand("list 1", reply);
  EXPECT_NE(strstr(reply, "(page 1/7)"), nullptr) << reply;
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
  memcpy(buf, good, len); buf[9] = 7;    check_bad(buf, len);       // bad hop_len
  memcpy(buf, good, len); buf[9] = 0; buf[12] = 3; check_bad(buf, len);  // all-wildcard entry
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

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(ForwardACL, LoadDefaultsFromBuildSpec) {
  ForwardACL acl;
  EXPECT_EQ(acl.loadDefaults("c8/08/7d,c8/08/11;c8/7d/08 c8/11/08"), 4);
  EXPECT_TRUE(acl.isEnabled());
  char reply[160];
  acl.handleCommand(" list", reply);
  EXPECT_STREQ(reply, "1:c8/08/7d h=0\n2:c8/08/11 h=0\n3:c8/7d/08 h=0\n4:c8/11/08 h=0");

  // captured packet is a flood from 08 to 7d whose first hop is c8 -> rule 1 blocks it
  Packet p = fromHex(CAPTURED);
  EXPECT_TRUE(acl.shouldBlock(&p));
  EXPECT_EQ(acl.getHits(0), 1u);
}

TEST(ForwardACL, LoadDefaultsWildcardsAndEmpty) {
  ForwardACL acl;
  EXPECT_EQ(acl.loadDefaults("c8f2/*/7d"), 1);
  EXPECT_EQ(acl.loadDefaults(""), 0);
  EXPECT_FALSE(acl.isEnabled());
}

TEST(ForwardACL, LoadDefaultsRejectsBadSpec) {
  ForwardACL acl;
  EXPECT_EQ(acl.loadDefaults("c8/08/7d,zz/08/7d"), -1);   // bad hex
  EXPECT_EQ(acl.getCount(), 0);
  EXPECT_FALSE(acl.isEnabled());
  EXPECT_EQ(acl.loadDefaults("c8/08"), -1);               // missing field
  EXPECT_EQ(acl.loadDefaults("c8/08/7d/11"), -1);         // extra field
  EXPECT_EQ(acl.loadDefaults("*/*/*"), -1);               // all-wildcard
  EXPECT_EQ(acl.loadDefaults("c8/08/7d,c8/08/7d"), -1);   // duplicate
  EXPECT_EQ(acl.loadDefaults("c8f2aa/08/7d"), -1);        // hop too long
}

TEST(ForwardACL, SpecHashDetectsChangedList) {
  const char* a = "c8/08/7d,c8/08/11,c8/7d/08,c8/11/08";
  EXPECT_EQ(ForwardACL::specHash(a), ForwardACL::specHash("c8/08/7d,c8/08/11,c8/7d/08,c8/11/08"));
  EXPECT_NE(ForwardACL::specHash(a), ForwardACL::specHash("c8/08/7d,c8/08/11,c8/7d/08"));
  EXPECT_NE(ForwardACL::specHash(a), ForwardACL::specHash("c8/08/7d,c8/08/11,c8/7d/08,c8/11/09"));
}
