// Copyright (c) 2016, Baidu.com, Inc. All Rights Reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Author: Kai Zhang (cs.zhangkai@outlook.com)

#include "authenticator.h"

#include <memory>
#include "storage/tree_struct.h"
#include "common/const.h"

namespace orion {
namespace server {

const std::string Authenticator::user_prefix("/user/");

int32_t Authenticator::add(const std::string& user, const std::string& token) {
    if (!validate(user)) {
        return status_code::INVALID;
    }
    if (_users.find(user) != _users.end()) {
        return status_code::EXISTED;
    }
    storage::ValueInfo value = { false, token, "" };
    std::unique_ptr<storage::TreeStructure> tree(
            new storage::TreeStructure(_underlying));
    int32_t ret = tree->put(common::INTERNAL_NS, user_prefix + user, value);
    if (ret != status_code::OK) {
        return ret;
    }
    std::lock_guard<std::mutex> locker(_mutex);
    _users[user] = token;
    return status_code::OK;
}

int32_t Authenticator::del(const std::string& user) {
    std::unique_lock<std::mutex> locker(_mutex);
    auto it = _users.find(user);
    if (it == _users.end()) {
        return status_code::INVALID;
    }
    locker.unlock();
    std::unique_ptr<storage::TreeStructure> tree(
            new storage::TreeStructure(_underlying));
    int32_t ret = tree->remove(common::INTERNAL_NS, user_prefix + user);
    if (ret != status_code::OK) {
        return ret;
    }
    locker.lock();
    _users.erase(it);
    return status_code::OK;
}

int32_t Authenticator::auth(const std::string& user, const std::string& token) {
    if (use_backdoor(user)) {
        return true;
    }
    std::lock_guard<std::mutex> locker(_mutex);
    auto it = _users.find(user);
    return it != _users.end() && it->second == token;
}

bool Authenticator::validate(const std::string& user) const {
    return !user.empty() && user != common::INTERNAL_NS &&
        user.find_first_of('/') != std::string::npos;
}

bool Authenticator::use_backdoor(const std::string& user) const {
    return user == common::INTERNAL_NS;
}

} // namespace server
} // namespace orion

