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

#include "rx.h"
#include "Fragment_m.h"
#include <cmath>

Define_Module(Rx);

void Rx::initialize() {
    destModuleTx = getParentModule()->getSubmodule("tx");
    destGateTx = destModuleTx->gate("wirelessInTx")->getId();

    dimAck = par("dimAck").intValue();
    channelBandwidth = par("bandwidthChannel").doubleValue();
    alphaError = par("alpha").doubleValue();
    // converting to bits
    acknowledgmentTrTime = static_cast<double>(dimAck * 8) / channelBandwidth;

    endToEndPktDelay = registerSignal("endToEndPktDelay");
}

void Rx::handleMessage(cMessage* msg) {
    f = check_and_cast<Fragment*>(msg);
    int transmittedFragSize = f->getDim();

    if (isFragmentCorrupted(transmittedFragSize)) {
        EV_INFO << "Received Fragment " << f->getFragmentId() << " corrupted. Sending NACK at " << simTime() << endl;
        sendAcknowledgment(/* isAck */ false);
    } else {
        EV_INFO << "Received Fragment " << f->getFragmentId() << " not corrupted. Sending ACK at " << simTime() << endl;
        if (f->getLast()) {
            simtime_t endToEndDelay = simTime() - f->getQueueArrivalTime();
            EV_INFO << "Packet reassembled at " << simTime() << endl;
            emit(endToEndPktDelay, endToEndDelay);
        }

        sendAcknowledgment(/* isAck */ true);
    }

    delete msg;
}

bool Rx::isFragmentCorrupted(int transmittedFragSize) {
    double T = uniform(0, 1);
    long double p = 1 - pow(1 - alphaError, static_cast<double>(transmittedFragSize));

    return T <= p;
}

void Rx::sendAcknowledgment(bool isAck) {
    const char* ackMsg = isAck ? "ACK" : "NACK";
    cPacket* acknowledgement = new cPacket(ackMsg);
    sendDirect(acknowledgement, 0, acknowledgmentTrTime, destModuleTx, destGateTx);
}
