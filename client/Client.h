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

#ifndef CLIENT_H_
#define CLIENT_H_

#include <list>
#include "General.h"
#include "generators/RandGenerator.h"
#include "packet/dataPacket_m.h"
#include "packet/ctlPacket_m.h"

using namespace std;

#include <math.h>
#include <omnetpp.h>

class Client : public cSimpleModule {
private:
	int totalClient;
	int cellId;
	int myId;
	RandGenerator * randgen;
	list<dataPacket *> * sendQ;
	long currentPCHBW;
//	long currentSCHBW;
	long dpktId;
	long osdpktId;
	long pktId;

	bool primaryBusy;
	bool tl_on;

	long totalRTQcost;
	long totalNRTQcost;

	bool generatingPacket;

	inline void handleNewDataPacket(dataPacket *);
	inline void handleReceivedDataPacket(dataPacket *);
	inline void handleOpenChannelPacket(ctlPacket *);
	inline void handleTransmissionResultPacket(cMessage *);
	inline void handleBRUpdate(cMessage *);
	inline void handlePrimayONOFFPacket(bool);
	inline void handleLPacket(cMessage *);
	inline void handleTimeoutPacket(dataPacket *);

	inline void scheduleNewDataPacket(bool);
	inline void findAndSendDataPacket(long, bool); // Boolean value is true if primary channel available.
	inline void setChannelBW(long, bool); // Boolean value is true if primary channel set.
	inline void BRUpdate(bool); // Boolean value is true if primary channel set.
	inline void closePrimaryChannel();
	inline void forceFreeChannel(bool); // forceTransmissionFinishTime

	inline void sendNewReqNotification(dataPacket *);
	inline void sendDataPacket(dataPacket *, bool);
//	void sendSuccessResultPacket(bool p);
//	void sendFailResultPacket(dataPacket *);
	inline void sendResultPacket(bool p);
	inline void sendCtlPacket(cMessage *);
	inline void sendPrintPacket(cMessage *);

public:
	void initialize();
	void handleMessage(cMessage *);
	void finish();
	virtual ~Client();
};

#endif /* CLIENT_H_ */
