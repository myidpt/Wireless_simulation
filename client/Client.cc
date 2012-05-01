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

#include "Client.h"

Define_Module(Client);

void Client::initialize() {
	totalClient = int(par("totalClient").longValue());
	cellId = (getId() - C_BASE)/(totalClient+CELL_REMAINDER);
	myId = (getId() - C_BASE)%(totalClient+CELL_REMAINDER);

	randgen = new RandGenerator();
	sendQ = new list<dataPacket*>;
	dpktId = myId * 100000;
	osdpktId = -1;

	totalRTQcost = 0; // Control the amount of requests in the queue.
	totalNRTQcost = 0; // Control the amount of requests in the queue.

#ifdef CLIENT_DEBUG
	if(cellId == 0 && myId == 0){
#endif

	generatingPacket = false;

	if(myId < RT_C_NUM)
		scheduleNewDataPacket(true);
	else
		scheduleNewDataPacket(false);
#ifdef CLIENT_DEBUG
	}
#endif

	currentPCHBW = 0;

#ifdef BR_UPDATE_ACTIVE
	cMessage * msg = new cMessage("BR_UPDATE");
	msg->setKind(BR_UPDATE);
	handleBRUpdate(msg);
#endif

	primaryBusy = false;
	cMessage * cmsg2 = new cMessage("TL");
	cmsg2->setKind(TL_START);
	handleLPacket(cmsg2);

//	printf("Client done.%d.%d,%d\n",getId(),cellId,myId);
//	fflush(stdout);

}

void Client::handleMessage(cMessage * msg){

	switch(msg->getKind()){
	case RT_DATA_SCH: // A new data request comes.
	case NRT_DATA_SCH:
		handleNewDataPacket((dataPacket *)msg);
		break;
	case B_DATA:
		handleReceivedDataPacket((dataPacket *)msg);
		break;
	case OPEN_CH: // Notifies that the channel is free.
		handleOpenChannelPacket((ctlPacket *)msg);
		break;
	case TX_RESULT: // Success.
		handleTransmissionResultPacket(msg);
		break;
	case PRIMARY_ON:
		handlePrimayONOFFPacket(true);
		delete msg;
		break;
	case PRIMARY_OFF:
		handlePrimayONOFFPacket(false);
		delete msg;
		break;
	case BR_UPDATE:
		handleBRUpdate(msg);
		break;
	case TL_START:
	case TL_FIN:
		handleLPacket(msg);
		break;
	case UP_TIMEOUT:
		handleTimeoutPacket((dataPacket *)msg);
		break;
	}
}

void Client::handleNewDataPacket(dataPacket * dpkt){
//	if(tl_on) // TL is on!
//		return;
	bool rt = dpkt->getRealtime();
	if((rt && (totalRTQcost + dpkt->getSize() > CLIENT_MAX_RT_Q_COST))
			|| ((!rt) && (totalNRTQcost + dpkt->getSize() > CLIENT_MAX_NRT_Q_COST))){
		delete	dpkt;
	}else{
#ifdef C_DEBUG
	cout << "Cell #" << cellId << " Client #" << myId << ": handleNewDataPacket. sendQ.size=" << sendQ->size() << endl;
#endif
		dpkt->setID(dpktId);
		dpktId ++;
		if(rt)
			totalRTQcost += dpkt->getSize();
		else
			totalNRTQcost += dpkt->getSize();
		sendQ->push_back(dpkt);
		sendNewReqNotification(dpkt);
	}
	generatingPacket = false;
	scheduleNewDataPacket(rt); // Schedule another one
}

void Client::handleReceivedDataPacket(dataPacket * dpkt){
#ifdef C_DEBUG
	cout << "Cell #" << cellId << " Client #" << myId << ": handleReceivedDataPacket#" << dpkt->getID() << endl;
#endif
	if((dpkt->getRealtime() && (SIMTIME_DBL(simTime()) - dpkt->getReceiverBSSendTime() > TIMEOUT_INT_RT)) ||
			(!dpkt->getRealtime() && (SIMTIME_DBL(simTime()) - dpkt->getReceiverBSSendTime() > TIMEOUT_INT_NRT))){ // time out
		if(dpkt->getRealtime() && dpkt->getDeadline() < SIMTIME_DBL(simTime())){
			dpkt->setKind(PRINT_DPKT);
			dpkt->setClientReceiveTime(SIMTIME_DBL(simTime()));
			sendPrintPacket(dpkt);
		}else{
			delete dpkt;
		}
		return;
	}
	dpkt->setClientReceiveTime(SIMTIME_DBL(simTime()));
	sendResultPacket(dpkt->getBCprimaryChannel());
	sendPrintPacket(dpkt); // To the sink. Destroyed there.
}

/*
// The dataPacket is considered as a control packet when it's used to carry the result.
void Client::sendSuccessResultPacket(bool p){
	ctlPacket * cpkt = new ctlPacket();
	cpkt->setName("TX_SUCCESS");
	cpkt->setKind(TX_SUCCESS);
	cpkt->setBitLength(0);
	cpkt->setClientID(myId);
	cpkt->setPrimaryChannel(p);
	sendCtlPacket(cpkt);
}

void Client::sendFailResultPacket(dataPacket * dpkt){
	dpkt->setName("TX_FAIL");
	dpkt->setKind(TX_FAIL);
	dpkt->setBitLength(0);
	sendCtlPacket(dpkt);
}
*/

void Client::handleOpenChannelPacket(ctlPacket * cpkt){ // Set the BW
	// Delete "OPEN_CH" reply.
#ifdef C_DEBUG
	cout << "Cell #" << cellId << " Client #" << myId << ": handleOpenChannelPacket#" << cpkt->getDatapacketID() << endl;
#endif
	setChannelBW(cpkt->getBW(), cpkt->getPrimaryChannel());
	findAndSendDataPacket(cpkt->getDatapacketID(), cpkt->getPrimaryChannel());
	delete cpkt;
}

void Client::handleTransmissionResultPacket(cMessage * pkt){
	osdpktId = -1; // Clear the local osdpkt.
	delete pkt;
	// It must be a good packet. Not timeout-ed if the basestation sends back a result packet.

	/*
	dataPacket * dpkt = (dataPacket *)pkt;
	dpkt->setName("C_DATA_RESEND");
	dpkt->setKind(C_DATA);
	dpkt->setResendCount1(dpkt->getResendCount1()+1);
	dpkt->setBitLength(dpkt->getSize());
	sendDataPacket(dpkt, dpkt->getBCprimaryChannel());
	*/
}

void Client::handleLPacket(cMessage * cmsg){
#ifdef C_DEBUG
	cout << "Cell #" << cellId << " Client #" << myId << ": handleLPacket. ";
#endif
	if(cmsg->getKind() == TL_START){
		// Close primary channels.
		tl_on = true;
		closePrimaryChannel();
		// Schedule next event.
		cmsg->setName("TL_FIN");
		cmsg->setKind(TL_FIN);
#ifdef C_DEBUG
		cout << "close primary channel." << endl;
#endif
		scheduleAt(SIMTIME_DBL(simTime())+TL_INT, cmsg);
	}else{
		// Open primary channels.
		tl_on = false;
#ifdef C_DEBUG
		cout << "open primary channel." << endl;
#endif
		BRUpdate(true);
		// Schedule next event.
		cmsg->setName("TL_START");
		cmsg->setKind(TL_START);
		scheduleAt(SIMTIME_DBL(simTime())+TNL_INT, cmsg);
	}
}

void Client::sendNewReqNotification(dataPacket * dpkt){
#ifdef C_DEBUG
	cout << "Cell #" << cellId << " Client #" << myId << ": sendNewReqNotification#" << dpkt->getID() << endl;
#endif
	ctlPacket * cpkt = new ctlPacket();
	cpkt->setName("C_NEW");
	cpkt->setKind(C_NEW);
	cpkt->setClientID(dpkt->getSenderID());
	cpkt->setDatapacketID(dpkt->getID());
	cpkt->setDelay(dpkt->getDelay());
	cpkt->setSize(dpkt->getBitLength());
	cpkt->setDeadline(dpkt->getDeadline());
	cpkt->setRealtime(dpkt->getRealtime());
	cpkt->setSubmissionTime(dpkt->getSubmissionTime());
	sendCtlPacket(cpkt);
}

void Client::sendResultPacket(bool p){
#ifdef C_DEBUG
	cout << "Cell #" << cellId << " Client #" << myId << ": sendResultPacket." << endl;
#endif
	ctlPacket * cpkt = new ctlPacket();
	cpkt->setName("TX_RESULT");
	cpkt->setKind(TX_RESULT);
	cpkt->setBitLength(0);
	cpkt->setClientID(myId);
	cpkt->setPrimaryChannel(p);
	sendCtlPacket(cpkt);
}

void Client::findAndSendDataPacket(long id, bool p){
#ifdef C_DEBUG
	cout << "Cell #" << cellId << " Client #" << myId << ": findAndSendDataPacket#" << id << endl;
#endif
	if(p == false)
		cout << "ERROR. Client findAndSendDataPacket. Secondary." << endl;
	dataPacket * dpkt = NULL;
	if(sendQ->empty()){
		fprintf(stderr,"ERROR - Client: No more data packet in queue.\n");
	}

	list<dataPacket *>::iterator it;
	for(it = sendQ->begin(); it != sendQ->end(); it ++){
		if((*it)->getID() == id){
			dpkt = *it;
			sendQ->erase(it); // You erase it from the queue when it send it.
			break;
		}
	}

	if(dpkt == NULL){
		fprintf(stderr, "ERROR - Client: Can't find dataPacket #%ld.\n", id);
	}

	if(dpkt->getRealtime())
		totalRTQcost -= dpkt->getSize();
	else
		totalNRTQcost -= dpkt->getSize();
	if(totalRTQcost < 0 || totalNRTQcost < 0){
		fprintf(stderr, "[ERROR]: Client totalQcost < 0.\n");
		fflush(stderr);
	}

	dpkt->setName("C_DATA"); // data packet
	dpkt->setKind(C_DATA);
	dpkt->setClientSendTime(SIMTIME_DBL(simTime()));
	dpkt->setCBprimaryChannel(p);
	osdpktId = dpkt->getID();

	dataPacket * dpkt2 = dpkt->dup(); // timeout packet
	dpkt2->setName("UP_TIMEOUT");
	dpkt2->setKind(UP_TIMEOUT); // One outstanding packet at a time.

	if(dpkt2->getRealtime())
		scheduleAt(SIMTIME_DBL(simTime())+TIMEOUT_INT_RT, dpkt2);
	else
		scheduleAt(SIMTIME_DBL(simTime())+TIMEOUT_INT_NRT, dpkt2);

	// The time-out packet becomes the real packet to send if timeout-ed.

	sendDataPacket(dpkt, p);
}

void Client::scheduleNewDataPacket(bool r){
#ifdef C_DEBUG
	cout << "Cell #" << cellId << " Client #" << myId << ": scheduleNewDataPacket R:" << r << endl;
#endif
	if(generatingPacket)
		return;
	if(SIMTIME_DBL(simTime()) > MAX_SIMTIME)
		return;
	dataPacket * dpkt = new dataPacket();
	if(r){
		dpkt->setName("RT_DATA_SCH");
		dpkt->setKind(RT_DATA_SCH);
		dpkt->setBitLength(RT_SIZE);
		dpkt->setSize(RT_SIZE); // Back-up size record.
		dpkt->setDelay(RT_REQ_DELAY);
		dpkt->setRealtime(true);
		dpkt->setSubmissionTime(SIMTIME_DBL(simTime()) + PT_PKTINT); // Static rate
	}else{
		dpkt->setName("NRT_DATA_SCH");
		dpkt->setKind(NRT_DATA_SCH);
		dpkt->setBitLength(NRT_SIZE);
		dpkt->setSize(NRT_SIZE); // Back-up size record.
		dpkt->setDelay(NRT_REQ_DELAY);
		dpkt->setRealtime(false);
		dpkt->setSubmissionTime(SIMTIME_DBL(simTime()) + randgen->expGen(NRT_EXPLAMDA));
	}
	dpkt->setSenderID(myId);
	dpkt->setSenderCell(cellId);
	dpkt->setReceiverID(int(totalClient * randgen->uniGen()));
	dpkt->setReceiverCell(int(CELL_NUM * randgen->uniGen()));
//	dpkt->setReceiverID(myId);
//	dpkt->setReceiverCell(cellId);
	if(dpkt->getRealtime())
		dpkt->setDeadline(dpkt->getSubmissionTime()+RT_DELAY);
	else
		dpkt->setDeadline(dpkt->getSubmissionTime()+NRT_DELAY);

	if(dpkt->getSubmissionTime() < 0 || dpkt->getSubmissionTime() > 10000000){
		fprintf(stderr, "[ERROR]: dpkt->getSubmissionTime = %lf.\n", dpkt->getSubmissionTime());
		fflush(stderr);
	}
	scheduleAt(dpkt->getSubmissionTime(), dpkt); // Schedule self message
	generatingPacket = true;
}

void Client::setChannelBW(long bw, bool p){
	if(p)
		currentPCHBW = bw;
	else
		cout << "ERROR Client. setChannelBW." << endl;
	BRUpdate(p);
}

void Client::handleBRUpdate(cMessage * cmsg){
	fprintf(stderr, "[ERROR]: should not have handleBRUpdate.\n");
	fflush(stderr);

	BRUpdate(true);
	scheduleAt(SIMTIME_DBL(simTime()) + BR_UPDATE_INT, cmsg);
}

void Client::BRUpdate(bool p){
#ifdef C_DEBUG
	cout << "Cell #" << cellId << " Client #" << myId << ": BRUpdate." << endl;
#endif
//	if(tl_on && p) // tl is on, you can't update p.
//		return;
	cGate * g;
	if(p)
		g = gate("pdataout");
	else
		cout << "ERROR. Client BRUpdate. Secondary." << endl;
	cDatarateChannel * ch = (cDatarateChannel *)(g->getTransmissionChannel());
	double br;
	if(p){
		if(currentPCHBW == 0){
			br = 0;
			return;
		}
		if(primaryBusy){
			br = currentPCHBW * log2(1+pow(randgen->rayGen(RAY_ROW),2) * BW_BR_S
				/ BW_BR_N0_P / currentPCHBW);
		}else{
			br = currentPCHBW * log2(1+pow(randgen->rayGen(RAY_ROW),2) * BW_BR_S
				/ BW_BR_N0 / currentPCHBW);
		}
	}else{
		cout << "ERROR. Client. BRUpdate. Secondary." << endl;
	}
//	printf("Client: set Channel bw = %ld, br = %ld.\n", currentPCHBW, (long)br);
	ch->setDatarate(br); // Set the data rate of this channel.
}

void Client::handlePrimayONOFFPacket(bool on){
#ifdef C_DEBUG
	cout << "Cell #" << cellId << " Client #" << myId << ": handlePrimayONOFFPacket. ON=" << on << endl;
#endif
	primaryBusy = on;
	BRUpdate(true);
}

void Client::closePrimaryChannel(){
	cGate * g = gate("pdataout");
	cDatarateChannel * ch = (cDatarateChannel *)(g->getTransmissionChannel());
	ch->setDatarate(1);
}

void Client::sendCtlPacket(cMessage * pkt){
	cChannel * cch = gate("control$o")->getTransmissionChannel();
	if(cch->isBusy()){
		sendDelayed(pkt, cch->getTransmissionFinishTime() - simTime(), "control$o");
	}
	else
		send(pkt, "control$o");
}

void Client::sendDataPacket(dataPacket * dpkt, bool p){
	if(p)
		send(dpkt, "pdataout");
	else
		cout << "ERROR. Client sendDataPacket. Secondary." << endl;
}

void Client::sendPrintPacket(cMessage * cmsg){
	cmsg->setName("PRINT_DPKT");
	cmsg->setKind(PRINT_DPKT);
	send(cmsg, "sink");
}

// From myself to others.
void Client::handleTimeoutPacket(dataPacket * dpkt){
	// timeout packet is back.
	if(osdpktId == dpkt->getID()){
		forceFreeChannel(dpkt->getCBprimaryChannel());

		if(!dpkt->getRealtime() || (dpkt->getRealtime() && dpkt->getDelay() > (SIMTIME_DBL(simTime())-dpkt->getSubmissionTime()))){ // Still good.
			// Time out detected, but the channel may still be busy.
			// Send C_NEW to re-schedule a slot from the basestation.
			dpkt->setResendCount1(dpkt->getResendCount1()+1);

			if(dpkt->getRealtime())
				totalRTQcost += dpkt->getSize();
			else
				totalNRTQcost += dpkt->getSize();

			sendQ->push_back(dpkt); // Re-insert it back to queue., if not real time.
//			sendNewReqNotification(dpkt);
		}else{ // Consider as sent.
			osdpktId = -1;
		}
	}else{
		delete dpkt;
	}
}

void Client::forceFreeChannel(bool p){
	cDatarateChannel * ch;
	if(p)
		ch = (cDatarateChannel *)gate("pdataout")->getTransmissionChannel();
	else
		ch = (cDatarateChannel *)gate("sdataout")->getTransmissionChannel();
	ch->forceTransmissionFinishTime(simTime());
}

void Client::finish(){
	free(randgen);
	free(sendQ);
}

Client::~Client() {
}
