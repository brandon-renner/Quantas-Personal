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

#include <algorithm>
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
    int SentRound = -1;
    int SafeRound = -1;

    std::vector<interfaceId> children = {};
    bool isRoot = false;
    bool isInit = false;

    int childrenAckFrom = 0;
};

} // namespace quantas

#endif /* SYNCBPEER_HPP */

/*void SyncPeerB::performComputation() {

    if ((SentRound <= SafeRound) && !computationPerformed) {
    std::cout << publicId() << " completed a computation" << std::endl;
    computationCount++;
    computationPerformed = true;
}
else if (computationPerformed) {
    // All nodes, after performing computation, must listen for messages
    // being sent to them
    while (!inStreamEmpty()) {
        Packet packet = popInStream();
        interfaceId source = packet.sourceId();
        json Message = packet.getMessage();
        if (Message["action"] == "safe" &&
            std::find(children.begin(), children.end(), source) !=
                children.end()) {
            std::cout << publicId() << " received safe message from child "
                      << source << std::endl;
            childrenAckFrom++;
            if (childrenAckFrom == children.size() && !isRoot &&
                SentRound <= SafeRound) {
                json msg;
                SentRound = RoundManager::currentRound();
                msg["round"] = SentRound;
                msg["action"] = "safe";
                for (const auto &nbr : neighbors()) {
                    unicastTo(msg, nbr);
                }
                messagesSent += neighbors().size();
                childrenAckFrom = 0;
            } else if (childrenAckFrom == children.size() && isRoot &&
                       SentRound <= SafeRound) {
                SentRound = SafeRound = RoundManager::currentRound();
                json msg;
                msg["round"] = SentRound;
                msg["action"] = "pulse";
                for (const auto &nbr : neighbors()) {
                    unicastTo(msg, nbr);
                }
                messagesSent += neighbors().size();
                childrenAckFrom = 0;
                computationPerformed = false;
            }
        } else if (Message["action"] == "pulse") {
            std::cout << publicId() << " received pulse message from root "
                      << source << std::endl;
            SafeRound = Message["round"];
            computationPerformed = false;
        }
    }
    // leaf nodes case: has no children, thus should be permitted to
    // send messages after performing computation even with no messages
    // received
    if (children.size() == 0 && SentRound <= SafeRound) {
        json msg;
        SentRound = RoundManager::currentRound();
        msg["round"] = SentRound;
        msg["action"] = "safe";
        for (const auto &nbr : neighbors()) {
            unicastTo(msg, nbr);
        }
        messagesSent += neighbors().size();
    }
}
}
*/