//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "tx.h"
#include "Fragment_m.h"

Define_Module(Tx);

void Tx::initialize() {
    destModule = getParentModule()->getSubmodule("rx");
    destGate = destModule->gate("wirelessInRx")->getId();

    fragmentSize = par("dimFragment").intValue();
    headerSize = par("dimHeader").intValue();
    channelBandwidth = par("bandwidthChannel").doubleValue();

    meanInterTimePkt = par("meanInterTimePkt").doubleValue();
    minPacketSize = par("minSizePkt").intValue();
    maxPacketSize = par("maxSizePkt").intValue();

    initializeSignals();

    // generation of first packet
    newPkt = new cMessage("packetReady");
    generateNewPacket();
}

void Tx::initializeSignals() {
    stats.signals.fragmentWaitingTime = registerSignal("fragmentWaitingTime");
    stats.signals.fragmentResponseTime = registerSignal("fragmentResponseTime");
    stats.signals.fragmentsInQueue = registerSignal("fragmentsInQueue");
    stats.signals.fragmentsInSystem = registerSignal("fragmentsInSystem");
    stats.signals.systemUtilization = registerSignal("systemUtilization");
    stats.signals.fragmentServiceTime = registerSignal("fragmentServiceTime");
    stats.signals.fragmentRetransmissions = registerSignal("fragmentRetransmissions");
    stats.signals.packetRetransmissions = registerSignal("packetRetransmissions");
    stats.signals.transmitterThroughput = registerSignal("transmitterThroughput");
    stats.signals.applicationThroughput = registerSignal("applicationThroughput");
    stats.signals.fragmentsCorrectlyTransmitted = registerSignal("fragmentsCorrectlyTransmitted");
    stats.signals.packetsCorrectlyTransmitted = registerSignal("packetsCorrectlyTransmitted");

    // server starts empty
    emit(stats.signals.systemUtilization, 0.0);
}

void Tx::generateNewPacket() {
    simtime_t nextPacketTime = exponential(meanInterTimePkt, 0);
    packetSizeToFragment = uniform(minPacketSize, maxPacketSize, 1);
    scheduleAt(simTime() + nextPacketTime, newPkt);

    EV_INFO << "New packet of " << packetSizeToFragment << "bytes available in " << nextPacketTime << "ms" << endl;
}

void Tx::handleMessage(cMessage* msg) {
    if (msg->isName("packetReady")) {
        enqueueFragments();
        EV_INFO << "fragInTransmission is nullptr " << (fragInTransmission == nullptr) << endl;
        if (fragInTransmission == nullptr) { // no current transmissions
            fragInTransmission = extractFragmentFromQueue();
            EV_INFO << "FragId: " << fragInTransmission->getFragmentId() << " started transmission at " << simTime() << endl;
            sendDirect(fragInTransmission->dup(), 0, fragInTransmission->getChannelTime(), destModule, destGate);

            // server was empty before, but now resumes work
            emit(stats.signals.systemUtilization, 1.0);
        }

        generateNewPacket();
    } else {
        // message from receiver
        if (msg->isName("ACK")) {
            EV_INFO << "FragId: " << fragInTransmission->getFragmentId() << " received ACK at " << simTime() << endl;
            recordSuccessfulTransmission();
            // successful transmission: can delete stored copy
            delete fragInTransmission;
            fragInTransmission = nullptr;

            if (!queue.isEmpty()) {
                fragInTransmission = extractFragmentFromQueue();
                EV_INFO << "FragId: " << fragInTransmission->getFragmentId() << "started transmission at " << simTime() << endl;
                sendDirect(fragInTransmission->dup(), 0, fragInTransmission->getChannelTime(), destModule, destGate);
            } else {
                // server stays empty till next packet is fragmented
                EV_INFO << "Queue currently empty" << endl;
                emit(stats.signals.systemUtilization, 0.0);
            }
        } else if (msg->isName("NACK")) {
            recordFailedTransmission();
            EV_INFO << "FragId: " << fragInTransmission->getFragmentId() << " received NACK at " << simTime() << endl;
            // retransmission
            EV_INFO << "FragId: " << fragInTransmission->getFragmentId() << "started retransmission at " << simTime() << endl;
            sendDirect(fragInTransmission->dup(), 0, fragInTransmission->getChannelTime(), destModule, destGate);
        }

        delete msg; // deleting the ACK/NACK due to no further use
    }
}

void Tx::finish() {
    cancelAndDelete(newPkt);

    simtime_t endingSimTime = simTime();
    double payloadTransmittedBits = static_cast<double>(stats.payloadTransmittedBytes) * 8.0;
    double totalTransmittedBits = static_cast<double>(stats.totalTransmittedBytes) * 8.0;

    double applicationThroughput = payloadTransmittedBits / endingSimTime;
    double transmitterThroughput = totalTransmittedBits / endingSimTime;
    emit(stats.signals.applicationThroughput, applicationThroughput); // bps
    emit(stats.signals.transmitterThroughput, transmitterThroughput); // bps

    emit(stats.signals.fragmentsCorrectlyTransmitted, stats.fragmentsTransmittedCount);
    emit(stats.signals.packetsCorrectlyTransmitted, stats.packetsTransmittedCount);
}

void Tx::enqueueFragments() {
    int fullFragmentTrSize = fragmentSize + headerSize;
    int lastFragmentTrSize = fullFragmentTrSize;

    if (!packetSizeToFragment || (packetSizeToFragment % fragmentSize != 0)){
        // last fragment not full
        // Note: this accounts for packetSizeToFragment < fragmentSize
        lastFragmentTrSize = (packetSizeToFragment % fragmentSize) + headerSize;
        EV_INFO << "Last fragment has dimension " << lastFragmentTrSize - headerSize << ", less than " << fragmentSize << endl;
    }

    // implementation of integer ceil(packetSizeToFragment / fragmentSize)
    // returns 1 if packetSizeToFragment < fragmentSize
    int numFragments = 1;
    if (packetSizeToFragment){
        numFragments = (packetSizeToFragment + fragmentSize - 1) / fragmentSize;
    }

    EV_INFO << "Number of fragments: " << numFragments << endl;

    for (int i = 0; i < numFragments; i++) {
        Fragment* f = new Fragment("readyToSend");
        f->setFragmentId(i);
        if (i == numFragments - 1 && lastFragmentTrSize) {
            f->setDim(lastFragmentTrSize);
            f->setLast(true);
        } else {
            f->setDim(fullFragmentTrSize);
            f->setLast(false);
        }
        // converting size to bits
        f->setChannelTime(static_cast<double>(f->getDim() * 8) / channelBandwidth);
        f->setQueueArrivalTime(simTime());
        queue.insert(f);
    }

    stats.fragmentsInSystemCount += numFragments;
    stats.fragmentsInQueueCount += numFragments;
    emit(stats.signals.fragmentsInSystem, stats.fragmentsInSystemCount);
    emit(stats.signals.fragmentsInQueue, stats.fragmentsInQueueCount);
}

Fragment* Tx::extractFragmentFromQueue() {
    Fragment* extractedFragment = check_and_cast<Fragment*>(queue.pop());
    extractedFragment->setFirstTransmissionTime(simTime());

    stats.fragmentsInQueueCount--;
    emit(stats.signals.fragmentsInQueue, stats.fragmentsInQueueCount);

    simtime_t waitingTime = simTime() - extractedFragment->getQueueArrivalTime();
    emit(stats.signals.fragmentWaitingTime, waitingTime);

    return extractedFragment;
}

void Tx::recordSuccessfulTransmission() {
    // fragment acknowledged is this->fragInTransmission

    stats.totalTransmittedBytes += fragInTransmission->getDim();
    stats.payloadTransmittedBytes += (fragInTransmission->getDim() - headerSize);
    stats.fragmentsTransmittedCount++;

    simtime_t responseTime = simTime() - fragInTransmission->getQueueArrivalTime();
    emit(stats.signals.fragmentResponseTime, responseTime);

    simtime_t serviceTime = simTime() - fragInTransmission->getFirstTransmissionTime();
    emit(stats.signals.fragmentServiceTime, serviceTime);

    emit(stats.signals.fragmentRetransmissions, stats.fragmentRetransmissionsCount);
    stats.fragmentRetransmissionsCount = 0;

    if (fragInTransmission->getLast()) {
        stats.packetsTransmittedCount++;
        emit(stats.signals.packetRetransmissions, stats.packetRetransmissionsCount);
        stats.packetRetransmissionsCount = 0;
    }

    stats.fragmentsInSystemCount--;
    emit(stats.signals.fragmentsInSystem, stats.fragmentsInSystemCount);
}

void Tx::recordFailedTransmission() {
    // fragment acknowledged with NACK is this->fragInTransmission
    stats.fragmentRetransmissionsCount++;
    stats.packetRetransmissionsCount++;
}