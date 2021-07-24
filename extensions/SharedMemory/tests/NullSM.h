// --------------------------------------------------------------------------
#ifndef NullSM_H_
#define NullSM_H_
// --------------------------------------------------------------------------
#include <string>
#include "IONotifyController.h"
// --------------------------------------------------------------------------
class NullSM:
    public uniset3::IONotifyController
{
    public:
        NullSM( uniset3::ObjectId id, const std::string& datfile );

        virtual ~NullSM();

    protected:

        virtual void logging( uniset3::messages::SensorMessage& sm ) override {};

    private:

};
// --------------------------------------------------------------------------
#endif // NullSM_H_
// --------------------------------------------------------------------------
