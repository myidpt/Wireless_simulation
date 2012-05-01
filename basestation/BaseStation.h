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

#ifndef BASESTATION_H_
#define BASESTATION_H_

#include <omnetpp.h>
#include <list>
#include "General.h"
#include "generators/RandGenerator.h"
#include "packet/dataPacket_m.h"
#include "packet/ctlPacket_m.h"
#include "packet/psPacket_m.h"

using namespace std;

class BaseStation : public cSimpleModule {
private:
//	ExpGenerator * expgen;
	int myCell;
	int totalClient;
	RandGenerator * randgen;

	list<ctlPacket *> * localreceiveQ; // waiting packets
	list<dataPacket *> * localsendQ; // waiting packets
	list<ctlPacket *> * oslocalreceiveQ; // outstanding packets
	list<dataPacket *> * oslocalsendQ; // outstanding packets

	long localreceivebwAlloc[MAX_C_INCELL];
	long localsendbwAlloc[MAX_C_INCELL];
//	bool localreceiveCh[MAX_C_INCELL]; // true: primary. false: secondary.
//	bool localsendCh[MAX_C_INCELL]; // true: primary. false: secondary.

	long totalPBW;
	long freePBW;
//	long totalSBW;
//	long freeSBW;

	double pRTWorkload;
	double pNRTWorkload;
//	double sRTWorkload;
//	double sNRTWorkload;
	long lastPRTBW;
	long lastPNRTBW;
//	long lastSRTBW;
//	long lastSNRTBW;
	double lastPRTBWtime;
	double lastPNRTBWtime;
//	double lastSRTBWtime;
//	double lastSNRTBWtime;
	FILE * outfp2;

/*
	long totalPReceiveBW;
	long freePReceiveBW;
	long totalPSendBW;
	long freePSendBW;
	long totalSReceiveBW;
	long freeSReceiveBW;
	long totalSSendBW;
	long freeSSendBW;
*/

	bool primaryBusy;
	bool primaryBusy_D; // Detected primary is busy.
	int cellLoc[PS_NUM][CELL_NUM];
	int PS[PS_NUM]; // -1 : No interference from that PS, otherwise, 0: PS off, 1: PS on.
	bool tl_on;

	int retx_count; // re-transmission count

	void tryToAllocAndReceive();
	void tryToAllocAndSend();

	bool deallocReceiveBW(int, bool);
	bool deallocSendBW(int, bool);

	void enLocalsendQ(dataPacket *);
	void enLocalreceiveQ(ctlPacket *);

	inline void handleDataPacket(dataPacket *);
	inline void handleInterBSDataPacket(dataPacket *);
	inline void handleNewReqNotification(ctlPacket *);
	inline void handleTransmissionResultPacket(cMessage *);
	inline void handlePrimaryONOFFPacket(psPacket *);
	inline void handleBRUpdate(cMessage *);
	inline void handleLPacket(cMessage *);
	inline void handleDownTimeoutPacket(dataPacket *);
	inline void handleUpTimeoutPacket(ctlPacket *);
//	void handleCancelPacket(ctlPacket *);
//	bool scheduleReceiveReqToSCh(ctlPacket *);
	inline bool scheduleReceiveReqToPCh(ctlPacket *);
//	bool scheduleSendReqToSCh(dataPacket *);
	inline bool scheduleSendReqToPCh(dataPacket *);

//	void tryToSendLocalDataPacket();
	inline void sendTransmissionResultPacket(dataPacket *);
	inline void sendDataPacket(dataPacket *, int, bool);// Boolean value true means from primary channel.
	inline void sendCtlPacket(cMessage *, int);
	inline void sendInterBSDataPacket(dataPacket *);
	inline void sendPrintInfoPacket(cMessage *);

	inline void BRUpdate(bool primary);
	inline void closePrimaryChannels();
	inline void setChannelBRfromBW(int, double, bool);
	inline void setChannelBR(int, double, bool);// Boolean value true means from primary channel.
	inline void freeChannel(int i, bool p);

//	void deschedulePacketsInPCh(); // When primary channel is busy, you need to deschedule the packets in primary channels.

	inline void analyzeChUse(bool p, bool r, long bw, bool add);
	inline void printChUse();

	// Debugger
	inline void printQInfo(int i);

public:
	void initialize();
	void handleMessage(cMessage *);
	void finish();
	virtual ~BaseStation();
};

#endif /* BASESTATION_H_ */
