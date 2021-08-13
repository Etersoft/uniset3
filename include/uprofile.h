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
#ifndef uprofile_H_
#define uprofile_H_
// --------------------------------------------------------------------------
#include <chrono>
// --------------------------------------------------------------------------
#define UPROFILE_TIME_BEGIN \
std::chrono::time_point<std::chrono::system_clock> _start, _end; \
_start = std::chrono::system_clock::now();

#define UPROFILE_TIME_RESULT_MCS(comm) \
end = std::chrono::system_clock::now(); \
std::cerr << __LINE__ << ":" << __FILE__ << "(" << __FUNCTION__  << "): " << comm << elapsed time: " << std::chrono::duration_cast<std::chrono::microseconds>(_end - _start).count() << " ms\n";

#define UPROFILE_TIME_RESULT_MS(comm) \
end = std::chrono::system_clock::now(); \
std::cerr << __LINE__ << ":" << __FILE__ << "(" << __FUNCTION__  << "): " << comm << elapsed time: " << std::chrono::duration_cast<std::chrono::microseconds>(_end - _start).count() << " ms\n";

#define UPROFILE_TIME_RESULT_NS(comm) \
end = std::chrono::system_clock::now(); \
std::cerr << __LINE__ << ":" << __FILE__ << "(" << __FUNCTION__  << "): " << comm << elapsed time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(_end - _start).count() << " ms\n";
// --------------------------------------------------------------------------
