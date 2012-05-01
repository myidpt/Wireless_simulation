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

import client.Client;
import basestation.BaseStation;
import primarystation.PrimaryStation;
import sink.Sink;

module Cell
{
    parameters:
        int Id;
        int numClients;
        int numPs; // Number of primary stations that have interference with it.
        @display("i=device/accesspoint_l");
    gates:
        inout pchdetect[numPs];
        inout intercell;
    types:
        channel idealChannel extends ned.IdealChannel
        {
        }
        channel CtlChannel extends ned.DatarateChannel
        {
            datarate = 0;
            delay = 0ms;
        }
        channel PrimaryDataChannel extends ned.DatarateChannel
        {
            datarate = 100bps;
            delay = 0ms;
            ber = 0; // bit error rate.
        }
        channel SecondaryDataChannel extends ned.DatarateChannel
        {
            datarate = 100bps;
            delay = 0ms;
            ber = 0; // bit error rate.
        }
    submodules:
        client[numClients]: Client {
            parameters:
                totalClient = numClients;
                @display("p=277,104");
        }
        basestation: BaseStation {
            parameters:
                totalClient = numClients;
                numPs = numPs;
           		@display("p=78,72");
        }
        sink: Sink{
        	parameters:
        	    numClients = numClients;
        }
    connections:
        // Outside
        for i=0..numPs-1 {
            pchdetect[i] <--> idealChannel <--> basestation.pchdetect[i];
        }
        
        intercell <--> idealChannel <--> basestation.intercell;

        // Inside
        for i=0..numClients-1 {
            client[i].sdataout --> SecondaryDataChannel --> basestation.sdatain[i];
            client[i].sdatain <-- SecondaryDataChannel <-- basestation.sdataout[i];

            client[i].pdataout --> PrimaryDataChannel --> basestation.pdatain[i];
            client[i].pdatain <-- PrimaryDataChannel <-- basestation.pdataout[i];

            client[i].control <--> CtlChannel <--> basestation.control[i];
        }
        
        // Sink
        basestation.sink --> idealChannel --> sink.bsink;
        for i=0..numClients-1 {
        	client[i].sink --> idealChannel --> sink.csink[i];
        }
}
