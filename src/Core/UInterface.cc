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
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include <string>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include "UInterface.h"
#include "Configuration.h"
#include "PassiveTimer.h"
#include "IOController.grpc.pb.h"
#include "UniSetObject.grpc.pb.h"
#include "UHelpers.h"
#include "IOController.h"

// -----------------------------------------------------------------------------
namespace uniset3
{
    // -----------------------------------------------------------------------------
    using namespace std;
    // -----------------------------------------------------------------------------
    static void continue_or_throw(const grpc::Status& st, const std::string& fname = "")
    {
        if( st.error_code() != grpc::StatusCode::UNAVAILABLE && st.error_code() != grpc::StatusCode::DEADLINE_EXCEEDED )
        {
            ostringstream err;
            err << fname << " error(" << st.error_code() << "): " << st.error_message();
            throw uniset3::SystemError(err.str());
        }
    }
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
     * \exception TimeOut - генерируется если в течение времени timeout не был получен ответ
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
            std::shared_ptr<ORefInfo> chan;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            GetValueParams request;
            request.set_id(id);
            google::protobuf::Int64Value reply;

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve( id, node );

                if( chan )
                {
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    chan->addMetaData(ctx);
                    std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                    grpc::Status st = stub->getValue(&ctx, request, &reply);

                    if( st.ok() )
                        return reply.value();

                    continue_or_throw(st, __FUNCTION__);
                }

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
            std::shared_ptr<ORefInfo> chan;
            google::protobuf::Empty reply;
            FreezeValueParams request;
            request.set_id(si.id());
            request.set_sup_id(sup_id);
            request.set_set(set);
            request.set_value(value);

            try
            {
                chan = rcache.resolve(si.id(), si.node());
            }
            catch( const uniset3::NameNotFound& ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(si.id(), si.node());

                if( chan )
                {
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    chan->addMetaData(ctx);
                    std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                    grpc::Status st = stub->freezeValue(&ctx, request, &reply);

                    if( st.ok() )
                        return;

                    continue_or_throw(st, __FUNCTION__);
                }

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
            std::shared_ptr<ORefInfo> chan;
            google::protobuf::Empty reply;
            SetValueParams request;
            request.set_id(id);
            request.set_sup_id(sup_id);
            request.set_value(value);

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                if( chan )
                {
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    chan->addMetaData(ctx);
                    std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                    grpc::Status st = stub->setValue(&ctx, request, &reply);
                    if( st.ok() )
                        return;

                    continue_or_throw(st, __FUNCTION__);
                }

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
            std::shared_ptr<ORefInfo> chan;
            google::protobuf::Empty reply;
            AskParams request;
            request.set_sid(id);
            request.set_cmd(cmd);
            request.mutable_ci()->set_id(backid);
            request.mutable_ci()->set_node(uconf->getLocalNode());

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                if( chan )
                {
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    chan->addMetaData(ctx);
                    std::unique_ptr<IONotifyController_i::Stub> stub(IONotifyController_i::NewStub(chan->c));
                    grpc::Status st = stub->askSensor(&ctx, request, &reply);

                    if( st.ok() )
                        return;

                    continue_or_throw(st, __FUNCTION__);
                }

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
            std::shared_ptr<ORefInfo> chan;
            RetIOType reply;
            GetIOTypeParams request;
            request.set_id(id);

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                if( chan )
                {
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    chan->addMetaData(ctx);
                    std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                    grpc::Status st = stub->getIOType(&ctx, request, &reply);

                    if( st.ok() )
                        return reply.type();

                    continue_or_throw(st, __FUNCTION__);
                }

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
            std::shared_ptr<ORefInfo> chan;
            google::protobuf::StringValue reply;
            GetTypeParams request;
            request.set_id(id);

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                if( chan )
                {
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    chan->addMetaData(ctx);
                    std::unique_ptr<UniSetObject_i::Stub> stub(UniSetObject_i::NewStub(chan->c));
                    grpc::Status st = stub->getType(&ctx, request, &reply);

                    if( st.ok() )
                        return reply.value();

                    continue_or_throw(st, __FUNCTION__);
                }

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
    std::shared_ptr<grpc::Channel> UInterface::resolveRepository( ObjectId node ) const
    {
        google::protobuf::BoolValue reply;
        google::protobuf::Empty req;

        std::shared_ptr<grpc::Channel> chan;

        for( size_t curNet = 0; curNet <= uconf->getCountOfNet(); curNet++)
        {
            try
            {
                auto repIP = uconf->repositoryAddressByNode(node, curNet);

                if( repIP.empty() )
                    continue;

                chan = grpc::CreateChannel(repIP, grpc::InsecureChannelCredentials());
                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                std::unique_ptr<URepository_i::Stub> stub(URepository_i::NewStub(chan));
                grpc::Status st = stub->exists(&ctx, req, &reply);

                if( st.ok() )
                    return chan;

                continue_or_throw(st, __FUNCTION__);
            }
            catch( const std::exception& ex ) {}
        }

        throw uniset3::ResolveNameError();
    }
    // ------------------------------------------------------------------------------------------------------------
    void UInterface::registered( const uniset3::ObjectRef oRef, bool force ) const
    {
        // если включён режим использования локальных файлов
        // то пишем IOR в файл
        if( uconf->isLocalIOR() )
        {
            uconf->iorfile->setIOR(oRef.id(), oRef);
            return;
        }

        google::protobuf::Empty reply;

        for( size_t i = 0; i < uconf->getRepeatCount(); i++ )
        {
            if( !rep )
                rep = resolveRepository(uconf->getLocalNode());

            grpc::ClientContext ctx;
            ctx.set_deadline(uconf->deadline());
            std::unique_ptr<URepository_i::Stub> stub(URepository_i::NewStub(rep));
            grpc::Status st = stub->registration(&ctx, oRef, &reply);

            if( st.ok() )
                return;

            continue_or_throw(st, __FUNCTION__);

            msleep(uconf->getRepeatTimeout());
            rep = nullptr;
        }

        throw uniset3::TimeOut();
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

        for (size_t i = 0; i < uconf->getRepeatCount(); i++)
        {
            if( !rep )
                rep = resolveRepository(uconf->getLocalNode());

            grpc::ClientContext ctx;
            ctx.set_deadline(uconf->deadline());
            std::unique_ptr<URepository_i::Stub> stub(URepository_i::NewStub(rep));
            grpc::Status st = stub->unregistration(&ctx, request, &reply);

            if( st.ok() )
                return;

            continue_or_throw(st, __FUNCTION__);

            msleep(uconf->getRepeatTimeout());
            rep = nullptr;
        }

        throw uniset3::TimeOut();
    }

    // ------------------------------------------------------------------------------------------------------------
    std::shared_ptr<UInterface::ORefInfo> UInterface::resolve( const uniset3::ObjectId rid, const uniset3::ObjectId node ) const
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

        try
        {
            if( uconf->isLocalIOR() && node == uconf->getLocalNode() )
            {
                auto o = make_shared<ORefInfo>();
                o->ref = uconf->iorfile->getRef(rid);
                o->c = grpc::CreateChannel(o->ref.addr(), grpc::InsecureChannelCredentials());
                rcache.cache(rid, node, o); // заносим в кэш
                return o;
            }

            google::protobuf::Int64Value request;
            request.set_value(rid);

            auto o = make_shared<ORefInfo>();
            std::shared_ptr<grpc::Channel> repChan;
            std::unique_ptr<URepository_i::Stub> stub;

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                try
                {
                    if( !repChan )
                        repChan = resolveRepository(node);

                    if( !repChan )
                    {
                        msleep(uconf->getRepeatTimeout());
                        continue;
                    }

                    stub = URepository_i::NewStub(repChan);
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    o->addMetaData(ctx);
                    grpc::Status st = stub->resolve(&ctx, request, &(o->ref));

                    if( st.ok() )
                    {
                        o->c = grpc::CreateChannel(o->ref.addr(), grpc::InsecureChannelCredentials());
                        rcache.cache(rid, node, o);
                        return o;
                    }
                    else if( st.error_code() == grpc::StatusCode::NOT_FOUND )
                    {
                        throw uniset3::ResolveNameError();
                    }

                    continue_or_throw(st, __FUNCTION__);
                }
                catch( const std::exception& ex ) {}

                msleep(uconf->getRepeatTimeout());
                repChan = nullptr;
            }

            throw uniset3::TimeOut();
        }
        catch( std::exception& ex )
        {
            ucrit << "UI(resolve): myID=" << myid <<  ": resolve id=" << rid << "@" << node
                  << " catch " << ex.what() << endl;
        }

        throw uniset3::ResolveNameError();
    }

    // -------------------------------------------------------------------------------------------
    uniset3::ObjectRef UInterface::resolveORefOnly( const uniset3::ObjectId rid, const uniset3::ObjectId node  ) const
    {
        if( rid == uniset3::DefaultObjectId )
            throw uniset3::ResolveNameError("UI(resolveORefOnly): ID=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(resolveORefOnly): id='" << rid << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ResolveNameError(err.str());
        }

        try
        {
            if( uconf->isLocalIOR() && node == uconf->getLocalNode() )
                return uconf->iorfile->getRef(rid);

            google::protobuf::Int64Value request;
            request.set_value(rid);

            auto o = make_shared<ORefInfo>();
            std::shared_ptr<grpc::Channel> repChan;
            std::unique_ptr<URepository_i::Stub> stub;

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                try
                {
                    if( !repChan )
                        repChan = resolveRepository(node);

                    if( !repChan )
                    {
                        msleep(uconf->getRepeatTimeout());
                        continue;
                    }

                    stub = URepository_i::NewStub(repChan);
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    o->addMetaData(ctx);
                    uniset3::ObjectRef oref;
                    grpc::Status st = stub->resolve(&ctx, request, &oref);

                    if( st.ok() )
                        return oref;

                    if( st.error_code() == grpc::StatusCode::NOT_FOUND )
                        throw uniset3::ResolveNameError();

                    continue_or_throw(st, __FUNCTION__);
                }
                catch( const std::exception& ex ) {}

                msleep(uconf->getRepeatTimeout());
                repChan = nullptr;
            }

            throw uniset3::TimeOut();
        }
        catch( std::exception& ex )
        {
            ucrit << "UI(resolveORefOnly): myID=" << myid <<  ": resolve id=" << rid << "@" << node
            << " catch " << ex.what() << endl;
        }

        throw uniset3::ResolveNameError();
    }
    // -------------------------------------------------------------------------------------------
    void UInterface::send(const uniset3::umessage::TransportMessage& msg, uniset3::ObjectId node)
    {
        ObjectId id = msg.consumer();

        if( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(send): ERROR: id=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
            node = uconf->getLocalNode();

        try
        {
            std::shared_ptr<ORefInfo> chan;
            google::protobuf::Empty reply;

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                if( chan )
                {
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    chan->addMetaData(ctx);
                    std::unique_ptr<UniSetObject_i::Stub> stub(UniSetObject_i::NewStub(chan->c));
                    grpc::Status st = stub->push(&ctx, msg, &reply);

                    if( st.ok() )
                        return;

                    continue_or_throw(st, __FUNCTION__);
                }

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
        if( name == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(sendText): ERROR: id=uniset3::DefaultObjectId");

        uniset3::ObjectId onode = (node == uniset3::DefaultObjectId) ? uconf->getLocalNode() : node;

        uniset3::umessage::TextMessage msg;
        auto header = msg.mutable_header();
        header->set_priority(uniset3::umessage::mpMedium);
        header->set_node(uconf->getLocalNode());
        header->set_supplier(myid);
        header->set_consumer(name);
        auto ts = uniset3::now_to_uniset_timespec();
        (*header->mutable_ts()) = ts;
        msg.set_txt(txt);
        msg.set_mtype(mtype);
        sendText(msg, onode);
    }
    // ------------------------------------------------------------------------------------------------------------
    void UInterface::sendText( const uniset3::umessage::TextMessage& msg, uniset3::ObjectId node )
    {
        if( msg.header().consumer() == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(sendText): ERROR: consumer=uniset3::DefaultObjectId");

        uniset3::umessage::TransportMessage tm = to_transport<uniset3::umessage::TextMessage>(msg);
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
            std::shared_ptr<ORefInfo> chan;
            ShortIOInfo reply;
            GetTimeChangeParams request;
            request.set_id(id);

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                if( chan )
                {
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    chan->addMetaData(ctx);
                    std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                    grpc::Status st = stub->getTimeChange(&ctx, request, &reply);

                    if( st.ok() )
                        return reply;

                    continue_or_throw(st, __FUNCTION__);
                }

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
            std::shared_ptr<ORefInfo> chan;
            google::protobuf::StringValue reply;
            GetInfoParams request;
            request.set_id(id);
            request.set_params(params);

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for (size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                if( chan )
                {
                    grpc::ClientContext ctx;
                    ctx.set_deadline(uconf->deadline());
                    chan->addMetaData(ctx);
                    std::unique_ptr<UniSetObject_i::Stub> stub(UniSetObject_i::NewStub(chan->c));
                    grpc::Status st = stub->getInfo(&ctx, request, &reply);

                    if( st.ok() )
                        return reply.value();

                    continue_or_throw(st, __FUNCTION__);
                }

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(getInfo): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(getInfo): Timeout", id, node));
    }
    // ------------------------------------------------------------------------------------------------------------
    void UInterface::ORefInfo::addMetaData( grpc::ClientContext& ctx )
    {
        for( const  auto& m : ref.metadata() )
            ctx.AddMetadata(m.first, m.second);
    }
    // ------------------------------------------------------------------------------------------------------------
    std::shared_ptr<UInterface::ORefInfo> UInterface::resolve( const uniset3::ObjectId id ) const
    {
        if( uconf->isLocalIOR() )
        {
            try
            {
                auto o = make_shared<ORefInfo>();
                o->ref = uconf->iorfile->getRef(id);
                o->c = grpc::CreateChannel(o->ref.addr(), grpc::InsecureChannelCredentials());
                return o;
            }
            catch(...) {}

            throw uniset3::ResolveNameError("UI(resolve): ID=" + to_string(id));
        }

        if( !rep )
            rep = resolveRepository(uconf->getLocalNode());

        std::unique_ptr<URepository_i::Stub> stub(URepository_i::NewStub(rep));
        grpc::ClientContext ctx;
        ctx.set_deadline(uconf->deadline());
        google::protobuf::Int64Value request;
        request.set_value(id);

        auto o = make_shared<ORefInfo>();
        grpc::Status st = stub->resolve(&ctx, request, &o->ref);

        if( !st.ok() )
            throw uniset3::ORepFailed();

        o->c = grpc::CreateChannel(o->ref.addr(), grpc::InsecureChannelCredentials());
        return o;
    }
    // ------------------------------------------------------------------------------------------------------------
    std::shared_ptr<UInterface::ORefInfo> UInterface::CacheOfResolve::resolve( const uniset3::ObjectId id, const uniset3::ObjectId node ) const
    {
        try
        {
            uniset3::uniset_rwmutex_rlock l(cmutex);

            auto it = mcache.find( uniset3::key(id, node) );

            if( it != mcache.end() )
            {
                it->second.ncall++;
                return it->second.oinf;
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
    void UInterface::CacheOfResolve::cache( const uniset3::ObjectId id, const uniset3::ObjectId node, std::shared_ptr<ORefInfo>& chan ) const
    {
        uniset3::uniset_rwmutex_wrlock l(cmutex);

        uniset3::KeyType k( uniset3::key(id, node) );

        auto it = mcache.find(k);

        if( it == mcache.end() )
            mcache.emplace(k, Item(chan));
        else
        {
            it->second.oinf = chan;
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
                        it->second.oinf->c = nullptr;
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
                it->second.oinf->c = nullptr;
                mcache.erase(it);
            }
        }
        catch( std::exception& ex )
        {
            uwarn << "UI::Chache::erase: exception: " << ex.what() << endl;
        }
    }

    // ------------------------------------------------------------------------------------------------------------
    bool UInterface::isExists( const uniset3::ObjectId id ) const noexcept
    {
        std::shared_ptr<ORefInfo> chan;

        try
        {
            if( uconf->isLocalIOR() )
            {
                chan = make_shared<ORefInfo>();
                chan->ref = uconf->iorfile->getRef(id);
                chan->c = grpc::CreateChannel(chan->ref.addr(), grpc::InsecureChannelCredentials());
            }
            else
            {
                chan = resolve(id, uconf->getLocalNode());
            }

            if( !chan )
                return false;

            grpc::ClientContext ctx;
            ctx.set_deadline(uconf->deadline());
            chan->addMetaData(ctx);
            ExistsParams req;
            req.set_id(id);
            google::protobuf::BoolValue resp;

            std::unique_ptr<UniSetObject_i::Stub> stub(UniSetObject_i::NewStub(chan->c));
            grpc::Status st = stub->exists(&ctx, req, &resp);

            if( st.ok() )
                return resp.value();
        }
        catch( const uniset3::Exception& ex )
        {
            // uwarn << "UI(isExist): " << ex << endl;
        }
        catch(...) {}

        return false;
    }
    // ------------------------------------------------------------------------------------------------------------
    bool UInterface::isExists( const uniset3::ObjectId id, const uniset3::ObjectId node ) const noexcept
    {
        if( node == DefaultObjectId )
            return false;

        if( node == uconf->getLocalNode() )
            return isExists(id); // local node

        ExistsParams req;
        req.set_id(id);
        google::protobuf::BoolValue resp;
        std::shared_ptr<ORefInfo> chan;

        try
        {
            chan = rcache.resolve(id, node);
        }
        catch( const uniset3::NameNotFound&  ) {}

        try
        {
            for( size_t i = 0; i < uconf->getRepeatCount(); i++ )
            {
                if( !chan )
                    chan = resolve(id, node);

                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                chan->addMetaData(ctx);
                std::unique_ptr<UniSetObject_i::Stub> stub(UniSetObject_i::NewStub(chan->c));
                grpc::Status st = stub->exists(&ctx, req, &resp);

                if( st.ok() )
                    return resp.value();

                continue_or_throw(st, __FUNCTION__);

                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch(...) {}

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
            std::shared_ptr<ORefInfo> chan;
            google::protobuf::Int64Value reply;
            GetRawValueParams request;
            request.set_id(sid);

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(sid, node);

                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                chan->addMetaData(ctx);
                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                grpc::Status st = stub->getRawValue(&ctx, request, &reply);

                if( st.ok() )
                    return reply.value();

                continue_or_throw(st, __FUNCTION__);

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
            std::shared_ptr<ORefInfo> chan;
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
                    chan = resolve(sid, node);

                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                chan->addMetaData(ctx);
                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                grpc::Status st = stub->calibrate(&ctx, request, &reply);

                if( st.ok() )
                    return;

                continue_or_throw(st, __FUNCTION__);

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
            std::shared_ptr<ORefInfo> chan;
            CalibrateInfo reply;
            GetCalibrateInfoParams request;
            request.set_id(sid);

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}


            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(sid, node);

                grpc::ClientContext ctx;
                chan->addMetaData(ctx);
                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                grpc::Status st = stub->getCalibrateInfo(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                continue_or_throw(st, __FUNCTION__);

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
            std::shared_ptr<ORefInfo> chan;
            SensorIOInfoSeq reply;
            GetSensorSeqParams request;
            *(request.mutable_seq())  = lst.getIDSeq();

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(sid, node);

                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                chan->addMetaData(ctx);
                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                grpc::Status st = stub->getSensorSeq(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                continue_or_throw(st, __FUNCTION__);
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
            std::shared_ptr<ORefInfo> chan;
            SensorIOInfo reply;
            GetSensorIOInfoParams request;
            request.set_id(sid);

            try
            {
                chan = rcache.resolve(sid, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(sid, node);

                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                chan->addMetaData(ctx);
                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                grpc::Status st = stub->getSensorIOInfo(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                continue_or_throw(st, __FUNCTION__);
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
            std::shared_ptr<ORefInfo> chan;
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
                    chan = resolve(sid, node);

                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                chan->addMetaData(ctx);
                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                grpc::Status st = stub->setOutputSeq(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                continue_or_throw(st, __FUNCTION__);
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
            std::shared_ptr<ORefInfo> chan;
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
                    chan = resolve(sid, node);

                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                chan->addMetaData(ctx);
                std::unique_ptr<IONotifyController_i::Stub> stub(IONotifyController_i::NewStub(chan->c));
                grpc::Status st = stub->askSensorsSeq(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                continue_or_throw(st, __FUNCTION__);
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
            std::shared_ptr<ORefInfo> chan;
            ShortMapSeq reply;
            GetSensorsParams request;
            request.set_id(id);

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}


            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                chan->addMetaData(ctx);
                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                grpc::Status st = stub->getSensors(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                continue_or_throw(st, __FUNCTION__);
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
            std::shared_ptr<ORefInfo> chan;
            SensorIOInfoSeq reply;
            GetSensorsMapParams request;
            request.set_id(id);

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                chan->addMetaData(ctx);
                std::unique_ptr<IOController_i::Stub> stub(IOController_i::NewStub(chan->c));
                grpc::Status st = stub->getSensorsMap(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                continue_or_throw(st, __FUNCTION__);
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
    uniset3::metrics::Metrics UInterface::metrics( const uniset3::ObjectId id, const uniset3::ObjectId node )
    {
        if ( id == uniset3::DefaultObjectId )
            throw uniset3::ORepFailed("UI(metrics): error node=uniset3::DefaultObjectId");

        if( node == uniset3::DefaultObjectId )
        {
            ostringstream err;
            err << "UI(metrics): id='" << id << "' error: node=uniset3::DefaultObjectId";
            throw uniset3::ORepFailed(err.str());
        }

        try
        {
            std::shared_ptr<ORefInfo> chan;
            metrics::Metrics reply;
            metrics::MetricsParams request;
            request.set_id(id);

            try
            {
                chan = rcache.resolve(id, node);
            }
            catch( const uniset3::NameNotFound&  ) {}

            for( size_t i = 0; i < uconf->getRepeatCount(); i++)
            {
                if( !chan )
                    chan = resolve(id, node);

                grpc::ClientContext ctx;
                ctx.set_deadline(uconf->deadline());
                chan->addMetaData(ctx);
                std::unique_ptr<metrics::MetricsExporter_i::Stub> stub(metrics::MetricsExporter_i::NewStub(chan->c));
                grpc::Status st = stub->metrics(&ctx, request, &reply);

                if( st.ok() )
                    return reply;

                continue_or_throw(st, __FUNCTION__);
                msleep(uconf->getRepeatTimeout());
                chan = nullptr;
            }
        }
        catch( const std::exception& ex )
        {
            rcache.erase(id, node);
            throw uniset3::SystemError("UI(metrics): " + string(ex.what()));
        }

        rcache.erase(id, node);
        throw uniset3::TimeOut(set_err("UI(metrics): Timeout", id, node));
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
                ready = isExists(id, node);

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
