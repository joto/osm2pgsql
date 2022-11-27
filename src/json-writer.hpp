#ifndef OSM2PGSQL_JSON_WRITER_HPP
#define OSM2PGSQL_JSON_WRITER_HPP

/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This file is part of osm2pgsql (https://osm2pgsql.org/).
 *
 * Copyright (C) 2006-2022 by the osm2pgsql developer community.
 * For a full list of authors see the git log.
 */

#include "format.hpp"

#include <cassert>
#include <iterator>
#include <string>

class json_writer_t
{
public:
    void null() { m_buffer.append("null"); }

    void boolean(bool value) { m_buffer.append(value ? "true" : "false"); }

    template <typename T>
    void number(T value)
    {
        fmt::format_to(std::back_inserter(m_buffer), "{}"_format(value));
    }

    void string(char const *str)
    {
        m_buffer += '"';
        while (auto const c = *str++) {
            switch (c) {
            case '\b':
                m_buffer.append("\\b");
                break;
            case '\f':
                m_buffer.append("\\f");
                break;
            case '\n':
                m_buffer.append("\\n");
                break;
            case '\r':
                m_buffer.append("\\r");
                break;
            case '\t':
                m_buffer.append("\\t");
                break;
            case '"':
                m_buffer.append("\\\"");
                break;
            case '\\':
                m_buffer.append("\\\\");
                break;
            default:
                if (c <= 0x1f) {
                    m_buffer.append(
                        R"(\u{:04x})"_format(static_cast<unsigned char>(c)));
                } else {
                    m_buffer += c;
                }
            }
        }
        m_buffer += '"';
    }

    void key(char const *key)
    {
        string(key);
        m_buffer += ':';
    }

    void start_object() { m_buffer += '{'; }

    void end_object()
    {
        assert(!m_buffer.empty());
        if (m_buffer.back() == ',') {
            m_buffer.back() = '}';
        } else {
            m_buffer += '}';
        }
    }

    void start_array() { m_buffer += '['; }

    void end_array()
    {
        assert(!m_buffer.empty());
        if (m_buffer.back() == ',') {
            m_buffer.back() = ']';
        } else {
            m_buffer += ']';
        }
    }

    void next() { m_buffer += ','; }

    std::string const &json() const noexcept { return m_buffer; }

private:
    std::string m_buffer;
};

#endif // OSM2PGSQL_JSON_WRITER_HPP
