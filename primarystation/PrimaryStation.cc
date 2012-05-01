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

#include "PrimaryStation.h"

Define_Module(PrimaryStation);

void PrimaryStation::initialize()
{
	myId = par("Id").longValue();

	randGen = new RandGenerator();
	primaryBusy = true;

	CELL_LOCATION;


#ifdef PS_ACTIVE
	sendONOFFPacket();

	cMessage * cmsg = new cMessage("PS_SCH");
	cmsg->setKind(PS_SCH);
	scheduleNextEvent(cmsg);
#endif

}

void PrimaryStation::handleMessage(cMessage *msg)
{
	switch(msg->getKind()){
	case PS_SCH:
		sendONOFFPacket();
		scheduleNextEvent(msg);
		break;
	default:
		fprintf(stderr,"ERROR:PrimaryStation. Unrecoganizable packet kind - %d\n",
				msg->getKind());
	}
}

void PrimaryStation::sendONOFFPacket(){
	psPacket * pspkt;
	if(primaryBusy == false)
		primaryBusy = true;
	else
		primaryBusy = false;
	for(int i = 0; (i < CELL_NUM) && (cellLoc[myId][i] >= 0); i ++){
//		printf("%d,%d->%d\n", myId, i, cellLoc[myId][i]);
//		fflush(stdout);
		if(primaryBusy == true){
			pspkt = new psPacket("PRIMARY_ON");
			pspkt->setPs(myId);
			pspkt->setKind(PRIMARY_ON);
		}else{
			pspkt = new psPacket("PRIMARY_OFF");
			pspkt->setPs(myId);
			pspkt->setKind(PRIMARY_OFF);
		}
		send(pspkt, "cell$o", i);
	}
}

void PrimaryStation::scheduleNextEvent(cMessage * cmsg){
	if(primaryBusy)
		scheduleAt(SIMTIME_DBL(simTime()) + randGen->expGen(PS_ON_EXPLAMDA), cmsg); // Schedule self message
	else
		scheduleAt(SIMTIME_DBL(simTime()) + randGen->expGen(PS_OFF_EXPLAMDA), cmsg);
}
