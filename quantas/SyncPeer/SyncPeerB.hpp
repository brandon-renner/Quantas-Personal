/*
Copyright 2026

This file is part of QUANTAS.
QUANTAS is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version. QUANTAS is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
QUANTAS. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef SYNCBPEER_HPP
#define SYNCBPEER_HPP

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../Common/Peer.hpp"

namespace quantas {

class SyncPeerB : public Peer {

  public:
    SyncPeerB(NetworkInterface *_netInterface);
    SyncPeerB(const SyncPeerB &rhs);
    ~SyncPeerB();

    void performComputation() override;
    void initParameters(const std::vector<Peer *> &_peers);
    void endOfRound(std::vector<Peer *> &_peers) override;

    int messagesSent = 0;
    int computationCount = 0;
    bool computationPerformed = false;
    int SentRound = 0;
    int SafeRound = 0;

    std::vector<interfaceId> children = {};
    bool isRoot = false;

    int childrenAckFrom = 0;
};

} // namespace quantas

#endif /* SYNCBPEER_HPP */