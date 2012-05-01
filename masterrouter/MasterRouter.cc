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

#include "MasterRouter.h"

Define_Module(MasterRouter);

void MasterRouter::initialize()
{
}

void MasterRouter::handleMessage(cMessage *msg)
{
	forwardDataPacket((dataPacket *)msg);
}

void MasterRouter::forwardDataPacket(dataPacket * dpkt){
	dpkt->setName("B_INTER_DATA");
	dpkt->setKind(B_INTER_DATA);
	cGate * g = gate("intercell$o", dpkt->getReceiverCell());
	send(dpkt, g);
}
