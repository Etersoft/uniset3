/*
 * Copyright (c) 2023 Pavel Vainerman.
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
// -------------------------------------------------------------------------
extern "C" {
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/plugin/nodestore.h>
}

#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include "open62541pp/ErrorHandling.h"
#include "Exceptions.h"
#include "OPCUAServer.h"
#include "unisetstd.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
using namespace uniset3::extensions;
using namespace uniset3::umessage;
// -----------------------------------------------------------------------------
// ( val, confval, default val )
static const std::string init3_str( const std::string& s1, const std::string& s2, const std::string& s3 )
{
    if( !s1.empty() )
        return s1;

    if( !s2.empty() )
        return s2;

    return s3;
}
// -----------------------------------------------------------------------------
OPCUAServer::OPCUAServer(uniset3::ObjectId objId, xmlNode* cnode, uniset3::ObjectId shmId, const std::shared_ptr<SharedMemory>& ic,
                         const string& _prefix ):
    UObject_SK(objId, cnode, string(_prefix + "-")),
    prefix(_prefix)
{
    auto conf = uniset_conf();

    if( ic )
        ic->logAgregator()->add(logAgregator());

    shm = make_shared<SMInterface>(shmId, ui, objId, ic);

    UniXML::iterator it(cnode);

    auto ip = conf->getArgParam("--" + argprefix + "host", "0.0.0.0");
    auto port = conf->getArgPInt("--" + argprefix + "port", it.getProp("port"), 4840);
    auto browseName = it.getProp2("browseName", conf->oind->getMapName(conf->getLocalNode()));
    auto description = it.getProp2("description", browseName);
    propPrefix = conf->getArgParam("--" + argprefix + "prop-prefix", it.getProp("propPrefix"));

    opcServer = unisetstd::make_unique<opcua::Server>((uint16_t)port);
    opcServer->setApplicationName(it.getProp2("appName", "Uniset3 OPC UA Server"));
    opcServer->setApplicationUri(it.getProp2("appUri", "urn:uniset3.server"));
    opcServer->setProductUri(it.getProp2("productUri", "https://github.com/Etersoft/uniset3/"));
    namePrefix = it.getProp2("namePrefix", "");
    updateTime_msec = conf->getArgPInt("--" + argprefix + "updatetime", it.getProp("updateTime"), (int)updateTime_msec);
    vmonit(updateTime_msec);
    myinfo << myname << "(init): OPC UA server " << ip << ":" << port << " updatePause=" << updateTime_msec << endl;

    opcServer->setLogger([this](auto level, auto category, auto msg)
    {
        mylog->level5() << myname
                        << "[" << getLogLevelName(level) << "] "
                        << "[" << getLogCategoryName(category) << "] "
                        << msg << std::endl;
    });

    auto opcConfig = UA_Server_getConfig(opcServer->handle());
    opcConfig->maxSubscriptions = conf->getArgPInt("--" + argprefix + "maxSubscriptions", it.getProp("maxSubscriptions"), (int)opcConfig->maxSubscriptions);
    opcConfig->maxSessions = conf->getArgPInt("--" + argprefix + "maxSessions", it.getProp("maxSessions"), (int)opcConfig->maxSessions);
    opcConfig->maxSecureChannels = conf->getArgPInt("--" + argprefix + "maxSecureChannels", it.getProp("maxSecureChannels"), (int)opcConfig->maxSessions);
    opcConfig->maxSessionTimeout = conf->getArgPInt("--" + argprefix + "maxSessionTimeout", it.getProp("maxSessionTimeout"), 5000);
    opcConfig->maxSecurityTokenLifetime = conf->getArgPInt("--" + argprefix + "maxSecurityTokenLifetime", it.getProp("maxSecurityTokenLifetime"), (int)opcConfig->maxSecurityTokenLifetime);

    myinfo << myname << "(init): OPC UA server:"
           << " maxSessions=" << opcConfig->maxSessions
           << " maxSubscriptions=" << opcConfig->maxSubscriptions
           << " maxSecureChannels=" << opcConfig->maxSecureChannels
           << " maxSessionTimeout=" << opcConfig->maxSessionTimeout
           << " maxSecurityTokenLifetime=" << opcConfig->maxSecurityTokenLifetime
           << endl;

    UA_LogLevel loglevel = UA_LOGLEVEL_ERROR;

    if( mylog->is_warn() )
        loglevel = UA_LOGLEVEL_WARNING;

    if( mylog->is_level5() )
        loglevel = UA_LOGLEVEL_INFO;

    if( mylog->is_level9() )
        loglevel = UA_LOGLEVEL_DEBUG;

    opcConfig->logger = UA_Log_Stdout_withLevel( loglevel );

    auto uroot = opcServer->getRootNode().addFolder(opcua::NodeId(0, "uniset"), "uniset");
    uroot.writeDescription({"en", "uniset i/o"});

    ioNode = unisetstd::make_unique<IONode>(uroot.addFolder(opcua::NodeId(0, browseName), browseName));
    ioNode->node.writeDescription({"ru-RU", description});
    ioNode->node.writeDisplayName({"en", browseName});
    opcServer->setCustomHostname(ip);

    /* Инициализация каталога */
    UniXML::iterator tit = conf->findNode(cnode, "folders");

    if(tit)
        initFolderMap(tit, "", ioNode);

    serverThread = unisetstd::make_unique<ThreadCreator<OPCUAServer>>(this, &OPCUAServer::serverLoop);
    serverThread->setFinalAction(this, &OPCUAServer::serverLoopTerminate);
    updateThread = unisetstd::make_unique<ThreadCreator<OPCUAServer>>(this, &OPCUAServer::updateLoop);

    // определяем фильтр
    s_field = conf->getArg2Param("--" + argprefix + "filter-field", it.getProp("filterField"));
    s_fvalue = conf->getArg2Param("--" + argprefix + "filter-value", it.getProp("filterValue"));
    auto regexp_fvalue = conf->getArg2Param("--" + argprefix + "filter-value-re", it.getProp("filterValueRE"));

    if( !regexp_fvalue.empty() )
    {
        try
        {
            s_fvalue = regexp_fvalue;
            s_fvalue_re = std::regex(regexp_fvalue);
        }
        catch( const std::regex_error& e )
        {
            ostringstream err;
            err << myname << "(init): '--" + argprefix + "filter-value-re' regular expression error: " << e.what();
            throw uniset3::SystemError(err.str());
        }
    }

    vmonit(s_field);
    vmonit(s_fvalue);
    vmonit(propPrefix);

    myinfo << myname << "(init): read s_field='" << s_field
           << "' s_fvalue='" << s_fvalue << "'"
           << " propPrefx='" << propPrefix << "'"
           << endl;

    if( !shm->isLocalwork() )
        ic->addReadItem(sigc::mem_fun(this, &OPCUAServer::readItem));
    else
        readConfiguration();

    // делаем очередь большой по умолчанию
    if( shm->isLocalwork() )
    {
        int sz = conf->getArgPInt("--uniset-object-size-message-queue", conf->getField("SizeOfMessageQueue"), 10000);

        if( sz > 0 )
            setMaxSizeOfMessageQueue(sz);
    }
}
// -----------------------------------------------------------------------------
OPCUAServer::~OPCUAServer()
{
}
// -----------------------------------------------------------------------------
void OPCUAServer::initFolderMap( uniset3::UniXML::iterator it, const std::string& parent_name, std::unique_ptr<IONode>& parent )
{
    if( !it.goChildren() )
    {
        return;
    }

    for( ; it.getCurrent(); it++ )
    {
        std::string cname = it.getProp("name");

        if( cname.empty() )
        {
            ostringstream err;
            err << myname << ": not found 'name' for folder <" << it.getName() << ">" << endl;
            throw uniset3::SystemError(err.str());
        }

        std::string desc = it.getProp("description");

        auto folder_name = cname;

        if(!parent_name.empty())
            folder_name = parent_name + "." + folder_name;

        try
        {
            auto ioNode = unisetstd::make_unique<IONode>(parent->node.addFolder(opcua::NodeId(0, folder_name), cname));
            ioNode->node.writeDescription({"en-US", desc});

            UniXML::iterator tit = it;
            initFolderMap( tit, folder_name, ioNode );

            myinfo << myname << "(initFolderMap): add new folder=" << folder_name << "(" << desc << ")" << endl;
            foldermap.emplace(folder_name, std::move(ioNode));
        }
        catch(const opcua::BadStatus& status)
        {
            ostringstream err;
            err << myname << ": catch <" << status.what() << "> on " << folder_name << endl;
            throw uniset3::SystemError(err.str());
        }
    }
}
//------------------------------------------------------------------------------
void OPCUAServer::readConfiguration()
{
    xmlNode* root = uniset_conf()->getXMLSensorsSection();

    if(!root)
    {
        ostringstream err;
        err << myname << "(readConfiguration): section <sensors> not found";
        throw SystemError(err.str());
    }

    UniXML::iterator it(root);

    if( !it.goChildren() )
    {
        mywarn << myname << "(readConfiguration): section <sensors> empty?!!" << endl;
        return;
    }

    for( ; it; it++ )
    {
        if( s_fvalue_re.has_value()  )
        {
            if( uniset3::check_filter_re(it, s_field, *s_fvalue_re) )
                initVariable(it);
        }
        else if( uniset3::check_filter(it, s_field, s_fvalue) )
            initVariable(it);
    }

    myinfo << myname << "(readConfiguration): init " << variables.size() << " variables" << endl;
}
// -----------------------------------------------------------------------------
bool OPCUAServer::readItem( const std::shared_ptr<UniXML>& xml, UniXML::iterator& it, xmlNode* sec )
{
    if( s_fvalue_re.has_value()  )
    {
        if( uniset3::check_filter_re(it, s_field, *s_fvalue_re) )
            initVariable(it);
    }
    else if( uniset3::check_filter(it, s_field, s_fvalue) )
        initVariable(it);

    return true;
}
//------------------------------------------------------------------------------
bool OPCUAServer::initVariable( UniXML::iterator& it )
{
    string sname = it.getProp("name");
    ObjectId sid = DefaultObjectId;

    if( it.getProp("id").empty() )
        sid = uniset_conf()->getSensorID(sname);
    else
        sid = it.getPIntProp("id", DefaultObjectId);

    if( sid == DefaultObjectId )
    {
        mycrit << myname << "(initVariable): (" << DefaultObjectId << ") Unknown Sensor ID for "
               << sname << endl;
        return false;
    }

    if( smTestID == DefaultObjectId )
        smTestID = sid;

    auto realIOType = uniset3::getIOType(it.getProp("iotype"));

    if( realIOType == uniset3::UnknownIOType )
    {
        mycrit << myname << "(initVariable): Unknown iotype " << it.getProp("iotype")
               << " for " << sname << endl;
        return false;
    }

    uniset3::IOType iotype = uniset3::AO; // by default

    auto rwmode = it.getProp(propPrefix + "opcua_rwmode");

    if( rwmode == "none" )
    {
        myinfo << myname << "(initVariable): rwmode='none'. Skip sensor " << sname << endl;
        return false;
    }

    if( rwmode == "w" )
    {
        if( realIOType == uniset3::DI || realIOType == uniset3::DO )
            iotype = uniset3::DI; // write access
        else
            iotype = uniset3::AI; // write access

        writeCount++;
    }
    else
    {
        if( realIOType == uniset3::DI || realIOType == uniset3::DO )
            iotype = uniset3::DO; // read access
        else
            iotype = uniset3::AO; // read access
    }

    auto vtype = it.getProp(propPrefix + "opcua_type");
    opcua::DataTypeId opctype = DefaultVariableType;

    if( vtype == "bool" )
        opctype = opcua::DataTypeId::Boolean;
    else if( vtype == "float" )
        opctype = opcua::DataTypeId::Float;
    else if( iotype == uniset3::DI || iotype == uniset3::DO )
        opctype = opcua::DataTypeId::Boolean;

    uint8_t precision = (uint8_t)it.getIntProp(propPrefix + "precision");

    DefaultValueUType mask = 0;
    uint8_t offset =  0;

    auto smask = it.getProp(propPrefix + "opcua_mask");

    if( !smask.empty() )
    {
        mask = uni_atoi(smask);
        offset = firstBit(mask);
    }

    sname = namePrefix + it.getProp2(propPrefix + "opcua_name", sname);

    auto folder = it.getProp(propPrefix + "opcua_folder");

    uniset3::OPCUAServer::IONode* node = nullptr;

    if( !folder.empty() )
    {
        if( foldermap.find(folder) == foldermap.end() )
        {
            ostringstream err;
            err << myname << "(initVariable): wrong opcua_folder for sensor " << sname << endl;
            throw SystemError(err.str());
        }

        node = foldermap[folder].get();
    }
    else
    {
        myinfo << myname << "(initVariable): opcua_folder not found.Using root folder for sensor " << sname << endl;
        node = ioNode.get();
    }

    if(!node)
    {
        ostringstream err;
        err << myname << "(initVariable): can't get node?!";
        throw SystemError(err.str());
    }

    auto desc = it.getProp2(propPrefix + "opcua_description", it.getProp("textname"));
    auto descLang = it.getProp2(propPrefix + "opcua_description_lang", "ru");

    auto displayName = it.getProp2(propPrefix + "opcua_displayname", sname);
    auto displayNameLang = it.getProp2(propPrefix + "opcua_displayname_lang", "en");

    auto method = it.getProp(propPrefix + "opcua_method");

    if(!method.empty())
    {
        if( iotype == uniset3::DO || iotype == uniset3::AO )
        {
            ostringstream err;
            err << myname << "(initVariable): wrong rwmode='r' for method sensor " << sname << endl;
            throw SystemError(err.str());
        }

        //Инициализация метода
        opcua::NodeId methodNodeId = opcua::NodeId(0, sname);

        UA_MethodAttributes attr = UA_MethodAttributes_default;
        attr.description = UA_LOCALIZEDTEXT_ALLOC(descLang.c_str(), desc.c_str());
        attr.displayName = UA_LOCALIZEDTEXT_ALLOC(displayNameLang.c_str(), displayName.c_str());
        attr.executable = true;

        UA_QualifiedName methodBrowseName = UA_QUALIFIEDNAME_ALLOC(methodNodeId.getNamespaceIndex(), sname.c_str());

        UA_Argument inputArguments;
        UA_Argument_init(&inputArguments);
        inputArguments.valueRank = UA_VALUERANK_SCALAR;

        if( iotype == uniset3::DI )
        {
            inputArguments.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
            inputArguments.description = UA_LOCALIZEDTEXT("en-US", "state");
            inputArguments.name = UA_STRING("state");
        }
        else if( iotype == uniset3::AI )
        {
            if( opctype == opcua::DataTypeId::Float )
            {
                inputArguments.dataType = UA_TYPES[UA_TYPES_FLOAT].typeId;
                inputArguments.description = UA_LOCALIZEDTEXT("en-US", "float value");
                inputArguments.name = UA_STRING("float value");
            }
            else
            {
                inputArguments.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
                inputArguments.description = UA_LOCALIZEDTEXT("en-US", "value");
                inputArguments.name = UA_STRING("value");
            }
        }

        UA_StatusCode result = UA_Server_addMethodNode(opcServer->handle(),
                               *methodNodeId.handle(), //requestedNewNodeId
                               *node->node.id().handle(),//parentNodeId
                               UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),//referenceTypeId
                               methodBrowseName,
                               attr,
                               &UA_setValueMethod,
                               1, &inputArguments,
                               0, nullptr,
                               this, nullptr);

        if (result == UA_STATUSCODE_GOOD)
        {
            auto i = methods.emplace(methodNodeId.hash(), sid);
            i.first->second.precision = precision;//only for float value
            methodCount++;
            shm->initIterator(i.first->second.it);
            myinfo << myname << "(initVariable): init method for " << sname << "(" << sid << ")" << endl;
        }

        UA_NodeId_clear(methodNodeId.handle());
        UA_MethodAttributes_clear(&attr);
        UA_QualifiedName_clear(&methodBrowseName);

        if (result != UA_STATUSCODE_GOOD)
        {
            mycrit << myname << "(initVariable): can't init method for " << sname << "(" << sid << ")" << endl;
            return false;
        }

        return true;
    }

    // s=xxx or ns=xxx
    if( sname[1] != '=' && sname[2] != '=' )
        sname = "s=" + sname;

    auto vnode = node->node.addVariable(UA_NODEID(sname.c_str()), sname);
    vnode.writeDataType(opctype);
    vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ);

    if( iotype == uniset3::AI || iotype == uniset3::DI )
        vnode.writeAccessLevel(UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE);

    vnode.writeDescription({descLang, desc});
    vnode.writeDisplayName({displayNameLang, displayName});

    // init default value
    DefaultValueType defVal = (DefaultValueType)it.getPIntProp("default", 0);

    if( opctype == opcua::DataTypeId::Boolean )
    {
        bool set = defVal ? true : false;
        vnode.writeValueScalar(set);
    }
    else if( opctype == opcua::DataTypeId::Float )
    {
        float v = defVal;
        vnode.writeValueScalar(v);
    }
    else
        vnode.writeValueScalar(defVal);

    auto i = variables.emplace(sid, vnode);
    i.first->second.stype = iotype;
    i.first->second.mask = mask;
    i.first->second.offset = offset;
    i.first->second.vtype = opctype;
    i.first->second.precision = precision;

    shm->initIterator(i.first->second.it);
    return true;
}
//------------------------------------------------------------------------------
void OPCUAServer::callback() noexcept
{
    try
    {
        // вызываем базовую функцию callback вместо UObject_SK::callback!
        // (оптимизация, чтобы не делать лишних проверок не нужных в этом процессе)
        UniSetObject::callback();
    }
    catch( const std::exception& ex )
    {
        mycrit << myname << "(callback): catch " << ex.what()  <<   endl;
    }
}
// -----------------------------------------------------------------------------
bool OPCUAServer::isExists()
{
    bool ret = UObject_SK::isExists();

    if( !ret )
        return false;

    return active;
}
//------------------------------------------------------------------------------
void OPCUAServer::sysCommand( const uniset3::umessage::SystemMessage* sm )
{
    UObject_SK::sysCommand(sm);

    if( sm->cmd() == SystemMessage::StartUp )
    {
        active  = true;
        myinfo << myname << "(sysCommand): init " << variables.size() << " variables" << endl;
        serverThread->start();
        updateThread->start();
    }
}
// -----------------------------------------------------------------------------
void OPCUAServer::serverLoopTerminate()
{
    opcServer->stop();
}
// -----------------------------------------------------------------------------
void OPCUAServer::serverLoop()
{
    if( opcServer == nullptr )
        return;

    PassiveTimer ptAct(activateTimeout);

    while( (!activated || !firstUpdate.load()) && !ptAct.checkTime() )
    {
        myinfo << myname << "(serverLoop): wait activate..." << endl;
        msleep(300);
    }

    if( !firstUpdate )
        mywarn << myname << "(serverLoop): first update [FAILED]..." << endl;

    myinfo << myname << "(serverLoop): started..." << endl;
    opcServer->run();
    myinfo << myname << "(serverLoop): terminated..." << endl;
}
// -----------------------------------------------------------------------------
bool OPCUAServer::deactivateObject()
{
    activated = false;

    if( opcServer )
        opcServer->stop();

    if( updateThread )
    {
        if( updateThread->isRunning() )
            updateThread->join();
    }

    if( serverThread )
    {
        if( serverThread->isRunning() )
            serverThread->join();
    }

    return UObject_SK::deactivateObject();
}
// -----------------------------------------------------------------------------
void OPCUAServer::help_print()
{
    cout << "--opcua-name        - ID for rrdstorage. Default: OPCUAServer1. " << endl;
    cout << "--opcua-confnode    - configuration section name. Default: <NAME name='NAME'...> " << endl;
    cout << "--opcua-heartbeat-id name  - ID for heartbeat sensor." << endl;
    cout << "--opcua-heartbeat-max val  - max value for heartbeat sensor." << endl;
    cout << "--opcua-filter-field name  - Считывать список опрашиваемых датчиков, только у которых есть поле field" << endl;
    cout << "--opcua-filter-value val   - Считывать список опрашиваемых датчиков, только у которых field=value" << endl;
    cout << "--opcua-sm-ready-timeout   - Время ожидания готовности SM к работе, мсек. (-1 - ждать 'вечно')" << endl;
    cout << "--opcua-sm-test-sid        - Использовать указанный датчик, для проверки готовности SharedMemory" << endl;

    cout << endl;
    cout << "OPC UA:" << endl;
    cout << "--opcua-updatetime msec      - Пауза между обновлением информации в/из SM. По умолчанию 200" << endl;
    cout << "--opcua-host ip              - IP на котором слушает OPC UA сервер" << endl;
    cout << "--opcua-port port            - Порт на котором слушает OPC UA сервер" << endl;
    cout << "--opcua-maxSubscriptions num - Максимальное количество подписок" << endl;
    cout << "--opcua-maxSessions num      - Максимальное количество сессий" << endl;
    cout << endl;
    cout << "Logs:" << endl;
    cout << "--opcua-log-...            - log control" << endl;
    cout << "             add-levels ...  " << endl;
    cout << "             del-levels ...  " << endl;
    cout << "             set-levels ...  " << endl;
    cout << "             logfile filename" << endl;
    cout << "             no-debug " << endl;
    cout << "LogServer: " << endl;
    cout << "--opcua-run-logserver      - run logserver. Default: localhost:id" << endl;
    cout << "--opcua-logserver-host ip  - listen ip. Default: localhost" << endl;
    cout << "--opcua-logserver-port num - listen port. Default: ID" << endl;
    cout << LogServer::help_print("prefix-logserver") << endl;
}
// -----------------------------------------------------------------------------
std::shared_ptr<OPCUAServer> OPCUAServer::init_opcua_server(int argc, const char* const* argv,
        uniset3::ObjectId icID, const std::shared_ptr<SharedMemory>& ic,
        const std::string& prefix )
{
    auto conf = uniset_conf();

    string name = conf->getArgParam("--" + prefix + "-name", "OPCUAServer");

    if( name.empty() )
    {
        cerr << "(OPCUAServer): Unknown name. Usage: --" <<  prefix << "-name" << endl;
        return nullptr;
    }

    ObjectId ID = conf->getObjectID(name);

    if( ID == uniset3::DefaultObjectId )
    {
        cerr << "(OPCUAServer): Not found ID for '" << name
             << " in '" << conf->getObjectsSection() << "' section" << endl;
        return nullptr;
    }

    string confname = conf->getArgParam("--" + prefix + "-confnode", name);
    xmlNode* cnode = conf->getNode(confname);

    if( !cnode )
    {
        cerr << "(OPCUAServer): " << name << "(init): Not found <" + confname + ">" << endl;
        return nullptr;
    }

    return make_shared<OPCUAServer>(ID, cnode, icID, ic, prefix);
}
// -----------------------------------------------------------------------------
void OPCUAServer::askSensors( uniset3::UIOCommand cmd )
{
    if( !shm->isLocalwork() )
        return;

    for(  auto it = variables.begin(); it != variables.end(); it++ )
    {
        try
        {
            if( it->second.stype == uniset3::AO || it->second.stype == uniset3::DO )
                shm->askSensor(it->first, cmd, getId());
        }
        catch( const uniset3::Exception& ex )
        {
            mycrit << myname << "(askSensors): " << ex << endl;
        }
    }
}
// -----------------------------------------------------------------------------
void OPCUAServer::sensorInfo( const uniset3::umessage::SensorMessage* sm )
{
    mylog4 << myname << "(sensorInfo): sm->id=" << sm->id() << " val=" << sm->value() << endl;
    auto it = variables.find(sm->id());

    if( it == variables.end() )
        return;

    try
    {
        if( it->second.stype == uniset3::DO || it->second.stype == uniset3::AO )
        {
            uniset3::uniset_rwmutex_wrlock lock(it->second.vmut);
            it->second.state = sm->value() ? true : false;
            it->second.value = getBits(sm->value(), it->second.mask, it->second.offset);
        }
    }
    catch( std::exception& ex )
    {
        mycrit << myname << "(sensorInfo): " << ex.what() << endl;
    }
}
// -----------------------------------------------------------------------------
void OPCUAServer::updateLoop()
{
    myinfo << myname << "(updateLoop): started..." << endl;
    PassiveTimer ptAct(activateTimeout);

    while( !activated && !ptAct.checkTime() )
    {
        myinfo << myname << "(updateLoop): wait activate..." << endl;
        msleep(300);
    }

    firstUpdate = false;
    update();
    myinfo << myname << "(updateLoop): first update [ok].." << endl;
    firstUpdate = true;

    while( activated )
    {
        update();
        msleep(updateTime_msec);
    }

    myinfo << myname << "(updateLoop): terminated.." << endl;
}
// ------------------------------------------------------------------------------------------
uint8_t OPCUAServer::firstBit( OPCUAServer::DefaultValueUType mask )
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(mask);
#else
    uint8_t n = 0;

    while( mask != 0 )
    {
        if( mask & 1 )
            break;

        mask = (mask >> 1);
        n++;
    }

    return n;
#endif
}
// ------------------------------------------------------------------------------------------
OPCUAServer::DefaultValueUType OPCUAServer::forceSetBits(
    OPCUAServer::DefaultValueUType value,
    OPCUAServer::DefaultValueUType set,
    OPCUAServer::DefaultValueUType mask,
    uint8_t offset )
{
    if( mask == 0 )
        return set;

    return OPCUAServer::setBits(value, set, mask, offset);
}
// ------------------------------------------------------------------------------------------
OPCUAServer::DefaultValueUType OPCUAServer::setBits(
    OPCUAServer::DefaultValueUType value,
    OPCUAServer::DefaultValueUType set,
    OPCUAServer::DefaultValueUType mask,
    uint8_t offset )
{
    if( mask == 0 )
        return value;

    return (value & (~mask)) | ((set << offset) & mask);
}
// ------------------------------------------------------------------------------------------
OPCUAServer::DefaultValueUType OPCUAServer::getBits(
    OPCUAServer::DefaultValueUType value,
    OPCUAServer::DefaultValueUType mask,
    uint8_t offset )
{
    if( mask == 0 )
        return value;

    return (value & mask) >> offset;
}
// ------------------------------------------------------------------------------------------
void OPCUAServer::update()
{
    auto t_start = std::chrono::steady_clock::now();

    for( auto it = this->variables.begin(); it != this->variables.end(); it++ )
    {
        try
        {
            if( !this->shm->isLocalwork() )
            {
                if( it->second.stype == uniset3::DO )
                {
                    it->second.state = shm->localGetValue(it->second.it, it->first) ? true : false;
                    it->second.value = getBits(it->second.state ? 1 : 0, it->second.mask, it->second.offset);
                }
                else if( it->second.stype == uniset3::AO )
                {
                    it->second.value = getBits(shm->localGetValue(it->second.it, it->first), it->second.mask, it->second.offset);
                    it->second.state = it->second.value ? true : false;
                }
            }

            if( it->second.stype == uniset3::DO )
            {
                uniset3::uniset_rwmutex_rlock lock(it->second.vmut);
                it->second.node.writeValueScalar(it->second.state);
            }
            else if( it->second.stype == uniset3::AO )
            {
                if( it->second.vtype == opcua::DataTypeId::Float )
                {
                    uniset3::uniset_rwmutex_rlock lock(it->second.vmut);
                    float fval = (float)it->second.value / pow(10.0, it->second.precision);
                    it->second.node.writeValueScalar( fval );
                }
                else
                {
                    uniset3::uniset_rwmutex_rlock lock(it->second.vmut);
                    it->second.node.writeValueScalar(it->second.value);
                }
            }
            else if( it->second.stype == uniset3::DI )
            {
                uniset3::uniset_rwmutex_rlock lock(it->second.vmut);
                auto set = it->second.node.readValueScalar<bool>();
                auto val = getBits(set ? 1 : 0, it->second.mask, it->second.offset);
                mylog6 << this->myname << "(updateLoop): sid=" << it->first
                       << " set=" << set
                       << " mask=" << it->second.mask
                       << endl;
                this->shm->localSetValue(it->second.it, it->first, val, this->getId());
            }
            else if( it->second.stype == uniset3::AI )
            {
                uniset3::uniset_rwmutex_rlock lock(it->second.vmut);
                DefaultValueType val = 0;

                if( it->second.vtype == opcua::DataTypeId::Float )
                    val = lroundf( it->second.node.readValueScalar<float>() * pow(10.0, it->second.precision) );
                else
                    val = getBits(it->second.node.readValueScalar<DefaultValueType>(), it->second.mask, it->second.offset);

                mylog6 << this->myname << "(updateLoop): sid=" << it->first
                       << " value=" << val
                       << " mask=" << it->second.mask
                       << " precision=" << it->second.precision
                       << endl;

                this->shm->localSetValue(it->second.it, it->first, val, this->getId());
            }
        }
        catch( const std::exception& ex )
        {
            mycrit << this->myname << "(updateLoop): sid=" << it->first
                   << "[" << it->second.stype
                   << "][" << (int)it->second.vtype
                   << "] update error: " << ex.what() << endl;
        }
    }

    auto t_end = std::chrono::steady_clock::now();
    mylog8 << myname << "(update): " << setw(10) << setprecision(7) << std::fixed
           << std::chrono::duration_cast<std::chrono::duration<float>>(t_end - t_start).count() << " sec" << endl;
}
// -----------------------------------------------------------------------------
std::string OPCUAServer::getMonitInfo() const
{
    ostringstream inf;
    inf << "   iolist: " << variables.size() << endl;
    inf << "write(in): " << writeCount << endl;
    inf << "read(out): " << variables.size() - writeCount << endl;
    inf << "methods(call): " << methodCount << endl;
    return inf.str();
}
// -----------------------------------------------------------------------------
UA_StatusCode OPCUAServer::UA_setValueMethod(UA_Server* server,
        const UA_NodeId* sessionId, void* sessionHandle,
        const UA_NodeId* methodId, void* methodContext,
        const UA_NodeId* objectId, void* objectContext,
        size_t inputSize, const UA_Variant* input,
        size_t outputSize, UA_Variant* output)
{
    if(inputSize != 1)
        return UA_STATUSCODE_BADARGUMENTSMISSING;

    OPCUAServer* srv = static_cast<OPCUAServer*>(methodContext);

    if(!srv)
        return UA_STATUSCODE_BADINTERNALERROR;

    auto it = srv->methods.find(UA_NodeId_hash(methodId));

    if( it == srv->methods.end() )
        return UA_STATUSCODE_BADMETHODINVALID;

    bool ret = false;
    DefaultValueType value = 0;

    // Корректность типов проверяется на уровне libopen62541(смотри описание opcua_method в OPCUAServer.h) перед вызовом обработчика UA_setValueMethod.
    if( input[0].type == &UA_TYPES[UA_TYPES_INT32] )
    {
        value = *(UA_Int32*) input[0].data;
    }
    else if( input[0].type == &UA_TYPES[UA_TYPES_BOOLEAN] )
    {
        value = *(UA_Boolean*) input[0].data ? 1 : 0;
    }
    else if( input[0].type == &UA_TYPES[UA_TYPES_FLOAT] )
    {
        value = lroundf( ( *(UA_Float*) input[0].data ) * pow(10.0, it->second.precision) );
    }
    else
        return UA_STATUSCODE_BADINVALIDARGUMENT;

    if(srv->log()->is_level6())
        srv->log()->level6() << srv->myname << "(update): " << value
                             << " nodeid: " << std::string_view((char*)methodId->identifier.string.data, methodId->identifier.string.length)
                             << " sid: "    << it->second.sid << endl;

    try
    {
        srv->shm->localSetValue(it->second.it, it->second.sid, value, srv->getId());
        ret = true;
    }
    catch( const std::exception& ex )
    {
        if(srv->log()->is_crit())
            srv->log()->crit() << srv->myname << "(UA_setValueMethod): (hb) " << ex.what() << std::endl;
    }


    if ( ret )
        return UA_STATUSCODE_GOOD;

    return UA_STATUSCODE_BAD;
}
// -----------------------------------------------------------------------------
