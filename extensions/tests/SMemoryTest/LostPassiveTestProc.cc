#include <iomanip>
#include "Exceptions.h"
#include "LostPassiveTestProc.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// -----------------------------------------------------------------------------
LostPassiveTestProc::LostPassiveTestProc( uniset3::ObjectId id, xmlNode* confnode ):
    LostTestProc_SK( id, confnode )
{
    auto conf = uniset_conf();

    UniXML::iterator cit(confnode);
    string f_field = conf->getArgParam("--" + argprefix + "filter-field", cit.getProp("filterField"));
    string f_value = conf->getArgParam("--" + argprefix + "filter-value", cit.getProp("filterValue"));

    xmlNode* snode = conf->getXMLSensorsSection();

    if( !snode )
        throw SystemError(myname + "(init): Not found <sensors> section?!");

    UniXML::iterator it(snode);

    if( !it.goChildren() )
        throw SystemError(myname + "(init): Section <sensors> empty?!");

    for( ; it; it++ )
    {
        if( !uniset3::check_filter(it, f_field, f_value) )
            continue;

        if( it.getProp("iotype") != "AI" )
            continue;

        slist.emplace(it.getIntProp("id"), 0);
    }

    setMaxSizeOfMessageQueue(slist.size() * 2 + 500);

    smTestID = slist.begin()->first;
}
// -----------------------------------------------------------------------------
LostPassiveTestProc::~LostPassiveTestProc()
{
}
// -----------------------------------------------------------------------------
bool LostPassiveTestProc::emptyQueue()
{
    return ( countMessages() == 0 );
}
// -----------------------------------------------------------------------------
long LostPassiveTestProc::checkValue( ObjectId sid )
{
    std::lock_guard<std::mutex> lock(mut);
    auto s = slist.find(sid);

    if( s == slist.end() )
    {
        ostringstream err;
        err << myname << "(checkValue): NOT FOUND?!! sensor ID=" << sid;
        throw uniset3::SystemError(err.str());
    }

    return s->second;
}
// -----------------------------------------------------------------------------
LostPassiveTestProc::LostPassiveTestProc()
{
    cerr << ": init failed!!!!!!!!!!!!!!!" << endl;
    throw uniset3::Exception(myname + "(init): FAILED..");
}
// -----------------------------------------------------------------------------
void LostPassiveTestProc::askSensors(uniset3::UIOCommand cmd)
{
    for( const auto& s : slist )
        askSensor(s.first, cmd);
}
// -----------------------------------------------------------------------------
void LostPassiveTestProc::sensorInfo(const SensorMessage* sm)
{
    std::lock_guard<std::mutex> lock(mut);

    auto s = slist.find(sm->id);

    if( s == slist.end() )
    {
        mycrit << myname << "(sensorInfo): ERROR: message from UNKNOWN SENSOR sm->id=" << sm->id << endl;
        uniset3::SimpleInfo_var i = getInfo("");
        mycrit << i->info << endl;
        std::abort();
    }

    s->second = sm->value;
}
// -----------------------------------------------------------------------------
