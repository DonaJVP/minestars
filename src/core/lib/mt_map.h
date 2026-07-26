/*
 * MineStars - MultiCraft - Minetest/Luanti
 * Copyright (C) 2025 Logiki, Donatto J. Viveros. P. <donatto555@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <mutex>
#include <unordered_map>

template<typename header, typename contents>
class MultithreadMap {
public:
    bool Has(header DATA) {
        std::lock_guard<std::mutex> lock(mtx);
        return _has(DATA);
    }

    //Just raw api
    bool _has(header DATA) {
        return _map.find(DATA) != _map.end();
    }

    void Set(header DATA, contents CTX) {
        std::lock_guard<std::mutex> lock(mtx);
        _map[DATA] = CTX;
    }
    bool Erase(header DATA) {
        std::lock_guard<std::mutex> lock(mtx);
        if (_has(DATA)) {
            _map.erase(DATA);
            return true;
        }
        return false;
    }
    contents Get(header DATA) {
        std::lock_guard<std::mutex> lock(mtx);
        if (_has(DATA)) {
            return _map.at(DATA);
        }
        return contents();
    }

    size_t getSize() {
        std::lock_guard<std::mutex> lock(mtx);
        return _map.size();
    }

    void Lock() {
        mtx.lock();
    }
    void unLock() {
        mtx.unlock();
    }
    std::unordered_map<header, contents> *GetRawMap() {
        return &_map;
    }
private:
    std::mutex mtx;
    std::unordered_map<header, contents> _map;
};
