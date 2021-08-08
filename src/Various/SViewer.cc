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
 *  \brief Программа просмотра состояния датчиков
 *  \author Pavel Vainerman
 */
// --------------------------------------------------------------------------
#include <sstream>
#include <iomanip>
#include "SViewer.h"
#include "UniSetObject.pb.h"
#include "URepository.grpc.pb.h"
#include "IOController.grpc.pb.h"
#include "UniSetTypes.h"
#include "ObjectIndex_Array.h"

// --------------------------------------------------------------------------
using namespace uniset3;
using namespace std;
// --------------------------------------------------------------------------
SViewer::SViewer(const string& csec, bool sn):
    csec(csec),
    isShortName(sn)
{
    ui = make_shared<UInterface>(uniset3::uniset_conf());
    ui->setCacheMaxSize(500);
}

SViewer::~SViewer()
{
}
// --------------------------------------------------------------------------
void SViewer::monitor( timeout_t msec )
{
    for(;;)
    {
        view();
        msleep(msec);
    }
}

// --------------------------------------------------------------------------
void SViewer::view()
{
    try
    {
        readSection(csec, "");
    }
    catch( const uniset3::Exception& ex )
    {
        cerr << ex << endl;
    }
}

// ---------------------------------------------------------------------------

void SViewer::readSection( const string& section, const string& secRoot )
{
    uniset3::ObjectRefList lst;
    std::shared_ptr<grpc::Channel> chan;
    grpc::ClientContext ctx;

    google::protobuf::StringValue sec;

    if ( secRoot.empty() )
        sec.set_value(section);
    else
        sec.set_value(secRoot + "/" + section);

    try
    {
        chan = grpc::CreateChannel(uniset_conf()->repositoryAddr(), grpc::InsecureChannelCredentials());
        std::unique_ptr<URepository_i::Stub> stub(URepository_i::NewStub(chan));

        auto status = stub->list(&ctx, sec, &lst);

        if( !status.ok() )
        {
            cerr << "(readSection): error: " << status.error_message() << endl;
            return;
        }
    }
    catch(std::exception& ex)
    {
        cerr << "(readSection): error: " << ex.what() << endl;
    }
    catch(...)
    {
        std::exception_ptr p = std::current_exception();
        cerr << "(readSection): get section list: " << (p ? p.__cxa_exception_type()->name() : "catch..") << std::endl;
    }

    if( !lst.refs().empty() )
    {
        cout << " секция " << sec.value() << " не содержит объектов " << endl;
        return;
    }

    for( const auto& o : lst.refs() )
    {
        try
        {
            getSensorsInfo(o.id());
        }
        catch( const std::exception& ex )
        {
            cout << "(readSection): " << ex.what() << endl;
        }
    }
}
// ---------------------------------------------------------------------------
void SViewer::getSensorsInfo( uniset3::ObjectId iocontrollerID )
{
    std::shared_ptr<UInterface::ORefInfo> chan;
    GetSensorsMapParams request;
    request.set_id(iocontrollerID);
    uniset3::SensorIOInfoSeq smap;
    uniset3::ThresholdsListSeq tlist;
    uniset3::GetThresholdsListParams trequest;
    trequest.set_id(iocontrollerID);

    try
    {
        chan = ui->resolve(iocontrollerID);

        if( !chan )
        {
            cerr  << "(getSensorsInfo): grpc resolve failed. Controller ID=" << iocontrollerID << endl;
            return;
        }

        try
        {
            grpc::ClientContext ctx;
            chan->addMetaData(ctx);
            std::unique_ptr<IOController_i::Stub> ic(IOController_i::NewStub(chan->c));

            auto status = ic->getSensorsMap(&ctx, request, &smap);

            if( status.ok() )
                updateSensors(smap, iocontrollerID);
            else
                cerr << "(getSensorsMap): error: " << status.error_message() << endl;
        }
        catch( const uniset3::NameNotFound& ex ) {}
        catch(...) {}

        try
        {
            grpc::ClientContext ctx;
            chan->addMetaData(ctx);
            std::unique_ptr<IONotifyController_i::Stub> inc(IONotifyController_i::NewStub(chan->c));
            auto status = inc->getThresholdsList(&ctx, trequest, &tlist);

            if( status.ok() )
                updateThresholds(tlist, iocontrollerID);
            else
                cerr << "(getThresholdsList): error: " << status.error_message() << endl;
        }
        catch( const uniset3::NameNotFound& ex ) {}
        catch(...) {}

        return;
    }
    catch( const std::exception& ex )
    {
        cout << "(getSensorsInfo):" << ex.what() << endl;
    }
    catch(...)
    {
        std::exception_ptr p = std::current_exception();
        cout << "(getSensorsInfo): " << (p ? p.__cxa_exception_type()->name() : "catch..") << std::endl;
    }
}

// ---------------------------------------------------------------------------
void SViewer::updateSensors( uniset3::SensorIOInfoSeq& smap, uniset3::ObjectId oid )
{
    const string owner = ObjectIndex::getShortName(uniset_conf()->oind->getMapName(oid));
    cout << "\n======================================================\n"
         << ObjectIndex::getShortName(uniset_conf()->oind->getMapName(oid))
         << "\t Датчики"
         << "\n------------------------------------------------------"
         << endl;

    for(const auto& s : smap.sensors())
    {
        if( s.type() == uniset3::AI || s.type() == uniset3::DI )
        {
            string name = uniset_conf()->oind->getNameById(s.si().id());

            if( isShortName )
                name = ObjectIndex::getShortName(name);

            string supplier = ObjectIndex::getShortName(uniset_conf()->oind->getMapName(s.supplier()));

            if( s.supplier() == uniset3::AdminID )
                supplier = "uniset-admin";

            const string txtname = uniset_conf()->oind->getTextName(s.si().id());
            printInfo( s.si().id(), name, s.value(), supplier, txtname, (s.type() == uniset3::AI ? "AI" : "DI") );
        }
    }

    cout << "------------------------------------------------------\n";

    cout << "\n======================================================\n" << owner;
    cout << "\t Выходы";
    cout << "\n------------------------------------------------------" << endl;

    for(const auto& s : smap.sensors())
    {
        if( s.type() == uniset3::AO || s.type() == uniset3::DO )
        {
            string name = uniset_conf()->oind->getNameById(s.si().id());

            if( isShortName )
                name = ObjectIndex::getShortName(name);

            string supplier = ObjectIndex::getShortName(uniset_conf()->oind->getMapName(s.supplier()));

            if( s.supplier() == uniset3::AdminID )
                supplier = "uniset-admin";

            const string txtname( uniset_conf()->oind->getTextName(s.si().id()) );
            printInfo( s.si().id(), name, s.value(), supplier, txtname, (s.type() == uniset3::AO ? "AO" : "DO"));
        }
    }

    cout << "------------------------------------------------------\n";

}
// ---------------------------------------------------------------------------
void SViewer::updateThresholds( uniset3::ThresholdsListSeq& tlst, uniset3::ObjectId oid )
{
    const string owner = ObjectIndex::getShortName(uniset_conf()->oind->getMapName(oid));
    cout << "\n======================================================\n" << owner;
    cout << "\t Пороговые датчики";
    cout << "\n------------------------------------------------------" << endl;

    for(const auto& s : tlst.thresholds())
    {
        cout << "(" << setw(5) << s.si().id() << ") | ";

        switch( s.type() )
        {
            case uniset3::AI:
                cout << "AI";
                break;

            case uniset3::AO:
                cout << "AO";
                break;

            default:
                cout << "??";
                break;
        }

        string sname = uniset_conf()->oind->getNameById(s.si().id());

        if( isShortName )
            sname = ObjectIndex::getShortName(sname);

        cout << " | " << setw(60) << sname << " | " << setw(5) << s.value() << endl;

        for( const auto& t : s.tlist().thresholds() )
        {
            cout << "\t(" << setw(3) << t.id() << ")  |  " << setw(5) << t.state() << "  |  hi: " << setw(5) << t.hilimit();
            cout << " | low: " << setw(5) << t.lowlimit();
            cout << endl;
        }
    }
}
// ---------------------------------------------------------------------------

void SViewer::printInfo(uniset3::ObjectId id, const string& sname, long value, const string& supplier,
                        const string& txtname, const string& iotype)
{
    std::ios_base::fmtflags old_flags = cout.flags();
    cout << "(" << setw(5) << id << ")" << " | " << setw(2) << iotype << " | " << setw(60) << sname
         << "   | " << setw(5) << value << "\t | "
         << setw(40) << left << supplier << endl; // "\t | " << txtname << endl;
    cout.setf(old_flags);
}
// ---------------------------------------------------------------------------
