/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2025 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include <catch.hpp>

#include "common-import.hpp"
#include "common-options.hpp"

namespace {

testing::db::import_t db;

} // anonymous namespace

TEST_CASE("default projection")
{
    options_t const options = testing::opt_t().slim();

    REQUIRE_NOTHROW(db.run_file(options, "test_output_pgsql_area.osm"));

    auto conn = db.db().connect();

    REQUIRE(2 == conn.get_count("osm2pgsql_test_polygon"));
    conn.assert_double(
        1.23927e+10,
        "SELECT way_area FROM osm2pgsql_test_polygon WHERE name='poly'");
    conn.assert_double(
        9.91828e+10,
        "SELECT way_area FROM osm2pgsql_test_polygon WHERE name='multi'");
}

TEST_CASE("latlon projection")
{
    options_t const options = testing::opt_t().slim().srs(PROJ_LATLONG);

    REQUIRE_NOTHROW(db.run_file(options, "test_output_pgsql_area.osm"));

    auto conn = db.db().connect();

    REQUIRE(2 == conn.get_count("osm2pgsql_test_polygon"));
    conn.assert_double(
        1, "SELECT way_area FROM osm2pgsql_test_polygon WHERE name='poly'");
    conn.assert_double(
        8, "SELECT way_area FROM osm2pgsql_test_polygon WHERE name='multi'");
}

TEST_CASE("latlon projection with way_area reprojection")
{
    options_t options = testing::opt_t().slim().srs(PROJ_LATLONG);
    options.reproject_area = true;

    REQUIRE_NOTHROW(db.run_file(options, "test_output_pgsql_area.osm"));

    auto conn = db.db().connect();

    REQUIRE(2 == conn.get_count("osm2pgsql_test_polygon"));
    conn.assert_double(
        1.23927e+10,
        "SELECT way_area FROM osm2pgsql_test_polygon WHERE name='poly'");
    conn.assert_double(
        9.91828e+10,
        "SELECT way_area FROM osm2pgsql_test_polygon WHERE name='multi'");
}
