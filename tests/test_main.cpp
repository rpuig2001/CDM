#include <gtest/gtest.h>
#include "core/StartupRequests.hpp"
#include "core/CtotScratchpad.hpp"

TEST(StartupRequests, ConfirmedRemovalOnlyRemovesStartupRequests) {
    std::unordered_map<std::string, std::string> requests = {
        {"REQUESTED", "STUP"}, {"CLEARED", "CLR-123.450"}, {"OTHER", "PUSH"}
    };

    EXPECT_TRUE(applyConfirmedStartupRemoval(requests, "REQUESTED"));
    EXPECT_TRUE(applyConfirmedStartupRemoval(requests, "CLEARED"));
    EXPECT_FALSE(applyConfirmedStartupRemoval(requests, "OTHER"));
    EXPECT_FALSE(applyConfirmedStartupRemoval(requests, "MISSING"));
    EXPECT_EQ(requests.size(), 1u);
    EXPECT_EQ(requests.at("OTHER"), "PUSH");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(CtotScratchpad, UpdatesCtotWithoutDiscardingFollowingRemarks) {
    EXPECT_EQ(ctot_scratchpad::setCtotRemark(".c1234 testremark", "2345"), ".C2345 testremark");
}

TEST(CtotScratchpad, AddsCtotBeforeExistingRemarks) {
    EXPECT_EQ(ctot_scratchpad::setCtotRemark("testremark", "2345"), ".C2345 testremark");
}

TEST(CtotScratchpad, RemovesOnlyCtotAndItsSeparator) {
    EXPECT_EQ(ctot_scratchpad::removeCtotRemark(".C1234 testremark"), ".testremark");
    EXPECT_EQ(ctot_scratchpad::removeCtotRemark(".c1234 additionalremarks"), ".additionalremarks");
    EXPECT_EQ(ctot_scratchpad::removeCtotRemark(".c1234 .additionalremarks"), ".additionalremarks");
    EXPECT_EQ(ctot_scratchpad::removeCtotRemark(".additionalremarks .C1234"), ".additionalremarks");
    EXPECT_EQ(ctot_scratchpad::removeCtotRemark(".C1234"), "");
    EXPECT_EQ(ctot_scratchpad::removeCtotRemark("testremark"), "testremark");
}