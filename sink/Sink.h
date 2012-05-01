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

#ifndef __SINK_H__
#define __SINK_H__

#include <omnetpp.h>
#include "General.h"
#include "packet/dataPacket_m.h"
#include "packet/ctlPacket_m.h"

class Sink : public cSimpleModule
{
protected:
	double totalRTdelay;
	double totalNRTdelay;
	long totalRTpkts;
	long totalNRTpkts;
	long totalRTretx;
	long totalNRTretx;
	long RTmiss;
	long NRTmiss;

	double pRTWorkload;
	double pNRTWorkload;
	double sRTWorkload;
	double sNRTWorkload;
	long lastPRTBW;
	long lastPNRTBW;
	long lastSRTBW;
	long lastSNRTBW;
	double lastPRTBWtime;
	double lastPNRTBWtime;
	double lastSRTBWtime;
	double lastSNRTBWtime;

	int totalClient;
	int cellId;
	FILE * outfp;
	FILE * outfp2;
	double recorded;

    virtual void initialize();
    virtual void handleMessage(cMessage *);
//    void analyzeChUse(ctlPacket *);
    void printDataPacketInfo(dataPacket *);
    void printPrimaryONOFFInfo(bool);
    void printPDetectionONInfo();
    void printPDetectedONOFFInfo(bool);
    void printStat();
    void printChUse(ctlPacket *);
    void finish();
};

#endif
