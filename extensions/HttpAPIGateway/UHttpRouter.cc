/*
 * Copyright (c) 2021 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
//-----------------------------------------------------------------------------
#include <Poco/URI.h>
#include "UniSetTypes.h"
#include "UHttpRouter.h"
//-----------------------------------------------------------------------------
using namespace uniset3;
//-----------------------------------------------------------------------------
UHttpRouter::UHttpRouter()
{
}
//-----------------------------------------------------------------------------
UHttpRouter::~UHttpRouter()
{

}
//-----------------------------------------------------------------------------
void UHttpContext::set_keys( const Keys& k )
{
    keys = k;
}
void UHttpContext::set_keys(Keys&& k)
{
    std::swap(keys, k);
}
//-----------------------------------------------------------------------------
std::string UHttpContext::key( const std::string& name ) const
{
    auto i = keys.find(name);

    if( i != keys.end() )
        return i->second;

    return "";
}
//-----------------------------------------------------------------------------
bool UHttpContext::key_exists( const std::string& name ) const
{
    return ( keys.find(name) != keys.end() );
}
//-----------------------------------------------------------------------------
struct UHandlerList::Handler
{
    Handler( const std::string& s, URequestHandler h ):
        path(s), handler(h) {}

    const UPath path;
    URequestHandler handler;
};

UHandlerList::UHandlerList()
{

}
//-----------------------------------------------------------------------------
UHandlerList::~UHandlerList()
{

}
//-----------------------------------------------------------------------------
UHandlerList& UHandlerList::add( const std::string& path, URequestHandler h )
{
    handlers.emplace_back(path, h);
    return *this;
}
//-----------------------------------------------------------------------------
bool UHandlerList::call( const std::string& path, Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& res) const
{
    UHttpContext ctx;
    UHttpContext::Keys keys;

    for( auto&& h : handlers )
    {
        if( h.path.compare(path, keys) )
        {
            ctx.set_keys(std::move(keys));
            h.handler(req, res, ctx);
            return true;
        }
    }

    return false;
}
//-----------------------------------------------------------------------------
UHandlerList& UHttpRouter::get()
{
    return hget;
}
//-----------------------------------------------------------------------------
UHandlerList& UHttpRouter::post()
{
    return hpost;
}
//-----------------------------------------------------------------------------
bool UHttpRouter::call( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& res ) const
{
    Poco::URI uri(req.getURI());

    if( req.getMethod() == Poco::Net::HTTPRequest::HTTP_GET )
        return hget.call(uri.getPath(), req, res);

    if( req.getMethod() == Poco::Net::HTTPRequest::HTTP_POST )
        return hpost.call(uri.getPath(), req, res);

    // throw?
    return false;
}
//-----------------------------------------------------------------------------
UPath::UPath( const std::string& path )
{
    segments = build(path);
}
//-----------------------------------------------------------------------------
UPath::~UPath()
{

}
//-----------------------------------------------------------------------------
std::string to_string( const UPath::PathMeta& m )
{
    return "{" + m.path + "," + m.key + "}";
}
//-----------------------------------------------------------------------------
bool UPath::PathMeta::operator==(const UPath::PathMeta& r) const
{
    return (path == r.path && key == r.key);
}
//-----------------------------------------------------------------------------
std::vector<UPath::PathMeta> UPath::build( const std::string& path, const char key_prefix )
{
    std::vector<UPath::PathMeta> ret;

    if( path.empty() )
        return ret;

    auto v = uniset3::split(path, '/');

    if( v.size() > 0 )
    {
        // exclude options
        auto pos = v[v.size() - 1].find('?');

        if( pos != std::string::npos )
            v[v.size() - 1] = v[v.size() - 1].substr(0, pos);
    }

    for( const auto& s : v )
    {
        PathMeta m;

        if( s[0] == key_prefix )
            m.key = s.substr(1); // :key -> key
        else
            m.path = s;

        ret.emplace_back( std::move(m) );
    }

    return ret;
}
//-----------------------------------------------------------------------------
bool UPath::compare(const std::string& path, UHttpContext::Keys& keys) const
{
    if( path.empty() || segments.empty() )
        return false;

    auto mdata = uniset3::split(path, '/');

    // exclude options
    if( path[path.size() - 1] != '/' )
    {
        auto pos = mdata[mdata.size() - 1].find('?');

        if( pos != std::string::npos )
            mdata[mdata.size() - 1] = mdata[mdata.size() - 1].substr(0, pos);
    }

    if( mdata.size() != segments.size() )
        return false;

    for( size_t i = 0; i < mdata.size(); i++ )
    {
        if( !segments[i].key.empty() )
        {
            keys[segments[i].key] = mdata[i];
            continue;
        }

        if( mdata[i] != segments[i].path )
            return false;
    }

    return true;
}
//-----------------------------------------------------------------------------
