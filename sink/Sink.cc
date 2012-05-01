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

#include "Sink.h"

Define_Module(Sink);

void Sink::initialize()
{
	totalClient = int(par("numClients").longValue());
	cellId = (getId() - C_BASE)/(totalClient+CELL_REMAINDER);
	char outfname[200] = OUT_PATH_PREFIX;
	int len = strlen(outfname);
	// Note: currently we only support less than 10000 client.
	outfname[len] = cellId/10 + '0';
	outfname[len+1] = cellId%10 + '0';
	outfname[len+2] = '\0';
	if( (outfp = fopen(outfname, "w+")) == NULL){
		fprintf(stderr, "ERROR Client: Trace file open failure: %s\n", outfname);
		deleteModule();
	}

	char outfname2[200] = STAT_PATH_PREFIX;
	len = strlen(outfname2);
	outfname2[len] = cellId/10 + '0';
	outfname2[len+1] = cellId%10 + '0';
	outfname2[len+2] = '\0';
	if( (outfp2 = fopen(outfname2, "w+")) == NULL){
		fprintf(stderr, "ERROR Client: Trace file open failure: %s\n", outfname2);
		deleteModule();
	}

	totalRTdelay = 0;
	totalNRTdelay = 0;
	totalRTpkts = 0;
	totalNRTpkts = 0;
	totalRTretx = 0;
	totalNRTretx = 0;
	RTmiss = 0;
	NRTmiss = 0;
	recorded = 0;

/*
	pRTWorkload = 0;
	pNRTWorkload = 0;
	sRTWorkload = 0;
	sNRTWorkload = 0;
	lastPRTBW = 0;
	lastPNRTBW = 0;
	lastSRTBW = 0;
	lastSNRTBW = 0;
	lastPRTBWtime = 0;
	lastPNRTBWtime = 0;
	lastSRTBWtime = 0;
	lastSNRTBWtime = 0;
*/
}

void Sink::handleMessage(cMessage *msg)
{
	if(SIMTIME_DBL(simTime()) - recorded >= STAT_INT){
		printStat();
	}
	switch(msg->getKind()){
	case PRINT_DPKT:
		printDataPacketInfo((dataPacket *)msg);
		break;
	case PRINT_PON:
		printPrimaryONOFFInfo(true);
		break;
	case PRINT_POFF:
		printPrimaryONOFFInfo(false);
		break;
	case PRINT_TLON:
		printPDetectionONInfo();
		break;
	case PRINT_PD_ON:
		printPDetectedONOFFInfo(true);
		break;
	case PRINT_PD_OFF:
		printPDetectedONOFFInfo(false);
		break;
	case PRINT_CH_DOWN_USE:
	case PRINT_CH_UP_USE:
//		analyzeChUse((ctlPacket *)msg);
		printChUse((ctlPacket *)msg);
		break;
	}
	delete msg;
}
/*
void Sink::printDataPacketInfo(dataPacket * dpkt){
	fprintf(outfp,"%.2lf %.4lf %.4lf\n",
		dpkt->getDelay(),
		dpkt->getSenderBSReceiveTime() - dpkt->getClientSendTime(),
		dpkt->getClientReceiveTime() - dpkt->getReceiverBSSendTime());
}
*/

void Sink::printStat(){
	fprintf(outfp2,"%lf %lf %lf %ld %ld %ld %ld %ld %ld\n",
			SIMTIME_DBL(simTime()), totalRTdelay, totalNRTdelay, totalRTpkts, totalNRTpkts,
			totalRTretx, totalNRTretx, RTmiss, NRTmiss);
	totalRTdelay = 0;
	totalNRTdelay = 0;
	totalRTpkts = 0;
	totalNRTpkts = 0;
	totalRTretx = 0;
	totalNRTretx = 0;
	RTmiss = 0;
	NRTmiss = 0;
	recorded = SIMTIME_DBL(simTime());
//	fprintf(outfp, "BW %lf %lf %lf %lf %lf\n", SIMTIME_DBL(simTime()), sRTWorkload, sNRTWorkload, pNRTWorkload, pRTWorkload);
}

void Sink::printDataPacketInfo(dataPacket * dpkt){
	/*
	fprintf(outfp,"%d %d %d %d %ld %ld %d %.4lf %d %d %.4lf %.4lf\n",
		dpkt->getSenderCell(),
		dpkt->getSenderID(),
		dpkt->getReceiverCell(),
		dpkt->getReceiverID(),
		dpkt->getID(),
		(long)dpkt->getBitLength(),
		dpkt->getRealtime(),
		dpkt->getDelay(),

		dpkt->getResendCount1(),
		dpkt->getResendCount2(),

		dpkt->getSubmissionTime(),
		dpkt->getClientReceiveTime() - dpkt->getSubmissionTime()
		);
*/
	if(dpkt->getRealtime()){
		totalRTdelay += dpkt->getClientReceiveTime() - dpkt->getSubmissionTime();
		totalRTpkts ++;
		totalRTretx += dpkt->getResendCount1() + dpkt->getResendCount2();

		fprintf(outfp, "RT: %lf, %lf, %lf, %lf\n", dpkt->getDelay(),
				dpkt->getClientReceiveTime(), dpkt->getSubmissionTime(),
				dpkt->getClientReceiveTime() - dpkt->getSubmissionTime());
//		fflush(stdout);

		if(dpkt->getClientReceiveTime() - dpkt->getSubmissionTime() > dpkt->getDelay()){
			RTmiss ++;
		}
	}else{
		totalNRTdelay += dpkt->getClientReceiveTime() - dpkt->getSubmissionTime();
		totalNRTpkts ++;
		totalNRTretx += dpkt->getResendCount1() + dpkt->getResendCount2();

		fprintf(outfp, "NRT: %lf, %lf, %lf, %lf\n", dpkt->getDelay(),
				dpkt->getClientReceiveTime(), dpkt->getSubmissionTime(),
				dpkt->getClientReceiveTime() - dpkt->getSubmissionTime());
//		fflush(stdout);

		if(dpkt->getClientReceiveTime() - dpkt->getSubmissionTime() > dpkt->getDelay()){
			NRTmiss ++;
		}
	}
}



void Sink::printPrimaryONOFFInfo(bool on){
	if(on)
		fprintf(outfp, "%lf PCH ON.\n", SIMTIME_DBL(simTime()));
	else
		fprintf(outfp, "%lf PCH OFF.\n", SIMTIME_DBL(simTime()));
}

void Sink::printPDetectionONInfo(){
	fprintf(outfp, "%lf PCH Detection ON.\n", SIMTIME_DBL(simTime()));
}

void Sink::printPDetectedONOFFInfo(bool on){
	if(on)
		fprintf(outfp, "%lf PCH Detected ON.\n", SIMTIME_DBL(simTime()));
	else
		fprintf(outfp, "%lf PCH Detected OFF.\n", SIMTIME_DBL(simTime()));
}

/*
void Sink::analyzeChUse(ctlPacket * cpkt){
	// P/S RT/NRT up/down
	double now = SIMTIME_DBL(simTime());
	if(cpkt->getPrimaryChannel()){
		if(cpkt->getRealtime()){
			pRTWorkload += (now - lastPRTBWtime) * lastPRTBW;
			lastPRTBW = cpkt->getBW();
			lastPRTBWtime = now;
		}else{
			pNRTWorkload += (now - lastPNRTBWtime) * lastPNRTBW;
			lastPNRTBW = cpkt->getBW();
			lastPNRTBWtime = now;
		}
	}else{
		if(cpkt->getRealtime()){
			sRTWorkload += (now - lastSRTBWtime) * lastSRTBW;
			lastSRTBW = cpkt->getBW();
			lastSRTBWtime = now;
		}else{
			sNRTWorkload += (now - lastSNRTBWtime) * lastSNRTBW;
			lastSNRTBW = cpkt->getBW();
			lastSNRTBWtime = now;
		}
	}
}
*/

void Sink::printChUse(ctlPacket * cpkt){
	if(cpkt->getKind() == PRINT_CH_DOWN_USE){
		if(cpkt->getPrimaryChannel())
			fprintf(outfp, "[%lf] DOWN P %d %ld\n", SIMTIME_DBL(simTime()), cpkt->getRealtime(), cpkt->getBW());
		else
			fprintf(outfp, "[%lf] DOWN S %d %ld\n", SIMTIME_DBL(simTime()), cpkt->getRealtime(), cpkt->getBW());
	}else{
		if(cpkt->getPrimaryChannel())
			fprintf(outfp, "[%lf] UP P %d %ld\n", SIMTIME_DBL(simTime()), cpkt->getRealtime(), cpkt->getBW());
		else
			fprintf(outfp, "[%lf] UP S %d %ld\n", SIMTIME_DBL(simTime()), cpkt->getRealtime(), cpkt->getBW());
	}
}

void Sink::finish(){
	fclose(outfp);
	fclose(outfp2);
}

/*
void Sink::printDataPacketInfo(dataPacket * dpkt){
	fprintf(outfp,"%d: %ld, %d:%d --> %d:%d  %lf\n"
			"\t%lf %lf %lf %lf %lf %lf %lf [%lf %d %d]\n",
			dpkt->getID(),
			(long)dpkt->getBitLength(),
			dpkt->getSenderCell(),
			dpkt->getSenderID(),
			dpkt->getReceiverCell(),
			dpkt->getReceiverID(),
			dpkt->getDelay(),

			dpkt->getSubmissionTime(), // The time of submitting the packet.
			dpkt->getClientSendTime(),
			dpkt->getSenderBSReceiveTime(),
			dpkt->getSenderBSSendTime(),
			dpkt->getReceiverBSReceiveTime(),
			dpkt->getReceiverBSSendTime(),
			dpkt->getClientReceiveTime(),
			dpkt->getClientReceiveTime() - dpkt->getSubmissionTime(),
			dpkt->getResendCount1(),
			dpkt->getResendCount2());
}
*/
/*
void Sink::printDataPacketInfo(dataPacket * dpkt){
	fprintf(outfp,"ID:%d-%ld, LEN:%ld, CBPCH:%d, BCPCH:%d, RT:%d, DLY:%.4lf, "
		"[%d:%d-->%d:%d]  RESEND: %d %d\n"
		"\tSUB:%.4lf SEND:%.4lf REC:%.4lf CQ:%.4lf C-B:%.4lf BQ:%.4lf B-C:%.4lf\n",
		dpkt->getSenderID(),
		dpkt->getID(),
		(long)dpkt->getBitLength(),
		dpkt->getCBprimaryChannel(),
		dpkt->getBCprimaryChannel(),
		dpkt->getRealtime(),
		dpkt->getDelay(),

		dpkt->getSenderCell(),
		dpkt->getSenderID(),
		dpkt->getReceiverCell(),
		dpkt->getReceiverID(),
		dpkt->getResendCount1(),
		dpkt->getResendCount2(),

		dpkt->getSubmissionTime(),
		dpkt->getClientSendTime(),
		dpkt->getClientReceiveTime(),
		dpkt->getClientSendTime() - dpkt->getSubmissionTime(), // The time of submitting the packet.
		dpkt->getSenderBSReceiveTime() - dpkt->getClientSendTime(),
		dpkt->getReceiverBSSendTime() - dpkt->getSenderBSReceiveTime(),
		dpkt->getClientReceiveTime() - dpkt->getReceiverBSSendTime()
		);
}
 */
