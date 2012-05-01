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

message dataPacket extends cPacket
{
    long ID = 0; // ID of the packet.
    int senderID = 0; // Sender ID
    int senderCell = 0; // Sender's cell ID
    int receiverID = 0; // Receiver ID
    int receiverCell = 0; // Receiver Cell
    int size = 0; // the size of the packet. Need it because when used as result packet, bitlength != size.
    double delay = 0;
    double deadline = 0;
    bool realtime = true;
    double submissionTime = 0; // The time of submitting the packet.
    double clientSendTime = 0;
    double senderBSReceiveTime = 0;
    double senderBSSendTime = 0;
    double receiverBSReceiveTime = 0;
    double receiverBSSendTime = 0;
    double clientReceiveTime = 0;
    int resendCount1 = 0; // If the packet is not sent correctly, retry sending. C->B
    int resendCount2 = 0; // If the packet is not sent correctly, retry sending. B->C
    bool CBprimaryChannel = true;
    bool BCprimaryChannel = true;
}
