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

//
// TODO generated message class
//

message ctlPacket extends cPacket
{
    int clientID = 0; // ID of the client, if sent from the client.
    long datapacketID = 0; // ID of the packet I want.
    int size = 0; // Size of the queued packet
    double delay = 10; // The delay it can tolerate
    double submissionTime = 0; // The time of submitting the packet.
    double deadline = 0;
    // Only for B->C messages
    long BW = 0; // The bitrate the base station asks the client to set.
    bool primaryChannel = true; // The bitrate is primary bit rate.
    bool realtime;
    int resendCount = 0;
}
