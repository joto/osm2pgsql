#ifndef OSMIUM_AREA_MULTIPOLYGON_COLLECTOR_HPP
#define OSMIUM_AREA_MULTIPOLYGON_COLLECTOR_HPP

/*

This file is part of Osmium (https://osmcode.org/libosmium).

Copyright 2013-2025 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <osmium/area/stats.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/relations/collector.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <vector>

namespace osmium {

    namespace relations {
        class RelationMeta;
    } // namespace relations

    /**
     * @brief Code related to the building of areas (multipolygons) from relations.
     */
    namespace area {

        /**
         * This class collects all data needed for creating areas from
         * relations tagged with type=multipolygon or type=boundary.
         * Most of its functionality is derived from the parent class
         * osmium::relations::Collector.
         *
         * The actual assembling of the areas is done by the assembler
         * class given as template argument.
         *
         * @tparam TAssembler Multipolygon Assembler class.
         * @pre The Ids of all objects must be unique in the input data.
         *
         * @deprecated Use MultipolygonManager instead.
         */
        template <typename TAssembler>
        class MultipolygonCollector : public osmium::relations::Collector<MultipolygonCollector<TAssembler>, false, true, false> {

            using collector_type = osmium::relations::Collector<MultipolygonCollector<TAssembler>, false, true, false>;

            using assembler_config_type = typename TAssembler::config_type;
            assembler_config_type m_assembler_config;

            osmium::memory::Buffer m_output_buffer;

            area_stats m_stats;

            enum {
                initial_output_buffer_size = 1024UL * 1024UL
            };

            enum {
                max_buffer_size_for_flush = 100UL * 1024UL
            };

            void flush_output_buffer() {
                if (this->callback()) {
                    osmium::memory::Buffer buffer{initial_output_buffer_size};
                    using std::swap;
                    swap(buffer, m_output_buffer);
                    this->callback()(std::move(buffer));
                }
            }

            void possibly_flush_output_buffer() {
                if (m_output_buffer.committed() > max_buffer_size_for_flush) {
                    flush_output_buffer();
                }
            }

        public:

            explicit MultipolygonCollector(const assembler_config_type& assembler_config) :
                collector_type(),
                m_assembler_config(assembler_config),
                m_output_buffer(initial_output_buffer_size, osmium::memory::Buffer::auto_grow::yes) {
            }

            const area_stats& stats() const noexcept {
                return m_stats;
            }

            /**
             * We are interested in all relations tagged with type=multipolygon
             * or type=boundary.
             *
             * Overwritten from the base class.
             */
            bool keep_relation(const osmium::Relation& relation) const {
                const char* type = relation.tags().get_value_by_key("type");

                // ignore relations without "type" tag
                if (!type) {
                    return false;
                }

                return (!std::strcmp(type, "multipolygon")) || (!std::strcmp(type, "boundary"));
            }

            /**
             * Overwritten from the base class.
             */
            bool keep_member(const osmium::relations::RelationMeta& /*relation_meta*/, const osmium::RelationMember& member) const {
                // We are only interested in members of type way.
                return member.type() == osmium::item_type::way;
            }

            /**
             * This is called when a way is not in any multipolygon
             * relation.
             *
             * Overwritten from the base class.
             */
            void way_not_in_any_relation(const osmium::Way& way) {
                // you need at least 4 nodes to make up a polygon
                if (way.nodes().size() <= 3) {
                    return;
                }
                try {
                    if (!way.nodes().front().location() || !way.nodes().back().location()) {
                        throw osmium::invalid_location{"invalid location"};
                    }
                    if (way.ends_have_same_location()) {
                        // way is closed and has enough nodes, build simple multipolygon
                        TAssembler assembler{m_assembler_config};
                        assembler(way, m_output_buffer);
                        m_stats += assembler.stats();
                        possibly_flush_output_buffer();
                    }
                } catch (const osmium::invalid_location&) {
                    // XXX ignore
                }
            }

            void complete_relation(const osmium::relations::RelationMeta& relation_meta) {
                const osmium::Relation& relation = this->get_relation(relation_meta);
                const osmium::memory::Buffer& buffer = this->members_buffer();

                std::vector<const osmium::Way*> ways;
                ways.reserve(relation.members().size());
                for (const auto& member : relation.members()) {
                    if (member.ref() != 0) {
                        const size_t offset = this->get_offset(member.type(), member.ref());
                        ways.push_back(&buffer.get<const osmium::Way>(offset));
                    }
                }

                try {
                    TAssembler assembler{m_assembler_config};
                    assembler(relation, ways, m_output_buffer);
                    m_stats += assembler.stats();
                    possibly_flush_output_buffer();
                } catch (const osmium::invalid_location&) {
                    // XXX ignore
                }
            }

            void flush() {
                flush_output_buffer();
            }

            osmium::memory::Buffer read() {
                osmium::memory::Buffer buffer{initial_output_buffer_size, osmium::memory::Buffer::auto_grow::yes};

                using std::swap;
                swap(buffer, m_output_buffer);

                return buffer;
            }

        }; // class MultipolygonCollector

    } // namespace area

} // namespace osmium

#endif // OSMIUM_AREA_MULTIPOLYGON_COLLECTOR_HPP
