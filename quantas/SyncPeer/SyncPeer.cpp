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

#include "SyncPeer.hpp"

namespace quantas {

static bool registerSyncPeer = []() {
    PeerRegistry::registerPeerType("SyncPeer", [](interfaceId pubId) {
        return new SyncPeer(new NetworkInterfaceAbstract(pubId));
    });
    return true;
}();

SyncPeer::SyncPeer(NetworkInterface *interfacePtr) : Peer(interfacePtr) {}

SyncPeer::SyncPeer(const SyncPeer &rhs) : Peer(rhs) {
    messagesSent = rhs.messagesSent;
    computationCount = rhs.computationCount;
    SentRound = rhs.SentRound;
    SafeRound = rhs.SafeRound;
    neighborsAckFrom = rhs.neighborsAckFrom;
}

SyncPeer::~SyncPeer() = default;

void SyncPeer::performComputation() {
    if (SentRound <= SafeRound) {
        std::cout << publicId() << " completed a computation" << std::endl;
        computationCount++;
        SentRound = RoundManager::currentRound();
        json msg;
        msg["round"] = SentRound;
        msg["action"] = "computation";
        for (const auto &nbr : neighbors()) {
            unicastTo(msg, nbr);
        }
        messagesSent += neighbors().size();
    } else {
        while (!inStreamEmpty()) {
            Packet packet = popInStream();
            interfaceId source = packet.sourceId();
            json Message = packet.getMessage();
            if (Message["action"] == "ack" &&
                neighbors().find(source) != neighbors().end() &&
                Message["round"] == SentRound) {
                std::cout << publicId()
                          << " received acknowledgement message from " << source
                          << std::endl;
                neighborsAckFrom++;
                if (neighborsAckFrom == neighbors().size()) {
                    SafeRound = RoundManager::currentRound();
                    neighborsAckFrom = 0;
                }
            } else if (Message["action"] == "computation") {
                json ackMsg;
                ackMsg["round"] = Message["round"];
                ackMsg["action"] = "ack";
                std::cout << publicId() << " sending acknowledgement to "
                          << source << std::endl;
                unicastTo(ackMsg, source);
            }
        }
    }
}

void SyncPeer::initParameters(const std::vector<Peer *> &_peers) {
    // no specific parameters to initialize for this peer type
}

void SyncPeer::endOfRound(std::vector<Peer *> &peers) {
    if (RoundManager::currentRound() >= RoundManager::lastRound()) {
        double computations = 0.0;
        double networkCost = 0.0;
        for (const auto *peer :
             reinterpret_cast<const std::vector<SyncPeer *> &>(peers)) {
            computations += peer->computationCount;
            networkCost += peer->messagesSent;
        }
        LogWriter::pushValue("messages", networkCost);
        LogWriter::pushValue("computations", computations);
    }
}

} // namespace quantas