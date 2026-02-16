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
    computationPerformed = rhs.computationPerformed;
}

SyncPeerB::~SyncPeerB() = default;

void SyncPeerB::initParameters(const std::vector<Peer *> &_peers) {
    const std::vector<SyncPeerB *> peers =
        reinterpret_cast<std::vector<SyncPeerB *> const &>(_peers);

    // Each Child send an initial message to its neighbor (parent) to
    // familiarize itself
    for (SyncPeerB *peer : peers) {
        if (peer->neighbors().size() > 1) {
            peer->isRoot = true;
        } else {
            for (const auto &nbr : peer->neighbors()) {
                json msg;
                msg["action"] = "init";
                peer->unicastTo(msg, nbr);
            }
        }
    }

    // grab init messages from children, add to children vector for later
    for (SyncPeerB *peer : peers) {
        while (!peer->inStreamEmpty()) {
            Packet packet = peer->popInStream();
            interfaceId source = packet.sourceId();
            json Message = packet.getMessage();
            if (Message["action"] == "init") {
                peer->children.push_back(source);
            }
        }
    }
}

void SyncPeerB::performComputation() {
    if ((SentRound <= SafeRound) && !computationPerformed) {
        std::cout << publicId() << " completed a computation" << std::endl;
        computationCount++;
        computationPerformed = true;
    } else if (computationPerformed) {
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
    }
}

} // namespace quantas