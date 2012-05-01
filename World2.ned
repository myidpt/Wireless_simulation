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

network World2
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
                @display("p=75,140");
        }
        cell1: Cell {
            parameters:
                Id = 1;
                numPs = 1;
                @display("p=340,127");
        }
        ps0: PrimaryStation {
            parameters:
                Id = 0;
                @display("p=67,248");
        }
        ps1: PrimaryStation {
            parameters:
                Id = 1;
                @display("p=340,266");
        }
        masterrouter: MasterRouter {
            @display("p=200,137");
        }
    connections:
        // Primary station <--> Cells
        ps0.cell[0] <--> idealchannel <--> cell0.pchdetect[0];
        ps1.cell[0] <--> idealchannel <--> cell1.pchdetect[0];
        // Cells <--> Cells
        cell0.intercell <--> idealchannel <--> masterrouter.intercell[0];
        cell1.intercell <--> idealchannel <--> masterrouter.intercell[1];
}
