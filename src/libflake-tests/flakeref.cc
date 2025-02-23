#include <gtest/gtest.h>

#include "fetch-settings.hh"
#include "flake/flakeref.hh"

#ifndef _WIN32
#define BASE_PATH "/foo/bar"
#else
#define BASE_PATH "C:/foo/bar"
#endif

namespace nix {

/* ----------- tests for flake/flakeref.hh --------------------------------------------------*/

    TEST(parseFlakeRef, path) {
        experimentalFeatureSettings.experimentalFeatures.get().insert(Xp::Flakes);

        fetchers::Settings fetchSettings;

        {
            auto s = BASE_PATH;
            auto flakeref = parseFlakeRef(fetchSettings, s);
            ASSERT_EQ(flakeref.to_string(), "path:" BASE_PATH);
        }

        {
            auto s = BASE_PATH "?revCount=123&rev=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
            auto flakeref = parseFlakeRef(fetchSettings, s);
            ASSERT_EQ(flakeref.to_string(), "path:" BASE_PATH "?rev=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa&revCount=123");
        }

        {
            auto s = BASE_PATH "?xyzzy=123";
            EXPECT_THROW(
                parseFlakeRef(fetchSettings, s),
                Error);
        }

        {
            auto s = BASE_PATH "#bla";
            EXPECT_THROW(
                parseFlakeRef(fetchSettings, s),
                Error);
        }

        {
            auto s = BASE_PATH "#bla";
            auto [flakeref, fragment] = parseFlakeRefWithFragment(fetchSettings, s);
            ASSERT_EQ(flakeref.to_string(), "path:" BASE_PATH);
            ASSERT_EQ(fragment, "bla");
        }

        {
            auto s = BASE_PATH "?revCount=123#bla";
            auto [flakeref, fragment] = parseFlakeRefWithFragment(fetchSettings, s);
            ASSERT_EQ(flakeref.to_string(), "path:" BASE_PATH "?revCount=123");
            ASSERT_EQ(fragment, "bla");
        }
    }

    TEST(to_string, doesntReencodeUrl) {
        fetchers::Settings fetchSettings;
        auto s = "http://localhost:8181/test/+3d.tar.gz";
        auto flakeref = parseFlakeRef(fetchSettings, s);
        auto unparsed = flakeref.to_string();
        auto expected = "http://localhost:8181/test/%2B3d.tar.gz";

        ASSERT_EQ(unparsed, expected);
    }

}
