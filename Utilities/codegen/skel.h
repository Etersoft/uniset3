// -----------------------------------------------------------------------------
#ifndef Skel_H_
#define Skel_H_
// -----------------------------------------------------------------------------
#include "Skel_SK.h"
// -----------------------------------------------------------------------------
class Skel:
    public Skel_SK
{
    public:
        Skel( uniset3::ObjectId id, xmlNode* confnode = uniset3::uniset_conf()->getNode("Skel") );
        virtual ~Skel();

    protected:
        Skel();

        virtual void step() override;
        virtual void sensorInfo( const uniset3::messages::SensorMessage* sm ) override;
        virtual void timerInfo( const uniset3::messages::TimerMessage* tm ) override;
        virtual void askSensors( uniset3::UIOCommand cmd ) override;

    private:
};
// -----------------------------------------------------------------------------
#endif // Skel_H_
// -----------------------------------------------------------------------------
