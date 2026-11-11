#include <gtest/gtest.h>

#include <string_view>

#include "dde/utils.h"

namespace ZemaxDDE {
namespace {

TEST(UpdateStatus, ParsesUpdated) {
    EXPECT_EQ(parseUpdateStatus("0"), UpdateStatus::Updated);
    EXPECT_EQ(parseUpdateStatus(" 0 \r\n"), UpdateStatus::Updated);
    EXPECT_EQ(parseUpdateStatus(std::string_view("0\0", 2)), UpdateStatus::Updated);
}

TEST(UpdateStatus, ParsesNotUpdated) {
    EXPECT_EQ(parseUpdateStatus("-1"), UpdateStatus::NotUpdated);
    EXPECT_EQ(parseUpdateStatus(" -1\r\n"), UpdateStatus::NotUpdated);
    EXPECT_EQ(parseUpdateStatus(std::string_view("-1\0", 3)), UpdateStatus::NotUpdated);
}

TEST(UpdateStatus, ParsesUnknown) {
    EXPECT_EQ(parseUpdateStatus(""), UpdateStatus::Unknown);
    EXPECT_EQ(parseUpdateStatus("   "), UpdateStatus::Unknown);
    EXPECT_EQ(parseUpdateStatus("0,1"), UpdateStatus::Unknown);
    EXPECT_EQ(parseUpdateStatus("error"), UpdateStatus::Unknown);
}

TEST(Ack, StatuslessAcknowledgementIsUnspecified) {
    // Zemax 13 answers with an empty packed block, so the status word reads
    // back as zero. Such an acknowledgement says nothing and must never be
    // read as a rejection.
    EXPECT_EQ(classifyAck(0, false), AckVerdict::Unspecified);
}

TEST(Ack, AckFlagIsAccepted) {
    EXPECT_EQ(classifyAck(kDdeAckFAck, true), AckVerdict::Accepted);
}

TEST(Ack, BusyFlagIsBusy) {
    EXPECT_EQ(classifyAck(kDdeAckFBusy, true), AckVerdict::Busy);
}

TEST(Ack, ReadableEmptyStatusIsRejected) {
    EXPECT_EQ(classifyAck(0, true), AckVerdict::Rejected);
}

TEST(Ack, AckWinsOverBusy) {
    EXPECT_EQ(classifyAck(kDdeAckFAck | kDdeAckFBusy, true), AckVerdict::Accepted);
}

TEST(ItemName, MatchesTheSentCommand) {
    EXPECT_TRUE(itemNameMatches("GetSag,1,0.5,0.5", "GetSag,1,0.5,0.5"));
    EXPECT_FALSE(itemNameMatches("GetSag,1,0.5,0.5", "GetSag,1,0.6,0.5"));
    EXPECT_FALSE(itemNameMatches("GetSag,1,0.5,0.5", "GetSag,1,0.5,0.5 "));
}

TEST(ItemName, EmptyNamesNeverMatch) {
    // An unreadable item name must not be mistaken for a match, and an empty
    // command is never a valid item.
    EXPECT_FALSE(itemNameMatches("", "GetName"));
    EXPECT_FALSE(itemNameMatches("GetName", ""));
    EXPECT_FALSE(itemNameMatches("", ""));
}

TEST(AtomName, InvalidAtomYieldsEmptyName) {
    EXPECT_EQ(atomName(0), "");
}

} // namespace
} // namespace ZemaxDDE
