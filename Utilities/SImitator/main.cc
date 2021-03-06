#include <iostream>
#include <algorithm>
#include "Exceptions.h"
#include "UInterface.h"
#include "IOController.grpc.pb.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// -----------------------------------------------------------------------------
void help_print()
{
    cout << endl;
    cout << "--sid id1@Node1,id2,..,idXX@NodeXX  - Аналоговые датчики (AI,AO)" << endl;
    cout << endl;
    cout << "--min val       - Нижняя граница датчика. По умолчанию 0" << endl;
    cout << "--max val       - Верхняя граница датчика. По умолчанию 100 " << endl;
    cout << "--step val      - Шаг датчика. По умолчанию 1" << endl;
    cout << "--pause msec    - Пауза. По умолчанию 200 мсек" << endl << endl;
    cout << uniset3::Configuration::help() << endl;
}
// -----------------------------------------------------------------------------
struct ExtInfo:
    public uniset3::ParamSInfo
{
    uniset3::IOType iotype;
};
// -----------------------------------------------------------------------------
int main( int argc, char** argv )
{
    //  std::ios::sync_with_stdio(false);

    try
    {
        // help
        // -------------------------------------
        if( argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) )
        {
            help_print();
            return 0;
        }

        // -------------------------------------

        auto conf = uniset_init(argc, argv, "configure.xml" );
        UInterface ui(conf);

        const string sid(conf->getArgParam("--sid"));

        if( sid.empty() )
        {
            cerr << endl << "Use --sid id1,..,idXX" << endl << endl;
            return 1;
        }

        auto lst = uniset3::getSInfoList(sid, conf);

        if( lst.empty() )
        {
            cerr << endl << "Use --sid id1,..,idXX" << endl << endl;
            return 1;
        }

        std::list<ExtInfo> l;
        bool useSync = false;

        for( auto&& it : lst )
        {
            uniset3::IOType t = conf->getIOType( it.si.id() );

            if( t != uniset3::AI && t != uniset3::AO )
            {
                cerr << endl << "WARNING! Неверный типа датчика '" << t << "' для id='" << it.fname << "'. Тип должен быть AI или AO." << endl << endl;
                // return 1;
            }

            if( it.si.node() == DefaultObjectId )
                it.si.set_node(conf->getLocalNode());
            else
                useSync = true; // если хоть один датчик на другом узле, используем удалённый вызов

            ExtInfo i;
            i.si = it.si;
            i.iotype = t;
            l.push_back(i);
        }

        int amin = conf->getArgInt("--min", "0");
        int amax = conf->getArgInt("--max", "100");

        if( amin > amax )
        {
            int temp = amax;
            amax = amin;
            amin = temp;
        }

        int astep = conf->getArgInt("--step", "1");

        if( astep <= 0 )
        {
            cerr << endl << "Ошибка, используйте --step val - любое положительное число" << endl << endl;
            return 1;
        }

        int amsec = conf->getArgInt("--pause", "200");

        if( amsec <= 10 )
        {
            cerr << endl << "Ошибка, используйте --pause val - любое положительное число > 10" << endl << endl;
            return 1;
        }

        cout << endl << "------------------------------" << endl;
        cout << " Вы ввели следующие параметры:" << endl;
        cout << "------------------------------" << endl;
        cout << "  sid = " << sid << endl;
        cout << "  min = " << amin << endl;
        cout << "  max = " << amax << endl;
        cout << "  step = " << astep << endl;
        cout << "  pause = " << amsec << endl;
        cout << "  sync_call = " << useSync << endl;
        cout << "------------------------------" << endl << endl;

        auto oref = ui.resolve(lst.begin()->si.id());

        if( !oref->c )
        {
            cerr << "can't resolve server: " << oref->ref.addr() << endl;
            return 1;
        }

        std::unique_ptr<IONotifyStreamController_i::Stub> stub(IONotifyStreamController_i::NewStub(oref->c));

        grpc::ClientContext ctx;
        SensorsStreamCmd request;
        request.set_cmd(uniset3::UIOSet);

        for( const auto& i : lst )
        {
            auto s = request.add_slist();
            s->set_id(i.si.id());
            s->set_val(i.val);
        }

        typedef std::unique_ptr< ::grpc::ClientReaderWriter< ::uniset3::SensorsStreamCmd, ::uniset3::umessage::SensorMessage>> StreamType;

        StreamType stream;

        if( !useSync )
            stream = stub->sensorsStream(&ctx);

        auto fSync = std::function<void(int)>([&](int i)
        {
            for( const auto& it : l )
            {
                try
                {
                    ui.setValue(it.si, i, DefaultObjectId);
                }
                catch( const uniset3::Exception& ex )
                {
                    cerr << endl << "save id=" << it.fname << " " << ex << endl;
                }
            }
        });

        auto fAsync = std::function<void(int)>([&](int i)
        {
            for( const auto& it : l )
            {
                for( int k = 0; k < request.slist_size(); k++ )
                    request.mutable_slist(k)->set_val(i);

                if( !stream->Write(request) )
                    cerr << endl << "write error.." << endl;
            }
        });

        int i = amin - astep, j = amax;

        while(1)
        {
            if(i >= amax)
            {
                j -= astep;

                if(j < amin)               // Принудительная установка нижней границы датчика
                    j = amin;

                cout << "\r" << " i = " << j << "     " << flush;

                if( useSync )
                    fSync(j);
                else
                    fAsync(j);

                if(j <= amin)
                {
                    i = amin;
                    j = amax;
                }
            }
            else
            {
                i += astep;

                if(i > amax)               // Принудительная установка верхней границы датчика
                    i = amax;

                cout << "\r" << " i = " << i << "     " << flush;

                if( useSync )
                    fSync(i);
                else
                    fAsync(i);
            }

            msleep(amsec);
        }
    }
    catch( const uniset3::Exception& ex )
    {
        cerr << endl << "(simitator): " << ex << endl;
        return 1;
    }
    catch( ... )
    {
        cerr << endl << "(simitator): catch..." << endl;
        return 1;
    }

    return 0;
}
// ------------------------------------------------------------------------------------------
