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
// --------------------------------------------------------------------------
#ifndef HttpAPIGateway_H_
#define HttpAPIGateway_H_
// --------------------------------------------------------------------------
#include <Poco/JSON/Object.h>
#include "UniSetTypes.h"
#include "DebugStream.h"
#include "UHttpRequestHandler.h"
#include "UHttpServer.h"
#include "UInterface.h"
#include "UHttpRouter.h"
// -------------------------------------------------------------------------
const std::string HTTP_API_GATEWAY_VERSION = "v01";
// -------------------------------------------------------------------------
namespace uniset3
{
    //------------------------------------------------------------------------------------------
    /*!
        \page  \page page_HttpAPIGateway Http-интерфейс для работы с объектами (HttpAPIGateway)

        - \ref sec_HttpAPIGateway_Comm
        - \ref sec_HttpAPIGateway_Conf

        \section sec_HttpAPIGateway_Comm Общее описание работы HttpAPIGateway
            HttpAPIGateway это http-сервер (сервис) который предоставляет некоторый интерфейс для работы
        с объектами (получение статистик, управление настройками). При этом внутри себя он задействует grpc-вызовы.
         \code
         http://host:port/api/v01/...
         \endcode

        \section sec_HttpAPIGateway_Conf Настройка работы HttpAPIGateway
        Для запуска HttpAPIGateway необходимо настроить на каком порту и сетевом интерфейсе будут приниматься запросы.
        Для этого необходимо в настройках прописать следующую секцию
        \code
        <UniSet>
          ...
          <HttpAPIGateway name="HttpAPIGateway" port="8008" host="0.0.0.0"/>
          ...
        </UniSet>
        \endcode
        - 0.0.0.0 - слушать на всех доступных интерфейсах

        Помимо этого, настройки можно указать в аргументах командной строки
        - `--api-host` - интерфейс на котором слушать запросы. По умолчанию: 0.0.0.0
        - `--api-port` - порт на котором слушать запросы. По умолчанию: 8008

        Длполнительные параметры:
        - `--api-max-queued num `     - Размер очереди запросов к http серверу. По умолчанию: 100.
        - `--api-max-threads num`     - Разрешённое количество потоков для http-сервера. По умолчанию: 3.
        - `--api-cors-allow addr`     - (CORS): Access-Control-Allow-Origin. Default: *
        - `--api-logs-add-levels any` - Управление логами
        - `--confile configure.xml`            - Файл с настройками
    */
    class HttpAPIGateway:
        public Poco::Net::HTTPRequestHandler
    {
        public:
            HttpAPIGateway( const std::string& name, int argc, const char* const* argv, const std::string& prefix );
            virtual ~HttpAPIGateway();

            /*! глобальная функция для инициализации объекта */
            static std::shared_ptr<HttpAPIGateway> init_apigateway( int argc, const char* const* argv, const std::string& prefix = "api-" );

            /*! глобальная функция для вывода help-а */
            static void help_print();

            inline std::shared_ptr<DebugStream> log()
            {
                return rlog;
            }

            virtual void handleRequest( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp ) override;

            void run();

        protected:
            virtual void initRouter();
            // REQUEST HANDLERS
            void requestMetrics( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp, const uniset3::UHttpContext& ctx );
            void requestResolve( Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp, const uniset3::UHttpContext& ctx );
            Poco::JSON::Object::Ptr respError( Poco::Net::HTTPServerResponse& resp, Poco::Net::HTTPResponse::HTTPStatus s, const std::string& message );

            std::shared_ptr<Poco::Net::HTTPServer> httpserv;
            std::string httpHost = { "" };
            int httpPort = { 8081 };
            std::string httpCORS_allow = { "*" };
            std::string httpReplyAddr = { "" };

            std::shared_ptr<DebugStream> rlog;
            std::string myname;
            uniset3::UInterface ui;
            uniset3::UHttpRouter router;

            class HttpAPIGatewayRequestHandlerFactory:
                public Poco::Net::HTTPRequestHandlerFactory
            {
                public:
                    explicit HttpAPIGatewayRequestHandlerFactory( HttpAPIGateway* r ): api(r) {}
                    virtual ~HttpAPIGatewayRequestHandlerFactory() = default;

                    virtual Poco::Net::HTTPRequestHandler* createRequestHandler( const Poco::Net::HTTPServerRequest& req ) override;

                private:
                    HttpAPIGateway* api;
            };

        private:
    };
    // ----------------------------------------------------------------------------------
} // end of namespace uniset3
//------------------------------------------------------------------------------------------
#endif
