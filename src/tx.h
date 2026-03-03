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

#ifndef __PROGETTOPECSN_TX_H_
#define __PROGETTOPECSN_TX_H_

#include "Fragment_m.h"
#include <omnetpp.h>

using namespace omnetpp;

class Tx : public cSimpleModule {
  private:
    cModule* destModule;
    int destGate;

    int packetSizeToFragment;

    int headerSize; // H
    int fragmentSize; // P
    double channelBandwidth; // C
    double meanInterTimePkt;
    int minPacketSize;
    int maxPacketSize;

    Fragment* fragInTransmission = nullptr;

    cQueue queue;
    cMessage* newPkt;

    struct Statistics {
        struct {
            simsignal_t fragmentWaitingTime;
            simsignal_t fragmentResponseTime;
            simsignal_t fragmentsInQueue;
            simsignal_t fragmentsInSystem;
            simsignal_t systemUtilization;
            simsignal_t fragmentServiceTime;
            simsignal_t fragmentRetransmissions;
            simsignal_t packetRetransmissions;
            simsignal_t transmitterThroughput;
            simsignal_t applicationThroughput;
            simsignal_t fragmentsCorrectlyTransmitted;
            simsignal_t packetsCorrectlyTransmitted;
        } signals;

        unsigned long totalTransmittedBytes = 0;
        unsigned long payloadTransmittedBytes = 0;

        int fragmentRetransmissionsCount = 0;
        int packetRetransmissionsCount = 0;

        int fragmentsInQueueCount = 0;
        int fragmentsInSystemCount = 0;

        int fragmentsTransmittedCount = 0;
        int packetsTransmittedCount = 0;

        bool serverBusy = false;
    } stats;

    void initializeSignals();
    void generateNewPacket();
    void enqueueFragments();
    void recordSuccessfulTransmission();
    void recordFailedTransmission();
    Fragment* extractFragmentFromQueue();

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage* msg);
    virtual void finish();
};

#endif
