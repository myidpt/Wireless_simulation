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

#include "BaseStation.h"

Define_Module(BaseStation);

void BaseStation::initialize() {
	totalClient = int(par("totalClient").longValue());
	myCell = (getId()-C_BASE)/(totalClient+CELL_REMAINDER);

	localreceiveQ = new list<ctlPacket *>;
	localsendQ = new list<dataPacket *>;
	oslocalreceiveQ = new list<ctlPacket *>;
	oslocalsendQ = new list<dataPacket *>;

	totalPBW = PRIMARY_BW;
	freePBW = totalPBW;

	retx_count = 0;

	for(int i = 0; i < MAX_C_INCELL; i ++){
		localreceivebwAlloc[i] = -1;
		localsendbwAlloc[i] = -1;
	}
	randgen = new RandGenerator();

	// Construct interfPS. -1 : No interference with that PS, otherwise, 0: PS off, 1: PS on.
	CELL_LOCATION;
	for(int i = 0; i < PS_NUM; i ++){
		PS[i] = -1;
		for(int j = 0; cellLoc[i][j] != -1 && j < CELL_NUM; j ++){
			if(cellLoc[i][j] == myCell)
				PS[i] = 0; // Initialially off.
		}
	}

	primaryBusy = false;
	primaryBusy_D = false;

	char outfname2[200] = BW_STAT_PATH_PREFIX;
	int len = strlen(outfname2);
	outfname2[len] = myCell/10 + '0';
	outfname2[len+1] = myCell%10 + '0';
	outfname2[len+2] = '\0';
	if( (outfp2 = fopen(outfname2, "w+")) == NULL){
		fprintf(stderr, "ERROR BaseStation: Trace file open failure: %s\n", outfname2);
		deleteModule();
	}
	pRTWorkload = 0;
	pNRTWorkload = 0;
	lastPRTBW = 0;
	lastPNRTBW = 0;
	lastPRTBWtime = 0;
	lastPNRTBWtime = 0;

#ifdef TL_DEBUG
	if(myCell == 0){
#endif
	cMessage * cmsg2 = new cMessage("TL");
	cmsg2->setKind(TL_START);
	handleLPacket(cmsg2);
#ifdef TL_DEBUG
	}
#endif

}

void BaseStation::handleMessage(cMessage *cmsg){
	switch(cmsg->getKind()){
	case C_DATA:
		handleDataPacket((dataPacket *)cmsg);
		break;
	case TX_RESULT:
		handleTransmissionResultPacket(cmsg);
		break;
	case B_INTER_DATA:
		handleInterBSDataPacket((dataPacket *)cmsg);
		break;
	case C_NEW:
//		printf("BaseStation: c_new = %lf  id = %ld.\n", SIMTIME_DBL(simTime()), ((ctlPacket *)cmsg)->getDatapacketID());
//		fflush(stdout);
		handleNewReqNotification((ctlPacket *)cmsg);
		break;
	case PRIMARY_ON:
	case PRIMARY_OFF:
		handlePrimaryONOFFPacket((psPacket *)cmsg);
		delete cmsg;
		break;
	case BR_UPDATE:
		fprintf(stderr, "There should not be BR_UPDATE messages.\n");
		handleBRUpdate(cmsg);
		break;
	case TL_START:
	case TL_FIN:
		handleLPacket(cmsg);
		break;
	case DOWN_TIMEOUT: // From yourself
		handleDownTimeoutPacket((dataPacket *)cmsg);
		break;
	case UP_TIMEOUT: // From yourself
//		printf("BaseStation: timeout = %lf.\n", SIMTIME_DBL(simTime()));
//		fflush(stdout);
		handleUpTimeoutPacket((ctlPacket *)cmsg);
		break;
	default:
		fprintf(stderr, "[ERROR] BaseStation: unknow message kind = %d.\n", cmsg->getKind());
		fflush(stderr);
	}
}

// cpkt is received from tryToAllocAndReceive. If successful, send OPEN_CHANNEL reply to the client.
// If successful, return true, if not, return false.
bool BaseStation::scheduleReceiveReqToPCh(ctlPacket * cpkt){
	if( (freePBW < UP_BW_UNIT * MAX_UP_UNITS) || tl_on || primaryBusy_D){
		return false;
	}

	long bwtoAlloc = UP_BW_UNIT * MAX_UP_UNITS; // Always allocate the maximum bandwidth to shorten the transmission.

	double timeallocfortx = cpkt->getSize() * EstBit_TXTIME / MAX_UP_UNITS; // The time allocated for the transmission

	// Calculate if there's interference with TL period. If so, return false.
	// Can not be avoided because you are already using the biggest bandwidth.
	int n = SIMTIME_DBL(simTime()) / (TL_INT+TNL_INT);
	double tl_on_t = (TL_INT+TNL_INT) * (n + 1); // The time next tl going to be on.
	if(tl_on_t < SIMTIME_DBL(simTime()) + timeallocfortx){ // Test if you can finish inside TNL.
		return false;
	}

	// If you reach here, you can send it via primary channel.
	int id = cpkt->getClientID();
	ctlPacket * reply = new ctlPacket("OPEN_CH", OPEN_CH);
	reply->setClientID(cpkt->getClientID());
	reply->setDatapacketID(cpkt->getDatapacketID());
	reply->setBW(bwtoAlloc);
	reply->setPrimaryChannel(true);
	sendCtlPacket(reply, id);

//	printf("Send OPEN_CH to %d - %ld.\n", reply->getClientID(), reply->getDatapacketID());
//	fflush(stdout);

	cpkt->setPrimaryChannel(true);
#ifdef BS_DEBUG
	cout << "BS#" << myCell << "ID=" << id << " localreceivebwAlloc[id]=" << localreceivebwAlloc[id] << " bwtoAlloc=" << bwtoAlloc << endl;
#endif
	localreceivebwAlloc[id] = bwtoAlloc;
	freePBW -= bwtoAlloc;

	analyzeChUse(true, cpkt->getRealtime(), bwtoAlloc, true);

#ifdef BW_DEBUG
	ctlPacket * ppkt = new ctlPacket("PRINT_CH_USE", PRINT_CH_UP_USE);
	ppkt->setPrimaryChannel(true);
	ppkt->setRealtime(cpkt->getRealtime());
	ppkt->setBW(totalPBW - freePBW);
	sendPrintInfoPacket(ppkt);
#endif

	cpkt->setName("UP_TIMEOUT");
	cpkt->setKind(UP_TIMEOUT);
	if(cpkt->getRealtime()) // Schedule self timeout message for the client -> server transmission.
		scheduleAt(SIMTIME_DBL(simTime())+TIMEOUT_INT_RT + 0.000001, cpkt);
	else
		scheduleAt(SIMTIME_DBL(simTime())+TIMEOUT_INT_NRT + 0.000001, cpkt);

	return true;
}

bool BaseStation::scheduleSendReqToPCh(dataPacket * dpkt){
	if( (freePBW < DOWN_BW_UNIT * MAX_DOWN_UNITS) || tl_on || primaryBusy_D){
		return false;
	}
	double timeallocfortx;
	// The time allocated for the transmission

	long bwtoAlloc = DOWN_BW_UNIT * MAX_DOWN_UNITS; // Always allocate the maximum bandwidth to shorten the transmission.
	if(freePBW < bwtoAlloc)
		bwtoAlloc = freePBW;

	timeallocfortx = dpkt->getSize() * EstBit_TXTIME / MAX_DOWN_UNITS;

	// Calculate if there's interference with TL period. If so, return false.
	// Can not be avoided because you are already using the biggest bandwidth.
	int n = SIMTIME_DBL(simTime()) / (TL_INT+TNL_INT);
	double tl_on_t = (TL_INT+TNL_INT) * (n + 1); // The time next tl going to be on.
	if(tl_on_t < SIMTIME_DBL(simTime()) + timeallocfortx){ // Test if you can finish inside TNL.
		return false;
	}

	// If you reach here, you can send it via primary channel.
	int id = dpkt->getReceiverID();

	dpkt->setBCprimaryChannel(true);
	localsendbwAlloc[id] = bwtoAlloc;
	freePBW -= localsendbwAlloc[id];

	// Have a copy sent, keep the original one.
	dataPacket * dpkt2 = dpkt->dup();

	dpkt->setName("DOWN_TIMEOUT");
	dpkt->setKind(DOWN_TIMEOUT);

	if(dpkt->getRealtime())
		scheduleAt(SIMTIME_DBL(simTime())+TIMEOUT_INT_RT, dpkt);
	else
		scheduleAt(SIMTIME_DBL(simTime())+TIMEOUT_INT_NRT, dpkt);

	analyzeChUse(true, dpkt->getRealtime(), bwtoAlloc, true);

#ifdef BW_DEBUG
	ctlPacket * ppkt = new ctlPacket("PRINT_CH_USE", PRINT_CH_DOWN_USE);
	ppkt->setPrimaryChannel(true);
	ppkt->setRealtime(dpkt->getRealtime());
	ppkt->setBW(totalPBW - freePBW);
	sendPrintInfoPacket(ppkt);
#endif

	setChannelBRfromBW(id, bwtoAlloc, true);
	dpkt2->setName("B_DATA");
	dpkt2->setKind(B_DATA);
	sendDataPacket(dpkt2, id, true);

	return true;
}

// Try to allocate the receive bw to receive more packets.
// Algorithm goes here.
void BaseStation::tryToAllocAndReceive(){
	// cpkt from waitingQ -> osQ.
	// New "OPEN_CH" reply.
	if(tl_on || primaryBusy_D)
		return;
	if(localreceiveQ->empty()){
		return;
	}

//	printf("BaseStation: tryToAllocAndReceive = %lf.\n", SIMTIME_DBL(simTime()));
//	fflush(stdout);

//	printf("localreceiveQ = %d.\n", localreceiveQ->size());
//	fflush(stdout);

	bool success = false;
	list<ctlPacket *>::iterator it;

#ifndef PRIORITY_QUEUE
	for(it = localreceiveQ->begin(); it != localreceiveQ->end(); it ++){
		if((freePBW < UP_BW_UNIT * MAX_UP_UNITS) || tl_on || primaryBusy_D){
			break;
		}

		if(localreceivebwAlloc[(*it)->getClientID()] > 0)
			continue;

		success = false;
		if((*it)->getRealtime() && freeSBW >= UP_BW_UNIT){ // RT packet && have free bw in S.
			if(scheduleReceiveReqToSCh(*it)){
				success = true;
			}
		}
		else if(!(*it)->getRealtime()){ // NRT packet && can send in P.
			if(scheduleReceiveReqToPCh(*it)){
				success = true;
			}
		}
		if(success){
			oslocalreceiveQ->push_back(*it); // Add to the os queue.
			localreceiveQ->erase(it); // remove from the waiting queue
			it = localreceiveQ->begin(); // start again.

			if(localreceiveQ->empty())
				break;
		}
	}
#endif

	for(it = localreceiveQ->begin(); it != localreceiveQ->end(); it ++){
		if(localreceivebwAlloc[(*it)->getClientID()] > 0)
			continue;

		success = false;
		if(scheduleReceiveReqToPCh(*it)){
			success = true;
		}
		if(success){
			oslocalreceiveQ->push_back(*it); // Add to the os queue.
			localreceiveQ->erase(it); // remove from the waiting queue
			it = localreceiveQ->begin(); // start again.
			if(localreceiveQ->empty())
				break;
		}
	}
}

// Try to allocate the receive bw to send more packets.
// Scheduler is here.
void BaseStation::tryToAllocAndSend(){
	if(tl_on || primaryBusy_D)
		return;
	if(localsendQ->empty())
		return;

//	printf("localsendQ = %d.\n", localreceiveQ->size());
//	fflush(stdout);

	bool success = false;
	list<dataPacket *>::iterator it;

#ifndef PRIORITY_QUEUE
	for(it = localsendQ->begin(); it != localsendQ->end(); it ++){
		if((freePBW < UP_BW_UNIT * MAX_UP_UNITS) || tl_on || primaryBusy_D){
			break;
		}

		if(localsendbwAlloc[(*it)->getReceiverID()] > 0)
			continue;

		success = false;
		if((*it)->getRealtime() && freeSBW >= DOWN_BW_UNIT){ // RT packet && have free bw in S.
			if(scheduleSendReqToSCh(*it)){
				success = true;
			}
		}
		else if(!(*it)->getRealtime()){ // NRT packet && can send in P.
			if(scheduleSendReqToPCh(*it)){
				success = true;
			}
		}
		if(success){
			oslocalsendQ->push_back(*it); // Add to the os queue.
			localsendQ->erase(it); // remove from the waiting queue
			it = localsendQ->begin(); // start again.
			if(localsendQ->empty())
				break;
		}
	}
#endif

	for(it = localsendQ->begin(); it != localsendQ->end(); it ++){
		if(localsendbwAlloc[(*it)->getReceiverID()] > 0)
			continue;

		success = false;
		if(scheduleSendReqToPCh(*it)){
			success = true;
		}
		if(success){
			oslocalsendQ->push_back(*it); // Add to the os queue.
			localsendQ->erase(it); // remove from the waiting queue
			it = localsendQ->begin(); // start again.
			if(localsendQ->empty())
				break;
		}
	}
}

void BaseStation::enLocalsendQ(dataPacket * dpkt){ // BS2
#ifdef BS_DEBUG
	cout << "BS#" << myCell << " enLocalsendQ#" << dpkt->getID() << endl;
#endif
	list<dataPacket*>::iterator it;
//	double itdl, dpktdl;
	for(it = localsendQ->begin(); it != localsendQ->end(); it ++){
		// Algorithm
#ifndef DEADLINE_ON
		if((*it)->getDeadline() - (*it)->getSize() * EstBit_TXTIME - (SIMTIME_DBL(simTime()))
		  > (dpkt->getDeadline() - dpkt->getSize() * EstBit_TXTIME - (SIMTIME_DBL(simTime()))))
#endif
#ifdef DEADLINE_ON
		if((*it)->getRealtime()){
			itdl = (*it)->getDeadline() - RT_TAG_SHIFT;
		}
		if(dpkt->getRealtime()){
			dpktdl = dpkt->getDeadline() - RT_TAG_SHIFT;
		}
		if(itdl > dpktdl)
#endif
			break;
	}
	localsendQ->insert(it,dpkt);

//	for(it = localsendQ->begin(); it != localsendQ->end(); it ++){
//		cout << ((*it)->getDeadline() - (*it)->getSize() * EstBit_TXTIME - (SIMTIME_DBL(simTime()))) << " = ";
//		cout << (*it)->getRealtime() << " ";
//	}
//	cout << endl;
}

void BaseStation::enLocalreceiveQ(ctlPacket * cpkt){ // BS1
#ifdef BS_DEBUG
	cout << "BS#" << myCell << " enLocalreceiveQ#" << cpkt->getDatapacketID() << endl;
#endif
	list<ctlPacket*>::iterator it;
	for(it = localreceiveQ->begin(); it != localreceiveQ->end(); it ++){ // Priority queue
		// Algorithm
		if(((*it)->getDeadline() - (*it)->getSize() * EstBit_TXTIME * 2 - SIMTIME_DBL(simTime()))
		  > (cpkt->getDeadline() - cpkt->getSize() * EstBit_TXTIME * 2 - SIMTIME_DBL(simTime())))
			break;
	}
	localreceiveQ->insert(it,cpkt);
}

// This one cancels the timeout event.
bool BaseStation::deallocReceiveBW(int id, bool del){
#ifdef BS_DEBUG
	cout << "BS#" << myCell << " deallocReceiveBW." << endl;
#endif
	bool realtime;
	list<ctlPacket *>::iterator it;
	for(it = oslocalreceiveQ->begin(); it != oslocalreceiveQ->end(); it ++){
		if((*it)->getClientID() == id){
			realtime = (*it)->getRealtime();
			oslocalreceiveQ->erase(it);
			if(del)
				cancelAndDelete(*it); // Cancel event and delete the duplicate one.
			else
				cancelEvent(*it);
			break;
		}
	}
	if(it == oslocalreceiveQ->end()){
		cout << "ERROR - BaseStation #" << myCell <<
				" deallocReceiveBW: can't find packet from client#" << id << " from os receive queue.\n";
		fflush(stdout);
		fflush(stderr);
	}

	if(localreceivebwAlloc[id] > 0){
		freePBW += localreceivebwAlloc[id];

		analyzeChUse(true, realtime, localreceivebwAlloc[id], false);
#ifdef BW_DEBUG
			ctlPacket * ppkt = new ctlPacket("PRINT_CH_USE", PRINT_CH_UP_USE);
			ppkt->setPrimaryChannel(true);
			ppkt->setRealtime(realtime);
			ppkt->setBW(totalPBW - freePBW);
			sendPrintInfoPacket(ppkt);
#endif
		localreceivebwAlloc[id] = -1;
		return true;
	}
	return false;
}

// This one cancels the timeout event.
bool BaseStation::deallocSendBW(int id, bool del){ // del is to inform if the request need to be destroyed.
#ifdef BS_DEBUG
	cout << "BS#" << myCell << " deallocReceiveBW." << endl;
#endif
	bool realtime;
	list<dataPacket *>::iterator it;
	for(it = oslocalsendQ->begin(); it != oslocalsendQ->end(); it ++){
		if((*it)->getReceiverID() == id){
			realtime = (*it)->getRealtime();
			oslocalsendQ->erase(it);
			if(del)
				cancelAndDelete(*it); // Cancel event and delete the duplicate one.
			else
				cancelEvent(*it);
			break;
		}
	}
	if(it == oslocalsendQ->end()){
		fprintf(stderr,"ERROR - BaseStation #%d deallocSendBW: can't find packet from client #%d from os send queue.\n", myCell, id);
		fflush(stdout);
	}

	if(localsendbwAlloc[id] > 0){
		freePBW += localsendbwAlloc[id];

		analyzeChUse(true, realtime, localsendbwAlloc[id], false);
#ifdef BW_DEBUG
			ctlPacket * ppkt = new ctlPacket("PRINT_CH_USE", PRINT_CH_DOWN_USE);
			ppkt->setPrimaryChannel(true);
			ppkt->setRealtime(realtime);
			ppkt->setBW(totalPBW - freePBW);
			sendPrintInfoPacket(ppkt);
#endif
		localsendbwAlloc[id] = -1;

		return true;
	}
	return false;
}


// From local to local / global
void BaseStation::handleDataPacket(dataPacket * dpkt){
#ifdef BS_DEBUG
	cout << "BS#" << myCell << " handleDataPacket#" << dpkt->getID() << endl;
#endif
	// dpkt -> localsendQ.
	if((dpkt->getRealtime() && (SIMTIME_DBL(simTime()) - dpkt->getClientSendTime() > TIMEOUT_INT_RT)) ||
			(!dpkt->getRealtime() && (SIMTIME_DBL(simTime()) - dpkt->getClientSendTime() > TIMEOUT_INT_NRT))){ // Judge if it is a timeout-ed packet.
		if(dpkt->getRealtime() && dpkt->getDelay() > (SIMTIME_DBL(simTime())-dpkt->getSubmissionTime())){
			// Delete the corresponding packet record.
//			deallocReceiveBW(dpkt->getSenderID(), true);
			dpkt->setKind(PRINT_DPKT);
			dpkt->setClientReceiveTime(SIMTIME_DBL(simTime()));
			sendPrintInfoPacket(dpkt);
		} else {// Delete it in the time out message
			delete dpkt;
		}
		return;
	}

	// Good packet. Send result
	dpkt->setSenderBSReceiveTime(SIMTIME_DBL(simTime()));
	sendTransmissionResultPacket(dpkt);

	// Timeout event will be canceled in deallocaReceiveBW.
	if(deallocReceiveBW(dpkt->getSenderID(), true)){
		tryToAllocAndSend(); // Send has higher priorityhandleNewDataPacket
		tryToAllocAndReceive(); // bw deallocation success.
	}
	if(dpkt->getCBprimaryChannel() && !primaryBusy_D)
		retx_count = 0;

	if(dpkt->getReceiverCell() == myCell){ // Receiver is local
		dpkt->setReceiverBSReceiveTime(SIMTIME_DBL(simTime()));
		enLocalsendQ(dpkt);
		tryToAllocAndSend();
	}
	else{ // Send to other cells.
		dpkt->setName("B_INTER_DATA");
		dpkt->setKind(B_INTER_DATA);
		dpkt->setSenderBSSendTime(SIMTIME_DBL(simTime()));
		sendInterBSDataPacket(dpkt);
	}
}

// From myself to others.
void BaseStation::handleDownTimeoutPacket(dataPacket * dpkt){
#ifdef BS_DEBUG
	cout << "BS#" << myCell << " handleDownTimeoutPacket#" << dpkt->getID() << endl;
#endif

	if(dpkt->getBCprimaryChannel() && !primaryBusy_D){
		retx_count ++;
		if(retx_count > RETX_LIMIT){
			primaryBusy_D = true;
			cMessage * pmsg;
			pmsg = new cMessage("PRINT_PD_ON", PRINT_PD_ON);
			sendPrintInfoPacket(pmsg);
		}
	}

	freeChannel(dpkt->getReceiverID(), dpkt->getBCprimaryChannel());
	if(dpkt->getRealtime() && (dpkt->getDelay() < SIMTIME_DBL(simTime())-dpkt->getSubmissionTime())){ // Missed deadline.
		if(deallocSendBW(dpkt->getReceiverID(), true)){
			tryToAllocAndSend();
			tryToAllocAndReceive();
		}
	}else{
		dpkt->setResendCount2(dpkt->getResendCount2()+1);
		if(deallocSendBW(dpkt->getReceiverID(), false)){
			enLocalsendQ(dpkt);
			tryToAllocAndSend();
			tryToAllocAndReceive();
		}
	}
}

// From clients to myself.
void BaseStation::handleUpTimeoutPacket(ctlPacket * cpkt){
#ifdef BS_DEBUG
	cout << "BS#" << myCell << " handleUpTimeoutPacket#" << cpkt->getDatapacketID() << endl;
#endif
	if(cpkt->getPrimaryChannel() && !primaryBusy_D){
		retx_count ++;
		if(retx_count > RETX_LIMIT){
			primaryBusy_D = true;
			cMessage * pmsg;
			pmsg = new cMessage("PRINT_PD_ON", PRINT_PD_ON);
			sendPrintInfoPacket(pmsg);
		}
	}

	if(cpkt->getRealtime() && cpkt->getDelay() < (SIMTIME_DBL(simTime())-cpkt->getSubmissionTime())){ // Missed deadline.
		if(deallocReceiveBW(cpkt->getClientID(), true)){
			tryToAllocAndSend();
			tryToAllocAndReceive();
		}
	}else{
		if(deallocReceiveBW(cpkt->getClientID(), false)){
			enLocalreceiveQ(cpkt);
			tryToAllocAndSend();
			tryToAllocAndReceive();
		}
	}
}

// Global to local, (global to global is impossible.
void BaseStation::handleInterBSDataPacket(dataPacket * dpkt){
#ifdef BS_DEBUG
	cout << "BS#" << myCell << " handleInterBSDataPacket#" << dpkt->getID() << endl;
#endif
	if(dpkt->getReceiverCell() == myCell){ // Receiver is local
		dpkt->setReceiverBSReceiveTime(SIMTIME_DBL(simTime()));
		enLocalsendQ(dpkt);
		tryToAllocAndSend();
	}else{ // Send to other cells.
		fprintf(stderr,"ERROR: BaseStation. Got an interBS data packet not pointing to myself.\n");
	}
}
/*
void BaseStation::handleCancelPacket(ctlPacket * cpkt){
	deallocReceiveBW(cpkt->getDatapacketID());
	delete cpkt;
}
*/
// Receive a boolean: primary. If update secondary, false. If update primary, true.
void BaseStation::BRUpdate(bool p){
	if(tl_on && p)
		return;
	double br;
	for(int i = 0; i < MAX_C_INCELL; i ++){
		if(localsendbwAlloc[i] > 0){
			if(p){ // Primary.
				if(primaryBusy){
					br = localsendbwAlloc[i] *
						log2(1 + pow(randgen->rayGen(RAY_ROW),2) * BW_BR_S
						/ BW_BR_N0_P / localsendbwAlloc[i]);
				}else{
					br = localsendbwAlloc[i] *
						log2(1 + pow((randgen->rayGen(RAY_ROW)),2) * BW_BR_S
						/ BW_BR_N0 / localsendbwAlloc[i]);
				}
				setChannelBR(i, br, true);
			}
			else if(!p){ // Secondary.
				cout << "ERROR. BRUpdate. Secondary." << endl;
			}
		}
	}
}

void BaseStation::handleBRUpdate(cMessage * cmsg){
//	BRUpdate(false);
	if(!tl_on){
		BRUpdate(true);// If in tl_on, only update secondary.
	}
	scheduleAt(SIMTIME_DBL(simTime())+BR_UPDATE_INT,cmsg);
}

void BaseStation::handleLPacket(cMessage * cmsg){
#ifdef BS_DEBUG
	cout << "BS#" << myCell << " handleLPacket." << endl;
#endif

	if(cmsg->getKind() == TL_START){
		// Close primary channels.
		closePrimaryChannels();
		tl_on = true;

		// Schedule next event.
		cmsg->setName("TL_FIN");
		cmsg->setKind(TL_FIN);
		scheduleAt(SIMTIME_DBL(simTime())+TL_INT, cmsg);

		cMessage * pmsg = new cMessage("PRINT_TLON");
		pmsg->setKind(PRINT_TLON);
		sendPrintInfoPacket(pmsg);

		printChUse(); // Print bw
	}else{
		if(randgen->boolGen(0.95)){
			primaryBusy_D = primaryBusy;
		}else{
			primaryBusy_D = ! primaryBusy;
		}

		retx_count = 0;

		// Open primary channels.
		tl_on = false;

		// Schedule next event.
		cmsg->setName("TL_START");
		cmsg->setKind(TL_START);
		scheduleAt(SIMTIME_DBL(simTime())+TNL_INT, cmsg);
		cMessage * pmsg;
		if(primaryBusy_D == true)
			pmsg = new cMessage("PRINT_PD_ON", PRINT_PD_ON);
		else
			pmsg = new cMessage("PRINT_PD_OFF", PRINT_PD_OFF);
		sendPrintInfoPacket(pmsg);

		// Restart.
		tryToAllocAndSend();
		tryToAllocAndReceive();

	}
}

void BaseStation::sendTransmissionResultPacket(dataPacket * dpkt){
#ifdef BS_DEBUG
	cout << "BS#" << myCell << " sendTransmissionResultPacket#" << dpkt->getID() << endl;
#endif
	ctlPacket * cpkt = new ctlPacket();
	cpkt->setName("TX_RESULT");
	cpkt->setKind(TX_RESULT);
	cpkt->setBitLength(0);
	cpkt->setBitError(false);
	cpkt->setClientID(dpkt->getSenderID());
	cpkt->setDatapacketID(dpkt->getID());
	sendCtlPacket(cpkt, dpkt->getSenderID());
}

void BaseStation::handleNewReqNotification(ctlPacket * cpkt){ // Sent from the client
	enLocalreceiveQ(cpkt);
	tryToAllocAndReceive();
}

// This means the transmission is successful.
void BaseStation::handleTransmissionResultPacket(cMessage * pkt){

	if(!primaryBusy_D){ // is primary
		retx_count = 0;
	}
	// Timeout event is canceled in deallocSendBW
	if(deallocSendBW(((ctlPacket *)pkt)->getClientID(), true)){
		tryToAllocAndSend();
		tryToAllocAndReceive();
	}
	delete pkt;
}

void BaseStation::sendDataPacket(dataPacket * dpkt, int i, bool p){
	dpkt->setReceiverBSSendTime(SIMTIME_DBL(simTime()));
	cGate * g;
	if(p)
		g = gate("pdataout", i);
	else
		g = gate("sdataout", i);
//	cChannel * cch = g->getChannel();
//	if(cch->isBusy())
//		sendDelayed(dpkt, cch->getTransmissionFinishTime() - simTime(), "g$o");
//	else
	send(dpkt, g);
}

void BaseStation::sendCtlPacket(cMessage * pkt, int i){
	cGate * g = gate("control$o", i);
	cChannel * cch = g->getTransmissionChannel();
	if(cch->isBusy()){
		sendDelayed(pkt, cch->getTransmissionFinishTime() - simTime(), g);
	}
	else
		send(pkt, g);
}

void BaseStation::sendInterBSDataPacket(dataPacket * dpkt){
	cGate * g = gate("intercell$o");
	send(dpkt, g);
}

void BaseStation::sendPrintInfoPacket(cMessage * cmsg){
	send(cmsg, "sink");
}

void BaseStation::handlePrimaryONOFFPacket(psPacket * pspkt){
	// Check all the PSs interfering with this cell.
	if(pspkt->getKind() == PRIMARY_ON){
		PS[pspkt->getPs()] = 1;
	}
	else{
		PS[pspkt->getPs()] = 0;
	}
	bool prev = primaryBusy;
	primaryBusy = false;

	for(int i = 0; i < PS_NUM; i++){
		if(PS[i] != -1){
			primaryBusy |= PS[i]; // 0: it may be free. 1: it is busy
		}
	}
	if(primaryBusy == prev) // The status is the same with previous status, nothing to do.
		return;

	// PrimaryBusy is changed!
	BRUpdate(true); // Update primary channel bitrate.

	// Send the notification to all the clients.
	cMessage * cmsg;
	for(int i = 0; i < totalClient; i ++){
		if(primaryBusy){
			cmsg= new cMessage("PRIMARY_ON");
			cmsg->setKind(PRIMARY_ON);
		}else{
			cmsg= new cMessage("PRIMARY_OFF");
			cmsg->setKind(PRIMARY_OFF);
		}
		sendCtlPacket(cmsg, i);
	}

	if(primaryBusy){
		cMessage * pmsg = new cMessage("PRINT_PON");
		pmsg->setKind(PRINT_PON);
		sendPrintInfoPacket(pmsg);
	}else{
		cMessage * pmsg = new cMessage("PRINT_POFF");
		pmsg->setKind(PRINT_POFF);
		sendPrintInfoPacket(pmsg);
	}
}

void BaseStation::setChannelBRfromBW(int i, double bw, bool p){
	double br;
	if(p && primaryBusy){
		br = bw * log2(1 + (pow(randgen->rayGen(RAY_ROW), 2) * BW_BR_S)
				/ (pow(randgen->rayGen(RAY_ROW), 2) * BW_BR_N0_P * bw));
	}else{
		br = bw * log2(1 + (pow(randgen->rayGen(RAY_ROW), 2) * BW_BR_S)
				/ (BW_BR_N0 * bw));
	}
	setChannelBR(i, br, p);
}

void BaseStation::setChannelBR(int i, double br, bool p){
//	if(tl_on && p) // Don't touch primary channel when tl is on.
//		return;
	if(br < 0.0001){
		fprintf(stderr, "ERROR - BS: trying to set bit rate to zero: %lf.\n",br);
		fflush(stdout);
		return;
	}
	cGate * g;
	if(p)
		g = gate("pdataout", i);
	else
		g = gate("sdataout", i);
	cDatarateChannel * ch = (cDatarateChannel *)(g->getTransmissionChannel());
	ch->setDatarate(br); // Set the data rate of this channel.
}

// When tl_on is set, close all the primary channels.
void BaseStation::closePrimaryChannels(){
	for(int i = 0; i < totalClient; i++){
		cGate * g = gate("pdataout", i);
		cDatarateChannel * ch = (cDatarateChannel *)(g->getTransmissionChannel());
		ch->setDatarate(1); // Can set to be 0, which means infinite.
	}
}

void BaseStation::freeChannel(int i, bool p){
	cDatarateChannel * ch;
	if(p)
		ch = (cDatarateChannel *)gate("pdataout", i)->getTransmissionChannel();
//	ch->forceTransmissionFinishTime(simTime());
	else
		ch = (cDatarateChannel *)gate("sdataout", i)->getTransmissionChannel();
	ch->forceTransmissionFinishTime(simTime());
}

void BaseStation::finish() {
	free(randgen);
	free(localreceiveQ);
	free(localsendQ);
	free(oslocalreceiveQ);
	free(oslocalsendQ);
	fclose(outfp2);
}

void BaseStation::printQInfo(int i){
	if(i == 0){
		printf("localreceiveQ-----------------\n");
		list<ctlPacket*>::iterator it;
		for(it = localreceiveQ->begin(); it != localreceiveQ->end(); it ++){
			printf("now %lf: %d - %ld - %lf - %lf - %lf - %d\n", SIMTIME_DBL(simTime()),
					(*it)->getClientID(), (*it)->getDatapacketID(),
					(*it)->getDeadline(), (*it)->getSubmissionTime(),
					(*it)->getDelay(), (*it)->getSize());
		}
		printf("-----------------\n");
	}else if(i == 1){
		printf("localsendQ-----------------\n");
		list<dataPacket*>::iterator it;
		for(it = localsendQ->begin(); it != localsendQ->end(); it ++){
			printf("%d - %ld - %d - %lf - %d\n",
					(*it)->getSenderID(), (*it)->getID(), (*it)->getBCprimaryChannel(),
					(*it)->getDeadline(), (*it)->getSize());
		}
		printf("-----------------\n");
	}else if(i == 2){
		printf("oslocalreceiveQ-----------------\n");
		list<ctlPacket*>::iterator it;
		for(it = oslocalreceiveQ->begin(); it != oslocalreceiveQ->end(); it ++){
			printf("now %lf: %d - %ld - %lf - %lf - %lf - %d\n", SIMTIME_DBL(simTime()),
					(*it)->getClientID(), (*it)->getDatapacketID(),
					(*it)->getDeadline(), (*it)->getSubmissionTime(),
					(*it)->getDelay(), (*it)->getSize());
		}
		printf("-----------------\n");
	}else if(i == 3){
		printf("oslocalsendQ-----------------\n");
		list<dataPacket*>::iterator it;
		for(it = oslocalsendQ->begin(); it != oslocalsendQ->end(); it ++){
			printf("%d - %ld - %d - %lf - %d\n",
					(*it)->getSenderID(), (*it)->getID(), (*it)->getBCprimaryChannel(),
					(*it)->getDeadline(), (*it)->getSize());
		}
		printf("-----------------\n");
	}
	fflush(stdout);
}

void BaseStation::analyzeChUse(bool p, bool r, long bw, bool add){
	// P/S RT/NRT up/down
	double now = SIMTIME_DBL(simTime());
	if(p){
		if(r){
			pRTWorkload += (now - lastPRTBWtime) * lastPRTBW;
			if(add)
				lastPRTBW += bw;
			else
				lastPRTBW -= bw;
			lastPRTBWtime = now;
		}else{
			pNRTWorkload += (now - lastPNRTBWtime) * lastPNRTBW;
			if(add)
				lastPNRTBW += bw;
			else
				lastPNRTBW -= bw;
			lastPNRTBWtime = now;
		}
	}else{
		cout << "ERROR. anaylyzeChUse. Secondary channel." << endl;
	}
}

void BaseStation::printChUse(){
	fprintf(outfp2, "BW %lf %lf %lf %ld [%ld %ld]\n", SIMTIME_DBL(simTime()),
			pRTWorkload, pNRTWorkload, totalPBW-freePBW,
			lastPRTBW, lastPNRTBW);
}

BaseStation::~BaseStation(){

}
