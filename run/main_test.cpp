
#include "gtest/gtest.h" // 引入 GoogleTest

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}