/*
 * Copyright (c) 2015 Pavel Vainerman.
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
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include "UInterface.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "IOController.grpc.pb.h"
#include "UniSetObject.grpc.pb.h"
#include "UniSetManager.grpc.pb.h"

// -----------------------------------------------------------------------------
namespace uniset3
{
    // -----------------------------------------------------------------------------
    using namespace std;
    // -----------------------------------------------------------------------------
    UInterface::UInterface( const std::shared_ptr<uniset3::Configuration>& _uconf ):
        myid(uniset3::DefaultObjectId),
        rcache(100, 20),
        oind(_uconf->oind),
        uconf(_uconf)
    {
        init();
    }
    // -----------------------------------------------------------------------------
    UInterface::UInterface( const uniset3::ObjectId backid, const shared_ptr<uniset3::ObjectIndex> _oind ):
        myid(backid),
        rcache(200, 40),
        oind(_oind),
        uconf(uniset3::uniset_conf())
    {
        if( oind == nullptr )
            oind = uconf->oind;

        init();
    }

    UInterface::~UInterface()
    {
    }

    void UInterface::init()
    {
    }
    // ------------------------------------------------------------------------------------------------------------
    void UInterface::initBackId( const uniset3::ObjectId backid )
    {
        myid = backid;
    }
    // ------------------------------------------------------------------------------------------------------------
    /*!
     * \param id - идентификатор датчика
     * \return текущее значение датчика
     * \exception IOBadParam - генерируется если указано неправильное имя датчика или секции
     * \exception IOTimeOut - генерируется если в течение времени timeout не был получен ответ
    */
    long UInterface::getValue( const uniset3::ObjectId id, const uniset3::ObjectId node ) const
    {
        if ( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getValue): error id=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getValue): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            std::shared_ptr<grpc::Channel> chan;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            google::protobuf::Int64Value request;
            request.set_value(id);
            google::protobuf::Int64Value reply;
            grpc::ClientContext ctx;

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve( id, node );

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->getValue(&ctx, request, &reply);

                if( st.ok() )
                    return reply.value();

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(getValue): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(getValue): TimeOut", id, node));
    }

    long UInterface::getValue( const uniset3::ObjectId name ) const
    {
        return getValue(name, uconf->getLocalNode());
    }


    // ------------------------------------------------------------------------------------------------------------
    void UInterface::setUndefinedState( const uniset3::SensorInfo& si, bool undefined, uniset3::ObjectId sup_id )
    {
        if( si.id() == uniset3::DefaultObjectId )
        {
            uwarn << "UI(setUndefinedState): ID=uniset3::DefaultObjectId" << endl;
            return;
        }

        if( sup_id == uniset3::DefaultObjectId )
            sup_id = myid;

        try
        {
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::Empty reply;
            SetUndefinedParams request;
            request.set_id(si.id());
            request.set_sup_id(sup_id);
            request.set_undefined(undefined);
            grpc::ClientContext ctx;

            try
            {
                chan = rcache.resolve(si.id(), si.node());
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve( si.id(), si.node() );

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->setUndefinedState(&ctx, request, &reply);

                if( st.ok() )
                    return;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(si.id(), si.node());
            throw uniset3::SystemError("UI(setUndefinedState): " + string(ex.what()));
        }

        rcache.erase(si.id(), si.node());
        uwarn << set_err("UI(setUndefinedState): Timeout", si.id(), si.node()) << endl;
    }
    // ------------------------------------------------------------------------------------------------------------
    void UInterface::freezeValue( const uniset3::SensorInfo& si, bool set, long value, uniset3::ObjectId sup_id )
    {
        if( si.id() == uniset3::DefaultObjectId )
        {
            uwarn << "UI(freezeValue): ID=uniset3::DefaultObjectId" << endl;
            return;
        }

        if( sup_id == uniset3::DefaultObjectId )
            sup_id = myid;

        try
        {
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::Empty reply;
            FreezeValueParams request;
            request.set_id(si.id());
            request.set_sup_id(sup_id);
            request.set_set(set);
            request.set_value(value);
            grpc::ClientContext ctx;

            try
            {
                chan = rcache.resolve(si.id(), si.node());
            }
            catch( const uniset3::NameNotFound& ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(si.id(), si.node());

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->freezeValue(&ctx, request, &reply);

                if( st.ok() )
                    return;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(si.id(), si.node());
            throw uniset3::SystemError("UI(freezeValue): " + string(ex.what()));
        }

        rcache.erase(si.id(), si.node());
        uwarn << set_err("UI(freezeValue): Timeout", si.id(), si.node()) << endl;
    }
    // ------------------------------------------------------------------------------------------------------------
    /*!
     * \param id - идентификатор датчика
     * \param value - значение, которое необходимо установить
     * \return текущее значение датчика
     * \exception IOBadParam - генерируется, если указано неправильное имя вывода или секции
    */
    void UInterface::setValue( const uniset3::ObjectId id, long value, const uniset3::ObjectId node, const uniset3::ObjectId sup_id ) const
    {
        if ( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(setValue): error: id=uniset3::DefaultObjectId");

        if ( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(setValue): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        /*
            if ( sup_id == uniset3::DefaultObjectId )
            {
                ostringstream err;
                err << "UI(setValue): id='" << id << "' error: supplier=uniset3::DefaultObjectId";
                throw uniset3::ORepFailed(err.str());
            }
        */
        try
        {
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::Empty reply;
            SetValueParams request;
            request.set_id(id);
            request.set_sup_id(sup_id);
            request.set_value(value);
            grpc::ClientContext ctx;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->setValue(&ctx, request, &reply);

                if( st.ok() )
                    return;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(setValue): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(setValue): Timeout", id, node));
    }

    void UInterface::setValue( const uniset3::ObjectId name, long value ) const
    {
        setValue(name, value, uconf->getLocalNode(), myid);
    }

    void UInterface::setValue( const uniset3::SensorInfo& si, long value, const uniset3::ObjectId sup_id ) const
    {
        setValue(si.id(), value, si.node(), sup_id);
    }

    // ------------------------------------------------------------------------------------------------------------
    /*!
     * \param id     - идентификатор датчика
     * \param node        - идентификатор узла на котором заказывается датчик
     * \param cmd - команда см. \ref uniset3::UIOCommand
     * \param backid - обратный адрес (идентификатор заказчика)
    */
    void UInterface::askRemoteSensor( const uniset3::ObjectId id, uniset3::UIOCommand cmd,
                                      const uniset3::ObjectId node,
                                      uniset3::ObjectId backid ) const
    {
        if( backid == uniset3::DefaultObjectId )
            backid = myid;

        if( backid == uniset3::DefaultObjectId )
            throw uniset3::IOBadParam("UI(askRemoteSensor): unknown back ID");

        if ( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(askRemoteSensor): error: id=uniset3::DefaultObjectId");

        if ( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(askRemoteSensor): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::Empty reply;
            AskParams request;
            request.set_sid(id);
            request.mutable_ci()->set_id(backid);
            request.mutable_ci()->set_node(uconf->getLocalNode());
            grpc::ClientContext ctx;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(id, node);

                std::unique_ptr<IONotifyController_i::Stub> stub(IONotifyController_i::NewStub(chan));
                grpc::Status st = stub->askSensor(&ctx, request, &reply);

                if( st.ok() )
                    return;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(askSensor): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(askSensor): Timeout", id, node));
    }

    void UInterface::askSensor( const uniset3::ObjectId name, uniset3::UIOCommand cmd, const uniset3::ObjectId backid ) const
    {
        askRemoteSensor(name, cmd, uconf->getLocalNode(), backid);
    }

    // ------------------------------------------------------------------------------------------------------------
    /*!
     * \param id - идентификатор объекта
     * \param node - идентификатор узла
    */
    IOType UInterface::getIOType( const uniset3::ObjectId id, const uniset3::ObjectId node ) const
    {
        if ( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getIOType): error: id=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getIOType): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            std::shared_ptr<grpc::Channel> chan;
            RetIOType reply;
            google::protobuf::Int64Value request;
            request.set_value(id);
            grpc::ClientContext ctx;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->getIOType(&ctx, request, &reply);

                if( st.ok() )
                    return reply.type();

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(getIOType): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(getIOType): Timeout", id, node));
    }

    IOType UInterface::getIOType( const uniset3::ObjectId id ) const
    {
        return getIOType(id, uconf->getLocalNode() );
    }
    // ------------------------------------------------------------------------------------------------------------
    /*!
     * \param id - идентификатор объекта
     * \param node - идентификатор узла
    */
    uniset3::ObjectType UInterface::getType( const uniset3::ObjectId id, const uniset3::ObjectId node) const
    {
        if ( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getType): попытка обратиться к объекту с id=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getType): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::StringValue reply;
            google::protobuf::Empty request;
            grpc::ClientContext ctx;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                std::unique_ptr<UniSetObject_i::Stub> stub(UniSetObject_i::NewStub(chan));
                grpc::Status st = stub->getType(&ctx, request, &reply);

                if( st.ok() )
                    return reply.value();

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(getType): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(getType): Timeout", id, node));
    }

    uniset3::ObjectType UInterface::getType( const uniset3::ObjectId id ) const
    {
        return getType(id, uconf->getLocalNode());
    }

    // ------------------------------------------------------------------------------------------------------------
    void UInterface::registered( const uniset3::ObjectId id, const uniset3::ObjectRef oRef, bool force ) const
    {
        // если включён режим использования локальных файлов
        // то пишем IOR в файл
        if( uconf->isLocalIOR() )
        {
            uconf->iorfile->setIOR(id, IORFile::makeIOR(oRef));
            return;
        }

        google::protobuf::Empty reply;
        grpc::ClientContext ctx;

        for( size_t i = 0; i < uconf->getRepeatCount(); i++ )
        {
            if( !rep )
                rep = grpc::CreateChannel(uconf->repositoryAddress(), grpc::InsecureChannelCredentials());

            std::unique_ptr<Repository_i::Stub> stub(Repository_i::NewStub(rep));
            grpc::Status st = stub->registration(&ctx, oRef, &reply);

            if( st.ok() )
                return;

            msleep(uconf->getRepeatTimeout());
            rep = nullptr;
        }
    }

    // ------------------------------------------------------------------------------------------------------------
    void UInterface::unregister( const uniset3::ObjectId id )
    {
        if( uconf->isLocalIOR() )
        {
            uconf->iorfile->unlinkIOR(id);
            return;
        }

        google::protobuf::Empty reply;
        google::protobuf::Int64Value request;
        request.set_value(id);
        grpc::ClientContext ctx;

        for( size_t i = 0; i < uconf->getRepeatCount(); i++ )
        {
            if( !rep )
                rep = grpc::CreateChannel(uconf->repositoryAddress(), grpc::InsecureChannelCredentials());

            std::unique_ptr<Repository_i::Stub> stub(Repository_i::NewStub(rep));
            grpc::Status st = stub->unregistration(&ctx, request, &reply);

            if( st.ok() )
                return;

            msleep(uconf->getRepeatTimeout());
            rep = nullptr;
        }
    }

    // ------------------------------------------------------------------------------------------------------------
    std::shared_ptr<grpc::Channel> UInterface::resolve( const uniset3::ObjectId rid, const uniset3::ObjectId node ) const
    {
        if( rid == uniset3::DefaultObjectId )
            throw uniset3::ResolveNameError("UI(resolve): ID=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(resolve): id='" << rid << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ResolveNameError(err.str());
        }

        rcache.erase(rid, node);
#if 0

        try
        {
            if( uconf->isLocalIOR() )
            {
                if( CORBA::is_nil(orb) )
                    orb = uconf->getORB();

                string sior;

                if( node == uconf->getLocalNode() )
                    sior = uconf->iorfile->getIOR(rid);
                else
                    sior = httpResolve(rid, node);

                if( !sior.empty() )
                {
                    CORBA::Object_var nso = orb->string_to_object(sior.c_str());
                    rcache.cache(rid, node, nso); // заносим в кэш
                    return nso._retn();
                }

                uwarn << "not found IOR-file for " << uconf->oind->getNameById(rid)
                      << " node=" << uconf->oind->getNameById(node)
                      << endl;
                throw uniset3::ResolveNameError();
            }

            if( node != uconf->getLocalNode() )
            {
                // Получаем доступ к NameService на данном узле
                ostringstream s;
                s << uconf << oind->getNodeName(node);
                string nodeName(s.str());
                const string bname(nodeName); // сохраняем базовое название

                for( size_t curNet = 1; curNet <= uconf->getCountOfNet(); curNet++)
                {
                    try
                    {
                        if( CORBA::is_nil(orb) )
                            orb = uconf->getORB();

                        ctx = ORepHelpers::getRootNamingContext( orb, nodeName );
                        break;
                    }
                    //                catch( const CORBA::COMM_FAILURE& ex )
                    catch( const uniset3::ORepFailed& ex )
                    {
                        // нет связи с этим узлом
                        // пробуем связаться по другой сети
                        // ПО ПРАВИЛАМ узел в другой должен иметь имя NodeName1...NodeNameX
                        ostringstream s;
                        s << bname << curNet;
                        nodeName = s.str();
                    }
                }

                if( CORBA::is_nil(ctx) )
                {
                    // uwarn << "NameService недоступен на узле "<< node << endl;
                    throw uniset3::NSResolveError();
                }
            }
            else
            {
                if( CORBA::is_nil(localctx) )
                {
                    ostringstream s;
                    s << uconf << oind->getNodeName(node);
                    const string nodeName(s.str());

                    if( CORBA::is_nil(orb) )
                    {
                        CORBA::ORB_var _orb = uconf->getORB();
                        localctx = ORepHelpers::getRootNamingContext( _orb, nodeName );
                    }
                    else
                        localctx = ORepHelpers::getRootNamingContext( orb, nodeName );
                }

                ctx = localctx;
            }

            CosNaming::Name_var oname = omniURI::stringToName( oind->getNameById(rid).c_str() );

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                try
                {
                    CORBA::Object_var nso = ctx->resolve(oname);

                    if( CORBA::is_nil(nso) )
                        throw uniset3::ResolveNameError();

                    // Для var
                    rcache.cache(rid, node, nso); // заносим в кэш
                    return nso._retn();
                }
                catch( const CORBA::TRANSIENT& ) {}

                msleep(uconf->getRepeatTimeout());
            }

            throw uniset3::TimeOut();
        }
        catch(const CosNaming::NamingContext::NotFound& nf) {}
        catch(const CosNaming::NamingContext::InvalidName& nf) {}
        catch(const CosNaming::NamingContext::CannotProceed& cp) {}
        catch( const CORBA::OBJECT_NOT_EXIST& ex )
        {
            throw uniset3::ResolveNameError("ObjectNOTExist");
        }
        catch( const CORBA::COMM_FAILURE& ex )
        {
            throw uniset3::ResolveNameError("CORBA::CommFailure");
        }
        catch( const CORBA::SystemException& ex )
        {
            // ошибка системы коммуникации
            // uwarn << "UI(resolve): CORBA::SystemException" << endl;
            throw uniset3::TimeOut();
        }
        catch( const uniset3::Exception& ex ) {}
        catch( std::exception& ex )
        {
            ucrit << "UI(resolve): myID=" << myid <<  ": resolve id=" << rid << "@" << node
                  << " catch " << ex.what() << endl;
        }

#endif
        throw uniset3::ResolveNameError();
    }

    // -------------------------------------------------------------------------------------------
    std::string UInterface::httpResolve( const uniset3::ObjectId id, const uniset3::ObjectId node ) const
    {
#ifndef DISABLE_REST_API
        size_t port = uconf->getHttpResovlerPort();

        if( port == 0 )
            return "";

        const std::string host = uconf->getNodeIp(node);

        if( host.empty() )
            return "";

        string ret;
        const string query = "api/v01/resolve/text?" + std::to_string(id);

        for( size_t i = 0; i < uconf->getRepeatCount(); i++ )
        {
            ret = resolver.get(host, port, query);

            if( !ret.empty() )
                return ret;

            msleep(uconf->getRepeatTimeout());
        }

#endif
        return "";
    }
    // -------------------------------------------------------------------------------------------
    void UInterface::send(const uniset3::messages::TransportMessage& msg, uniset3::ObjectId node)
    {
        ObjectId id = msg.header().consumer();

        if( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(send): ERROR: id=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(send): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::Empty reply;
            grpc::ClientContext ctx;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                std::unique_ptr<UniSetObject_i::Stub> stub(UniSetObject_i::NewStub(chan));
                grpc::Status st = stub->push(&ctx, msg, &reply);

                if( st.ok() )
                    return;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(send): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(send): Timeout", id, node));
    }
    // ------------------------------------------------------------------------------------------------------------
    void UInterface::sendText(const ObjectId name, const std::string& txt, int mtype, const ObjectId node )
    {
        if ( name == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(sendText): ERROR: id=uniset3::DefaultObjectId");

        uniset3::ObjectId onode = (node == uniset3::DefaultObjectId) ? uconf->getLocalNode() : node;

        uniset3::messages::TextMessage msg;
        auto header = msg.mutable_header();
        header->set_type(uniset3::messages::mtTextInfo);
        header->set_priority(uniset3::messages::mpMedium);
        header->set_node(uconf->getLocalNode());
        header->set_supplier(myid);
        header->set_consumer(name);
        auto ts = uniset3::now_to_uniset_timespec();
        (*header->mutable_ts()) = ts;
        msg.set_msg(txt);
        msg.set_mtype(mtype);
        sendText(msg, onode);
    }
    // ------------------------------------------------------------------------------------------------------------
    void UInterface::sendText( const uniset3::messages::TextMessage& msg, uniset3::ObjectId node )
    {
        if( msg.header().consumer() == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(sendText): ERROR: consumer=uniset3::DefaultObjectId");

        ObjectId id = msg.header().consumer();

        uniset3::messages::TransportMessage tm;
        auto header = tm.mutable_header();
        header->set_type(msg.header().type());
        header->set_priority(msg.header().priority());
        header->set_node(uconf->getLocalNode());
        header->set_supplier(myid);
        header->set_consumer(id);
        auto ts = uniset3::now_to_uniset_timespec();
        (*header->mutable_ts()) = ts;
        tm.set_data(msg.SerializeAsString());
        send(tm, node);
    }

    // ------------------------------------------------------------------------------------------------------------
    uniset3::ShortIOInfo UInterface::getTimeChange( const uniset3::ObjectId id, const uniset3::ObjectId node ) const
    {
        if( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getTimeChange): Unknown id=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getTimeChange): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            std::shared_ptr<grpc::Channel> chan;
            ShortIOInfo reply;
            google::protobuf::Int64Value request;
            request.set_value(id);
            grpc::ClientContext ctx;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->getTimeChange(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(getTimeChange): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(getTimeChange): Timeout", id, node));
    }

    // ------------------------------------------------------------------------------------------------------------
    std::string UInterface::getObjectInfo( const ObjectId id, const std::string& params, const ObjectId node ) const
    {
        if( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getInfo): Unknown id=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getInfo): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::StringValue reply;
            google::protobuf::StringValue request;
            request.set_value(params);
            grpc::ClientContext ctx;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                std::unique_ptr<UniSetObject_i::Stub> stub(UniSetObject_i::NewStub(chan));
                grpc::Status st = stub->getInfo(&ctx, request, &reply);

                if( st.ok() )
                    return reply.value();

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(getTimeChange): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(getInfo): Timeout", id, node));
    }
    // ------------------------------------------------------------------------------------------------------------
    string UInterface::apiRequest(const ObjectId id, const string& query, const ObjectId node) const
    {
        if( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(apiRequest): Unknown id=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(apiRequest): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::StringValue reply;
            google::protobuf::StringValue request;
            request.set_value(query);
            grpc::ClientContext ctx;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                std::unique_ptr<UniSetObject_i::Stub> stub(UniSetObject_i::NewStub(chan));
                grpc::Status st = stub->request(&ctx, request, &reply);

                if( st.ok() )
                    return reply.value();

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(apiRequest): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(apiRequest): Timeout", id, node));
    }
    // ------------------------------------------------------------------------------------------------------------
    std::shared_ptr<grpc::Channel> UInterface::resolve( const std::string& name ) const
    {
#warning Not realized yet!
        return nullptr;
#if 0

        if( uconf->isLocalIOR() )
        {
            params

            if( CORBA::is_nil(orb) )
                orb = uconf->getORB();

            const string sior( uconf->iorfile->getIOR(oind->getIdByName(name)) );

            if( !sior.empty() )
                return orb->string_to_object(sior.c_str());
        }

        return rep.resolve( name );
#endif
    }
    // ------------------------------------------------------------------------------------------------------------
    std::shared_ptr<grpc::Channel> UInterface::resolve( const uniset3::ObjectId id ) const
    {
#warning Not realized yet!
        return nullptr;
#if 0

        if( uconf->isLocalIOR() )
        {
            if( CORBA::is_nil(orb) )
                orb = uconf->getORB();

            const string sior( uconf->iorfile->getIOR(id) );

            if( !sior.empty() )
                return orb->string_to_object(sior.c_str());
        }

        return rep.resolve( oind->getNameById(id) );
#endif
    }
    // ------------------------------------------------------------------------------------------------------------
    std::shared_ptr<grpc::Channel> UInterface::CacheOfResolve::resolve( const uniset3::ObjectId id, const uniset3::ObjectId node ) const
    {
        try
        {
            uniset3::uniset_rwmutex_rlock l(cmutex);

            auto it = mcache.find( uniset3::key(id, node) );

            if( it != mcache.end() )
            {
                it->second.ncall++;
                return it->second.chan;
            }
        }
        catch( std::exception& ex )
        {
            uwarn << "UI(CacheOfResolve::resolve): exception: " << ex.what() << endl;
            throw uniset3::SystemError(ex.what());
        }

        throw uniset3::NameNotFound();
    }
    // ------------------------------------------------------------------------------------------------------------
    void UInterface::CacheOfResolve::cache( const uniset3::ObjectId id, const uniset3::ObjectId node, std::shared_ptr<grpc::Channel>& chan ) const
    {
        uniset3::uniset_rwmutex_wrlock l(cmutex);

        uniset3::KeyType k( uniset3::key(id, node) );

        auto it = mcache.find(k);

        if( it == mcache.end() )
            mcache.emplace(k, Item(chan));
        else
        {
            it->second.chan = chan;
            it->second.ncall++;
        }
    }
    // ------------------------------------------------------------------------------------------------------------
    bool UInterface::CacheOfResolve::clean() noexcept
    {
        try
        {
            uniset3::uniset_rwmutex_wrlock l(cmutex);

            uinfo << "UI: clean cache...." << endl;

            for( auto it = mcache.begin(); it != mcache.end();)
            {
                if( it->second.ncall <= minCallCount )
                {
                    try
                    {
                        it->second.chan = nullptr;
                        mcache.erase(it++);
                    }
                    catch(...) {}
                }
                else
                    ++it;
            }
        }
        catch( std::exception& ex )
        {
            uwarn << "UI::Chache::clean: exception: " << ex.what() << endl;
        }

        if( mcache.size() < MaxSize )
            return true;

        return false;
    }
    // ------------------------------------------------------------------------------------------------------------

    void UInterface::CacheOfResolve::erase( const uniset3::ObjectId id, const uniset3::ObjectId node ) const noexcept
    {
        try
        {
            uniset3::uniset_rwmutex_wrlock l(cmutex);

            auto it = mcache.find( uniset3::key(id, node) );

            if( it != mcache.end() )
            {
                it->second.chan = nullptr;
                mcache.erase(it);
            }
        }
        catch( std::exception& ex )
        {
            uwarn << "UI::Chache::erase: exception: " << ex.what() << endl;
        }
    }

    // ------------------------------------------------------------------------------------------------------------
    bool UInterface::isExist( const uniset3::ObjectId id ) const noexcept
    {
#warning Not realized yet!
#if 0
        try
        {
            if( uconf->isLocalIOR() )
            {
                if( CORBA::is_nil(orb) )
                    orb = uconf->getORB();

                const string sior( uconf->iorfile->getIOR(id) );

                if( !sior.empty() )
                {
                    CORBA::Object_var oref = orb->string_to_object(sior.c_str());
                    return rep.isExist( oref );
                }

                return false;
            }

            return rep.isExist( oind->getNameById(id) );
        }
        catch( const uniset3::Exception& ex )
        {
            // uwarn << "UI(isExist): " << ex << endl;
        }
        catch(...) {}

#endif
        return false;
    }
    // ------------------------------------------------------------------------------------------------------------
    bool UInterface::isExist( const uniset3::ObjectId id, const uniset3::ObjectId node ) const noexcept
    {
        if( node == uconf->getLocalNode() )
            return isExist(id);

#warning Not realized yet!
#if 0
        CORBA::Object_var oref;

        try
        {
            try
            {
                oref = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound& ) {}

            if( CORBA::is_nil(oref) )
                oref = resolve(id, node);

            return rep.isExist( oref );
        }
        catch(...) {}

#endif
        return false;
    }
    // --------------------------------------------------------------------------------------------
    string UInterface::set_err( const std::string& pre, const uniset3::ObjectId id, const uniset3::ObjectId node ) const
    {
        if( id == uniset3::DefaultObjectId )
            return string(pre + " uniset3::DefaultObjectId");

        const string nm = oind->getNameById(id);

        ostringstream s;
        s << pre << " (" << id << ":" << node << ")" << (nm.empty() ? "UnknownName" : nm);
        return s.str();
    }
    // --------------------------------------------------------------------------------------------
    void UInterface::askThreshold( const uniset3::ObjectId sid, const uniset3::ThresholdId tid,
                                   uniset3::UIOCommand cmd,
                                   long low, long hi, bool invert,
                                   const uniset3::ObjectId backid ) const
    {
        askRemoteThreshold(sid, uconf->getLocalNode(), tid, cmd, low, hi, invert, backid);
    }
    // --------------------------------------------------------------------------------------------
    void UInterface::askRemoteThreshold( const uniset3::ObjectId sid, const uniset3::ObjectId node,
                                         uniset3::ThresholdId tid, uniset3::UIOCommand cmd,
                                         long lowLimit, long hiLimit, bool invert,
                                         uniset3::ObjectId backid ) const
    {
        if( backid == uniset3::DefaultObjectId )
            backid = myid;

        if( backid == uniset3::DefaultObjectId )
            throw uniset3::IOBadParam("UI(askRemoteThreshold): unknown back ID");

        if ( sid == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(askRemoteThreshold): error: id=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(askRemoteThreshold): id='" << sid << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::Empty reply;
            AskThresholdParams request;
            request.set_sid(sid);
            request.set_tid(tid);
            request.set_lowlimit(lowLimit);
            request.set_hilimit(hiLimit);
            request.set_invert(invert);
            request.set_cmd(cmd);
            auto ci = request.mutable_ci();
            ci->set_id(backid);
            ci->set_id(uconf->getLocalNode());

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(sid, node);

                std::unique_ptr<IONotifyController_i::Stub> stub(IONotifyController_i::NewStub(chan));
                grpc::Status st = stub->askThreshold(&ctx, request, &reply);

                if( st.ok() )
                    return;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(sid, node);
            throw uniset3::SystemError("UI(askThreshold): " + string(ex.what()));
        }

        rcache.erase(sid, node);
        throw uniset3::TimeOut(set_err("UI(askThreshold): Timeout", sid, node));

    }
    // --------------------------------------------------------------------------------------------
    uniset3::ThresholdInfo
    UInterface::getThresholdInfo( const uniset3::ObjectId sid, const uniset3::ThresholdId tid ) const
    {
        uniset3::SensorInfo si;
        si.set_id(sid);
        si.set_node(uconf->getLocalNode());
        return getThresholdInfo(si, tid);
    }
    // --------------------------------------------------------------------------------------------------------------
    uniset3::ThresholdInfo
    UInterface::getThresholdInfo( const uniset3::SensorInfo& si, const uniset3::ThresholdId tid ) const
    {
        if ( si.id() == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getThresholdInfo): error: id=uniset3::DefaultObjectId");

        if( si.node() == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getThresholdInfo): id='" << si.id() << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        ObjectId sid = si.id();
        ObjectId node = si.node();

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            ThresholdInfo reply;
            GetThresholdInfoParams request;
            request.set_sid(sid);
            request.set_tid(tid);

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(sid, node);

                std::unique_ptr<IONotifyController_i::Stub> stub(IONotifyController_i::NewStub(chan));
                grpc::Status st = stub->getThresholdInfo(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(sid, node);
            throw uniset3::SystemError("UI(getThresholdInfo): " + string(ex.what()));
        }

        rcache.erase(sid, node);
        throw uniset3::TimeOut(set_err("UI(getThresholdInfo): Timeout", sid, node));
    }
    // --------------------------------------------------------------------------------------------
    long UInterface::getRawValue( const uniset3::SensorInfo& si )
    {
        if( si.id() == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getRawValue): error: id=uniset3::DefaultObjectId");

        if( si.node() == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getRawValue): id='" << si.id() << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        ObjectId sid = si.id();
        ObjectId node = si.node();

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::Int64Value reply;
            google::protobuf::Int64Value request;
            request.set_value(sid);

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(sid, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->getRawValue(&ctx, request, &reply);

                if( st.ok() )
                    return reply.value();

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(sid, node);
            throw uniset3::SystemError("UI(getRawValue): " + string(ex.what()));
        }

        rcache.erase(sid, node);
        throw uniset3::TimeOut(set_err("UI(getRawValue): Timeout", sid, node));
    }
    // --------------------------------------------------------------------------------------------
    void UInterface::calibrate(const uniset3::SensorInfo& si,
                               const uniset3::CalibrateInfo& ci,
                               uniset3::ObjectId admId )
    {
        if( admId == uniset3::DefaultObjectId )
            admId = myid;

        //    if( admId==uniset3::DefaultObjectId )
        //        throw uniset3::IOBadParam("UI(askTreshold): неизвестен ID администратора");

        if( si.id() == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(calibrate): error: id=uniset3::DefaultObjectId");

        if( si.node() == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(calibrate): id='" << si.id() << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        ObjectId sid = si.id();
        ObjectId node = si.node();

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            google::protobuf::Empty reply;
            CalibrateParams request;
            request.set_id(sid);
            request.set_adminid(admId);
            *(request.mutable_ci()) = ci;

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(sid, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->calibrate(&ctx, request, &reply);

                if( st.ok() )
                    return;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(sid, node);
            throw uniset3::SystemError("UI(calibrate): " + string(ex.what()));
        }

        rcache.erase(sid, node);
        throw uniset3::TimeOut(set_err("UI(calibrate): Timeout", sid, node));
    }
    // --------------------------------------------------------------------------------------------
    uniset3::CalibrateInfo UInterface::getCalibrateInfo( const uniset3::SensorInfo& si )
    {
        if ( si.id() == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getCalibrateInfo): попытка обратиться к объекту с id=uniset3::DefaultObjectId");

        if( si.node() == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getCalibrateInfo): id='" << si.id() << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        ObjectId sid = si.id();
        ObjectId node = si.node();

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            CalibrateInfo reply;
            google::protobuf::Int64Value request;
            request.set_value(sid);

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}


            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(sid, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->getCalibrateInfo(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(sid, node);
            throw uniset3::SystemError("UI(getCalibrateInfo): " + string(ex.what()));
        }

        rcache.erase(sid, node);
        throw uniset3::TimeOut(set_err("UI(getCalibrateInfo): Timeout", sid, node));
    }
    // --------------------------------------------------------------------------------------------
    uniset3::SensorIOInfoSeq UInterface::getSensorSeq( const uniset3::IDList& lst )
    {
        if( lst.empty() )
            return uniset3::SensorIOInfoSeq();

        uniset3::ObjectId sid = lst.getFirst();
        uniset3::ObjectId node = uconf->getLocalNode();

        if ( sid == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getSensorSeq): попытка обратиться к объекту с id=uniset3::DefaultObjectId");

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            SensorIOInfoSeq reply;
            IDSeq request(lst.getIDSeq());

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(sid, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->getSensorSeq(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(sid, node);
            throw uniset3::SystemError("UI(getCalibrateInfo): " + string(ex.what()));
        }

        rcache.erase(sid, node);
        throw uniset3::TimeOut(set_err("UI(getSensorSeq): Timeout", sid, node));

    }
    // --------------------------------------------------------------------------------------------
    uniset3::SensorIOInfo UInterface::getSensorIOInfo( const uniset3::SensorInfo& si )
    {
        if ( si.id() == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getSensorIOInfo): error node=uniset3::DefaultObjectId");

        if ( si.node() == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getSensorIOInfo): попытка обратиться к объекту с id=uniset3::DefaultObjectId");

        ObjectId sid = si.id();
        ObjectId node = si.node();

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            SensorIOInfo reply;
            google::protobuf::Int64Value request;
            request.set_value(sid);

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(sid, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->getSensorIOInfo(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(sid, node);
            throw uniset3::SystemError("UI(getSensorIOInfo): " + string(ex.what()));
        }

        rcache.erase(sid, node);
        throw uniset3::TimeOut(set_err("UI(getSensorIOInfo): Timeout", sid, node));
    }
    // --------------------------------------------------------------------------------------------
    uniset3::IDSeq UInterface::setOutputSeq( const uniset3::OutSeq& seq, uniset3::ObjectId sup_id )
    {
        if( seq.sensors().size() == 0 )
            return uniset3::IDSeq();


        if ( seq.sensors(0).si().id() == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(setOutputSeq): попытка обратиться к объекту с id=uniset3::DefaultObjectId");

        ObjectId sid = seq.sensors(0).si().id();
        ObjectId node = seq.sensors(0).si().node();

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            IDSeq reply;
            SetOutputParams request;
            request.set_supplier(sid);
            *(request.mutable_lst()) = seq;

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound& ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(sid, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->setOutputSeq(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(sid, node);
            throw uniset3::SystemError("UI(setOutputSeq): " + string(ex.what()));
        }

        rcache.erase(sid, node);
        throw uniset3::TimeOut(set_err("UI(setOutputSeq): Timeout", sid, node));
    }
    // --------------------------------------------------------------------------------------------
    uniset3::IDSeq UInterface::askSensorsSeq( const uniset3::IDList& lst,
            uniset3::UIOCommand cmd, uniset3::ObjectId backid )
    {
        if( lst.empty() )
            return uniset3::IDSeq();

        if( backid == uniset3::DefaultObjectId )
            backid = myid;

        if( backid == uniset3::DefaultObjectId )
            throw uniset3::IOBadParam("UI(askSensorSeq): unknown back ID");

        ObjectId sid = lst.getFirst();
        ObjectId node = uconf->getLocalNode();

        if ( sid == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(askSensorSeq): попытка обратиться к объекту с id=uniset3::DefaultObjectId");

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            IDSeq reply;
            AskSeqParams request;
            request.set_cmd(cmd);
            *request.mutable_ids() = lst.getIDSeq();
            auto ci = request.mutable_ci();
            ci->set_id(backid);
            ci->set_node(uconf->getLocalNode());

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(sid, node);

                std::unique_ptr<IONotifyController_i::Stub> stub(IONotifyController_i::NewStub(chan));
                grpc::Status st = stub->askSensorsSeq(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(sid, node);
            throw uniset3::SystemError("UI(askSensorsSeq): " + string(ex.what()));
        }

        rcache.erase(sid, node);
        throw uniset3::TimeOut(set_err("UI(askSensorsSeq): Timeout", sid, node));
    }
    // -----------------------------------------------------------------------------
    uniset3::ShortMapSeq UInterface::getSensors( const uniset3::ObjectId id, uniset3::ObjectId node )
    {
        if ( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getSensors): error node=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getSensors): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            ShortMapSeq reply;
            google::protobuf::Empty request;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}


            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(id, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->getSensors(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(getSensors): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(getSensors): Timeout", id, node));
    }
    // -----------------------------------------------------------------------------
    uniset3::SensorIOInfoSeq UInterface::getSensorsMap( const uniset3::ObjectId id, const uniset3::ObjectId node )
    {
        if ( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getSensorsMap): error node=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getSensorsMap): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            SensorIOInfoSeq reply;
            google::protobuf::Empty request;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(id, node);

                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan));
                grpc::Status st = stub->getSensorsMap(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(getSensorsMap): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(getSensorsMap): Timeout", id, node));
    }
    // -----------------------------------------------------------------------------
    uniset3::ThresholdsListSeq UInterface::getThresholdsList( const uniset3::ObjectId id, const uniset3::ObjectId node )
    {
        if ( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(getThresholdsList): error node=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(getThresholdsList): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            grpc::ClientContext ctx;
            std::shared_ptr<grpc::Channel> chan;
            ThresholdsListSeq reply;
            google::protobuf::Empty request;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = rcache.resolve(id, node);

                std::unique_ptr<IONotifyController_i::Stub> stub(IONotifyController_i::NewStub(chan));
                grpc::Status st = stub->getThresholdsList(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(getThresholdsList): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(getThresholdsList): Timeout", id, node));
    }
    // -----------------------------------------------------------------------------
    bool UInterface::waitReady( const uniset3::ObjectId id, int msec, int pmsec, const uniset3::ObjectId node ) noexcept
    {
        std::atomic_bool cancelFlag = { false };
        return waitReadyWithCancellation(id, msec, cancelFlag, pmsec, node);
    }
    // -----------------------------------------------------------------------------
    bool UInterface::waitWorking( const uniset3::ObjectId id, int msec, int pmsec, const uniset3::ObjectId node ) noexcept
    {
        if( msec < 0 )
            msec = 0;

        if( pmsec < 0 )
            pmsec = 0;

        PassiveTimer ptReady(msec);

        while( !ptReady.checkTime() )
        {
            try
            {
                getValue(id, node);
                return true;
            }
            catch(...) {}

            msleep(pmsec);
        }

        return false;
    }
    // -----------------------------------------------------------------------------
    bool UInterface::waitReadyWithCancellation(const ObjectId id, int msec,
            std::atomic_bool& cancelFlag, int pmsec, const ObjectId node) noexcept
    {
        if( msec < 0 )
            msec = 0;

        if( pmsec < 0 )
            pmsec = 0;

        PassiveTimer ptReady(msec);
        bool ready = false;

        while( !ptReady.checkTime() && !ready && !cancelFlag )
        {
            try
            {
                ready = isExist(id, node);

                if( ready )
                    break;
            }
            catch(...)
            {
                break;
            }

            msleep(pmsec);
        }

        return ready;
    }
    // -----------------------------------------------------------------------------
    uniset3::IOType UInterface::getConfIOType( const uniset3::ObjectId id ) const noexcept
    {
        if( !uconf )
            return uniset3::UnknownIOType;

        xmlNode* x = uconf->getXMLObjectNode(id);

        if( !x )
            return uniset3::UnknownIOType;

        UniXML::iterator it(x);
        return uniset3::getIOType( it.getProp("iotype") );
    }
    // -----------------------------------------------------------------------------
} // end of namespace uniset3
