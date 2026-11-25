#include <gtest/gtest.h>
#include "core/StartupRequests.hpp"

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