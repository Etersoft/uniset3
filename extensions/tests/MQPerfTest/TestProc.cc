#include <iomanip>
#include "Exceptions.h"
#include "TestProc.h"
// -----------------------------------------------------------------------------
using namespace std;
using namespace uniset3;
// -----------------------------------------------------------------------------
TestProc::TestProc( uniset3::ObjectId id, xmlNode* confnode ):
    TestProc_SK( id, confnode )
{
}
// -----------------------------------------------------------------------------
TestProc::~TestProc()
{
}
// -----------------------------------------------------------------------------
bool TestProc::isFullQueue()
{
    return ( getCountOfLostMessages() > 0 );
}
// -----------------------------------------------------------------------------
TestProc::TestProc()
{
    cerr << ": init failed!!!!!!!!!!!!!!!" << endl;
    throw Exception();
}
// -----------------------------------------------------------------------------
