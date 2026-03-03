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

package progettopecsn;

simple Tx
{
  	parameters:
   		int dimHeader @unit(B);
   		int dimFragment @unit(B);
   		double bandwidthChannel @unit(bps);
   		
   		double meanInterTimePkt @unit(s);
        int minSizePkt @unit(B);
        int maxSizePkt @unit(B);

        // statistics
        @signal[fragmentWaitingTime](type=simtime_t);
        @signal[fragmentResponseTime](type=simtime_t);
        @signal[fragmentsInQueue](type=long);
        @signal[fragmentsInSystem](type=long);
        @signal[systemUtilization](type=double);
        @signal[fragmentServiceTime](type=simtime_t);
        @signal[fragmentRetransmissions](type=long);
        @signal[packetRetransmissions](type=long);
        @signal[transmitterThroughput](type=double);
        @signal[applicationThroughput](type=double);
        @signal[fragmentsCorrectlyTransmitted](type=long);
        @signal[packetsCorrectlyTransmitted](type=long);

        @statistic[fragmentWaitingTime](source=fragmentWaitingTime;record=mean,max);
        @statistic[fragmentResponseTime](source=fragmentResponseTime;record=mean,max,vector);
        @statistic[fragmentsInQueue](source=fragmentsInQueue;record=timeavg,max);
        @statistic[fragmentsInSystem](source=fragmentsInSystem;record=timeavg,max,vector);
        @statistic[systemUtilization](source=systemUtilization;record=timeavg);
        @statistic[fragmentServiceTime](source=fragmentServiceTime;record=mean,max);
        @statistic[fragmentRetransmissions](source=fragmentRetransmissions;record=mean,max,sum,vector);
        @statistic[packetRetransmissions](source=packetRetransmissions;record=mean,max,sum,vector);
		@statistic[transmitterThroughput](source=transmitterThroughput;record=last);
		@statistic[applicationThroughput](source=applicationThroughput;record=last);
		@statistic[fragmentsCorrectlyTrasmitted](source=fragmentsCorrectlyTransmitted;record=last);
		@statistic[packetsCorrectlyTransmitted](source=packetsCorrectlyTransmitted;record=last);

    gates:
        input wirelessInTx @directIn;
}
