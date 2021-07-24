// -----------------------------------------------------------------------------
#ifndef LostPassiveTestProc_H_
#define LostPassiveTestProc_H_
// -----------------------------------------------------------------------------
#include <unordered_map>
#include "Debug.h"
#include "LostTestProc_SK.h"
// -----------------------------------------------------------------------------
/* Пассивный процесс, который только заказывает датчики, но не выставляет */
class LostPassiveTestProc:
    public LostTestProc_SK
{
    public:
        LostPassiveTestProc( uniset3::ObjectId id, xmlNode* confnode = uniset3::uniset_conf()->getNode("LostPassiveTestProc") );
        virtual ~LostPassiveTestProc();

        bool emptyQueue();
        long checkValue( uniset3::ObjectId sid );

    protected:
        LostPassiveTestProc();

        virtual void askSensors( uniset3::UIOCommand cmd ) override;
        virtual void sensorInfo( const uniset3::messages::SensorMessage* sm ) override;

        std::mutex mut;
        std::unordered_map<uniset3::ObjectId, long> slist;

    private:
};
// -----------------------------------------------------------------------------
#endif // LostPassiveTestProc_H_
// -----------------------------------------------------------------------------
