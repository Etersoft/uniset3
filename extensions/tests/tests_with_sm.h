#ifndef tests_with_sm_H_
#define tests_with_sm_H_
// --------------------------------------------------------------------------
#include <memory>
#include "SharedMemory.h"
#include "SMInterface.h"
// --------------------------------------------------------------------------
// Для некоторых тестов необходим SMInterface инициализированный для работы с SharedMemory
// поэтому сделана такая специальная функция
// реализацию смотри в tests_with_sm.cc
std::shared_ptr<uniset3::SMInterface> smiInstance();
std::shared_ptr<uniset3::SharedMemory> shmInstance();
// --------------------------------------------------------------------------
#endif // tests_with_sm_H_
