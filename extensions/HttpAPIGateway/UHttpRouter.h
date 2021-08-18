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
#ifndef UHttpRouter_H_
#define UHttpRouter_H_
//-----------------------------------------------------------------------------
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
//-----------------------------------------------------------------------------
namespace uniset3
{
    //-------------------------------------------------------------------------
    class UHttpContext;
    typedef std::function<void(Poco::Net::HTTPServerRequest&, Poco::Net::HTTPServerResponse&, const UHttpContext&)> URequestHandler;
    //-------------------------------------------------------------------------
    class UHttpContext
    {
        public:
            UHttpContext() = default;
            ~UHttpContext() = default;

            typedef std::unordered_map<std::string, std::string> Keys;

            std::string key( const std::string& name ) const;
            bool key_exists( const std::string& name ) const;

        private:
            Keys keys;
    };
    //-------------------------------------------------------------------------
    /*! path examples:
     * /root/:key1/path/index
     * /root/:key1/path/:key2
     * /root/:key1/path/:key2/:key3
     * /root/:key1/path/:key2?option1
     */
    class UPath
    {
        public:
            UPath( const std::string& path );
            ~UPath();

            struct PathMeta
            {
                std::string path = { "" };
                std::string key = { "" };
                bool operator==(const PathMeta& r) const;
            };

            friend std::string to_string( const uniset3::UPath::PathMeta& m );

            /*! path: /root/:key1/path/:key2/query?options -> [root,"",path,"", query] (exclude options) */
            static std::vector<PathMeta> build( const std::string& path, const char key_prefix = ':' );

            bool compare( const std::string& path, UHttpContext::Keys& keys ) const;
        private:
            std::vector<PathMeta> segments;
    };


    //-------------------------------------------------------------------------

    class UHandlerList
    {
        public:
            UHandlerList();
            ~UHandlerList();

            UHandlerList& add( const std::string& path, URequestHandler h );
            bool call( const std::string& path, Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& res) const;

        private:
            struct Handler;
            std::vector<Handler> handlers;
    };
    //-------------------------------------------------------------------------
    /*! Работа с роутами */
    class UHttpRouter
    {
        public:
            UHttpRouter();
            virtual ~UHttpRouter();

            UHandlerList& get();
            UHandlerList& post();

            bool call( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& res) const;

        protected:
            UHandlerList hget;
            UHandlerList hpost;

        private:
    };
    //-------------------------------------------------------------------------
} // end of namespace uniset3
//-----------------------------------------------------------------------------
#endif
