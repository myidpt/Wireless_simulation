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

import Cell;
import primarystation.PrimaryStation;
import masterrouter.MasterRouter;

network World
{
    types:
        channel idealchannel extends ned.IdealChannel
        {
        }
    submodules:
        cell0: Cell {
            parameters:
                Id = 0;
                numPs = 1;
                @display("p=200,50");
        }
        cell1: Cell {
            parameters:
                Id = 1;
                numPs = 2;
                @display("p=50,200");
        }
        cell2: Cell {
            parameters:
                Id = 2;
                numPs = 3;
                @display("p=200,200");
        }
        cell3: Cell {
            parameters:
                Id = 3;
                numPs = 2;
                @display("p=350,200");
        }
        cell4: Cell {
            parameters:
                Id = 4;
                numPs = 1;
                @display("p=50,350");
        }
        cell5: Cell {
            parameters:
                Id = 5;
                numPs = 2;
                @display("p=200,350");
        }
        cell6: Cell {
            parameters:
                Id = 6;
                numPs = 1;
                @display("p=350,350");
        }

        ps0: PrimaryStation {
            parameters:
                Id = 0;
                @display("p=200,125");
        }
        ps1: PrimaryStation {
            parameters:
                Id = 1;
                @display("p=125,275");
        }
        ps2: PrimaryStation {
            parameters:
                Id = 2;
                @display("p=285,275");
        }

        masterrouter: MasterRouter {
            @display("p=200,265");
        }
    connections:
        // Primary station <--> Cells
        ps0.cell[0] <--> idealchannel <--> cell0.pchdetect[0];
        ps0.cell[1] <--> idealchannel <--> cell1.pchdetect[0];
        ps0.cell[2] <--> idealchannel <--> cell2.pchdetect[0];
        ps0.cell[3] <--> idealchannel <--> cell3.pchdetect[0];

        ps1.cell[0] <--> idealchannel <--> cell1.pchdetect[1];
        ps1.cell[1] <--> idealchannel <--> cell2.pchdetect[1];
        ps1.cell[2] <--> idealchannel <--> cell4.pchdetect[0];
        ps1.cell[3] <--> idealchannel <--> cell5.pchdetect[0];

        ps2.cell[0] <--> idealchannel <--> cell2.pchdetect[2];
        ps2.cell[1] <--> idealchannel <--> cell3.pchdetect[1];
        ps2.cell[2] <--> idealchannel <--> cell5.pchdetect[1];
        ps2.cell[3] <--> idealchannel <--> cell6.pchdetect[0];

        // Cells <--> Cells
        cell0.intercell <--> idealchannel <--> masterrouter.intercell[0];
        cell1.intercell <--> idealchannel <--> masterrouter.intercell[1];
        cell2.intercell <--> idealchannel <--> masterrouter.intercell[2];
        cell3.intercell <--> idealchannel <--> masterrouter.intercell[3];
        cell4.intercell <--> idealchannel <--> masterrouter.intercell[4];
        cell5.intercell <--> idealchannel <--> masterrouter.intercell[5];
        cell6.intercell <--> idealchannel <--> masterrouter.intercell[6];
}
