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

#include "SyncPeerB.hpp"

namespace quantas {

static bool registerSyncPeerB = []() {
    PeerRegistry::registerPeerType("SyncPeerB", [](interfaceId pubId) {
        return new SyncPeerB(new NetworkInterfaceAbstract(pubId));
    });
    return true;
}();

SyncPeerB::SyncPeerB(NetworkInterface *interfacePtr) : Peer(interfacePtr) {}

SyncPeerB::SyncPeerB(const SyncPeerB &rhs) : Peer(rhs) {
    messagesSent = rhs.messagesSent;
    computationCount = rhs.computationCount;
    SentRound = rhs.SentRound;
    SafeRound = rhs.SafeRound;
    childrenAckFrom = rhs.childrenAckFrom;
    children = rhs.children;
    isRoot = rhs.isRoot;
    isInit = rhs.isInit;
}

SyncPeerB::~SyncPeerB() = default;

void SyncPeerB::performComputation() {
    if (!isInit) {
        if (neighbors().size() > 1) {
            isRoot = true;
            isInit = true;
        } else {
            for (const auto &nbr : neighbors()) {
                json msg;
                msg["action"] = "init";
                unicastTo(msg, nbr);
            }
            isInit = true;
        }
    } else {
        // Every node should check its instream every round to see if it has
        // received "init" or "safe" messages from its children, leaf skips over
        // this while loop at the start of every synchronized "round" of
        // computation
        while (!inStreamEmpty()) {
            Packet packet = popInStream();
            interfaceId source = packet.sourceId();
            json Message = packet.getMessage();
            if (Message["action"] == "init") {
                // This case should only happen in the first round, and is used
                // to initialize the children vector for each node
                children.push_back(source);
            } else if (Message["action"] == "safe" &&
                       std::find(children.begin(), children.end(), source) !=
                           children.end()) {
                childrenAckFrom++;
                if (childrenAckFrom == children.size() && !isRoot &&
                    SentRound <= SafeRound) {
                    computationCount++;
                    json msg;
                    SentRound = RoundManager::currentRound();
                    msg["action"] = "safe";
                    for (const auto &nbr : neighbors()) {
                        unicastTo(msg, nbr);
                    }
                    messagesSent += neighbors().size();
                    childrenAckFrom = 0;
                } else if (childrenAckFrom == children.size() && isRoot &&
                           SentRound <= SafeRound) {
                    computationCount++;
                    SentRound = SafeRound = RoundManager::currentRound();
                    json msg;
                    msg["round"] = SentRound;
                    msg["action"] = "pulse";
                    broadcast(msg);
                    messagesSent += neighbors().size();
                    childrenAckFrom = 0;
                    SynchedStepsBeta++;
                }
            } else if (Message["action"] == "pulse") {
                SafeRound = Message["round"];
            }
        }
        // leaf nodes case: has no children, thus should be permitted to
        // send messages after performing computation even with no messages
        // received
        if (children.size() == 0 && SentRound <= SafeRound) {
            computationCount++;
            json msg;
            SentRound = RoundManager::currentRound();
            msg["action"] = "safe";
            for (const auto &nbr : neighbors()) {
                unicastTo(msg, nbr);
            }
            messagesSent += neighbors().size();
        }
    }
}

void SyncPeerB::initParameters(const std::vector<Peer *> &_peers) {
    // no specific parameters to initialize for this peer type
}

void SyncPeerB::endOfRound(std::vector<Peer *> &_peers) {
    if (RoundManager::currentRound() >= RoundManager::lastRound()) {
        double computations = 0.0;
        double networkCost = 0.0;
        for (const auto *peer :
             reinterpret_cast<const std::vector<SyncPeerB *> &>(_peers)) {
            computations += peer->computationCount;
            networkCost += peer->messagesSent;
        }
        LogWriter::pushValue("messages", networkCost);
        LogWriter::pushValue("computations", computations);
        LogWriter::pushValue("synchronized steps", SynchedStepsBeta);
    }
}

} // namespace quantas