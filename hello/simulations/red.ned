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

package hello.simulations;

import inet.base.LifecycleController;
import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.nodes.aodv.AODVRouter;
import inet.nodes.inet.AdhocHost;
import inet.world.radio.ChannelControl;
import inet.world.radio.IdealChannelModel;
import inet.world.scenario.ScenarioManager;


//
// TODO auto-generated type
//
network Red
{
    parameters:
        int numNodes = default(1);

        @display("bgb=564,255");
    submodules:
        modulo[numNodes]: Modulo {
            @display("p=272,161;i=device/pocketpc;is=vs");
        }
        configurator: IPv4NetworkConfigurator {
            parameters:
                addDefaultRoutes = false;
                addStaticRoutes = false;
                addSubnetRoutes = false;
                config = xml("<config><interface hosts='*' address='192.168.x.x' netmask = '255.255.0.0' /> </config>");
                @display("p=245,20;is=vs");
        }
        routingTableRecorder: RoutingTableRecorder {
            @display("p=147,20;is=vs");
        }
        scenarioManager: ScenarioManager {
            @display("p=330,20;is=vs");
        }
        lifecycleController: LifecycleController {
            @display("p=435,20;is=vs");
        }
        channelControl: IdealChannelModel {
            @display("p=46,20;is=vs");
        }
}
