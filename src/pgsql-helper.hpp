#ifndef OSM2PGSQL_PGSQL_HELPER_HPP
#define OSM2PGSQL_PGSQL_HELPER_HPP

/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2025 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include "idlist.hpp"
#include "osmtypes.hpp"

#include <string>

class pg_conn_t;
class pg_result_t;


/**
 * Iterate over the result from a pgsql query and generate a list of all the
 * ids from the first column.
 *
 * \param result The result to iterate over.
 * \returns A list of ids.
 */
idlist_t get_ids_from_result(pg_result_t const &result);

void create_geom_check_trigger(pg_conn_t const &db_connection,
                               std::string const &schema,
                               std::string const &table,
                               std::string const &condition);

void drop_geom_check_trigger(pg_conn_t const &db_connection,
                             std::string const &schema,
                             std::string const &table);

void analyze_table(pg_conn_t const &db_connection, std::string const &schema,
                   std::string const &name);

void drop_table_if_exists(pg_conn_t const &db_connection,
                          std::string const &schema, std::string const &name);

#endif // OSM2PGSQL_PGSQL_HELPER_HPP
