/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



/*
    TEK5100/9100 Exercise Part B Assignment : Solution based on wifi-simple-adhoc-grid.cc

    1. Setup a wireless ad hoc network. You may use examples/wireless/wifi-simple-adhoc-grid.cc as a base.

    2. Install a routing protocol.

    3. Setup three UDP traffic flows.

    4. Setup the ns-3 flow monitor for each of these flows.

    5. Now using the flow monitor, observe the throughput of each of the
    UDP flows.

    7. Repeat the experiment for different number of nodes, flows, topologies, protocols, rates, packet sizes, mobility models with RTS/CTS enabled on the wifi devices.

    8. Show the difference in throughput and packet drops if any.



*/

//the codes below are build based on ns3 version 3.30.1

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/applications-module.h"
#include "ns3/propagation-loss-model.h"

//#include <nqos-wifi-mac-helper>

// routing protocols
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/olsr-helper.h"

// header file specific to this exercise
#include "myapp.h"

// added for flow monitoring
#include "ns3/gnuplot.h"
#include "ns3/flow-monitor-module.h"
#include <ns3/flow-monitor-helper.h>

// animator
#include "ns3/netanim-module.h"
// std library files
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

NS_LOG_COMPONENT_DEFINE ("TEK5100_Exercise_Part_B");

using namespace ns3;


//============================================

Ptr<ConstantVelocityMobilityModel> cvmm;
double position_interval = 1.0;
std::string tracebase = "scratch/wireless";

// two callbacks
void printPosition()
{
  Vector thePos = cvmm->GetPosition();
  Simulator::Schedule(Seconds(position_interval), &printPosition);
  std::cout << "position: " << thePos << std::endl;
}

void stopMover()
{
  cvmm -> SetVelocity(Vector(0,0,0));
}


//=======================================



void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet);

uint32_t MacTxDropCount, PhyTxDropCount, PhyRxDropCount;

void
MacTxDrop(Ptr<const Packet> p)
{
  NS_LOG_INFO("Packet Drop");
  MacTxDropCount++;
}

void
PrintDrop()
{
  std::cout << Simulator::Now().GetSeconds() << "\tMAC TX packets loss:" << MacTxDropCount << "\tPHY TX packets loss:"<< PhyTxDropCount << "\tPHY RX packets loss:" << PhyRxDropCount << "\n";
  Simulator::Schedule(Seconds(5.0), &PrintDrop);
}

void
PhyTxDrop(Ptr<const Packet> p)
{
  NS_LOG_INFO("Packet Drop");
  PhyTxDropCount++;
}
void
PhyRxDrop(Ptr<const Packet> p, WifiPhyRxfailureReason wrfr)
{
  NS_LOG_INFO("Packet Drop");
  PhyRxDropCount++;
}
int main (int argc, char *argv[])
{
	//uint32_t rtsThreshold = 65000;
	double SimulationTime = 100;
	double SinkStartTime =20;
	double ApplicationStartTime = SinkStartTime + 2;

	uint32_t packetSize = 512;
	uint32_t numPackets = 10000000;
	//bool shortGuardInterval = false;
	//uint32_t chWidth = 40;
	int ap1_x = 0;
	int ap1_y = 0;
	int sta1_x = 1100;
	int sta1_y = 0;

	std::string data_rate ("10Mb/s");


	Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue("DsssRate1Mbps"));
	//Config::SetDefault("ns3::WifiRemoteStationManager::PhyControlMode", StringValue("DsssRate1Mbps"));
	//Create 2 nodes
	NodeContainer wifiNodes;
	wifiNodes.Create (2);

	YansWifiPhyHelper wifiPhy;
	wifiPhy.Set ("ChannelNumber", UintegerValue (11));


	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	//wifiChannel.AddPropagationLoss("ns3::TwoRayGroundPropagationLossModel", "Frequency", DoubleValue(2.4e+09));
	wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency", DoubleValue(2.4e+09));
	wifiPhy.SetChannel (wifiChannel.Create ());
	//wifiPhy.SetPreambleDetectionModel("ns3::ThresholdPreambleDetectionModel");
	wifiPhy.SetPreambleDetectionModel("ns3::SimpleFrameCaptureModel");


	WifiHelper wifi;
	// [deprecated] wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
	wifi.SetStandard (WIFI_STANDARD_80211b);
	//wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager", "RtsCtsThreshold", UintegerValue (rtsThreshold));
	wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
	//wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode",StringValue("DsssRate1Mbps"),"ControlMode",StringValue("DsssRate1Mbps"));


	WifiMacHelper wifiMac;
	wifiMac.SetType("ns3::AdhocWifiMac");

	NetDeviceContainer wifiDevices = wifi.Install (wifiPhy, wifiMac, wifiNodes);

	// Set channel width
	//Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (chWidth));
	// Set guard interval
	//Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (shortGuardInterval));



	// Configure the mobility.
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	//Initial position of AP and STA
	positionAlloc->Add (Vector (ap1_x, ap1_y, 2.0));
	positionAlloc->Add (Vector (sta1_x, sta1_y, 2.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiNodes.Get (0));
	mobility.Install (wifiNodes.Get (1));


	//Configure the IP stack
	InternetStackHelper stack;
	stack.Install (wifiNodes);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer ifcont = ipv4.Assign (wifiDevices);

//	Ipv4StaticRoutingHelper ipv4RoutingHelper;
	// Create static routes from N0 to N1
//	Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (wifiDevices.Get(0));
	 //The ifIndex for this outbound route is 1; the first p2p link added
//	staticRouting->AddHostRouteTo ( Ipv4Address ("10.1.1.2"), 1);
	Ipv4StaticRoutingHelper staticRouting;
	Ipv4ListRoutingHelper list;
	list.Add(staticRouting, 0);

	//InternetStackHelper internet;
	stack.SetRoutingHelper(list);
	//internet.Install(wifiNodes);

	Ptr<Ipv4> ipv4_0 = wifiNodes.Get(0)->GetObject<Ipv4>();
	Ptr<Ipv4> ipv4_1 = wifiNodes.Get(1)->GetObject<Ipv4>();

	Ipv4StaticRoutingHelper ipv4RoutingHelper;

	Ptr<Ipv4StaticRouting> sr0 = ipv4RoutingHelper.GetStaticRouting(ipv4_0);
	Ptr<Ipv4StaticRouting> sr1 = ipv4RoutingHelper.GetStaticRouting(ipv4_1);
	Ipv4Address i0 = Ipv4Address("10.1.1.1");
	Ipv4Address i1 = Ipv4Address("10.1.1.2");
	sr0->AddHostRouteTo(i1,i1,1);
	sr1->AddHostRouteTo(i0,i0,1);


	uint16_t sinkPort = 6; // use the same for all apps

	Address sinkAddress1 (InetSocketAddress (ifcont.GetAddress (1), sinkPort)); // interface of n24
	PacketSinkHelper packetSinkHelper1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
	ApplicationContainer sinkApps1 = packetSinkHelper1.Install (wifiNodes.Get (1)); //n1 as sink
	sinkApps1.Start (Seconds (SinkStartTime));
	sinkApps1.Stop (Seconds (SimulationTime));

	Ptr<Socket> ns3UdpSocket1 = Socket::CreateSocket (wifiNodes.Get (0), UdpSocketFactory::GetTypeId ()); //source at n0

	// Create UDP application at n0
	Ptr<MyApp> app1 = CreateObject<MyApp> ();
	app1->Setup (ns3UdpSocket1, sinkAddress1, packetSize, numPackets, DataRate (data_rate));
	wifiNodes.Get (0)->AddApplication (app1);
	app1->SetStartTime (Seconds (ApplicationStartTime));
	app1->SetStopTime (Seconds (SimulationTime));


    std::string fileNameWithNoExtension = "TEK5100_FlowVSThroughput";
		//this is a file which contains throughput performance
    std::string graphicsFileName        = fileNameWithNoExtension + ".png";
    std::string plotFileName            = fileNameWithNoExtension + ".plt";
    std::string plotTitle               = "Flow vs Throughput";
    std::string dataTitle               = "Throughput";

    // Instantiate the plot and set its title.
    Gnuplot gnuplot (graphicsFileName);
    gnuplot.SetTitle (plotTitle);

    // Make the graphics file, which the plot file will be when it
    // is used with Gnuplot, be a PNG file.
    gnuplot.SetTerminal ("png");

    // Set the labels for each axis.
    gnuplot.SetLegend ("Flow", "Throughput");



   Gnuplot2dDataset dataset;
   dataset.SetTitle (dataTitle);
   dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

// Install FlowMonitor on all nodes
  FlowMonitorHelper fmHelper;
  Ptr<FlowMonitor> monitor = fmHelper.InstallAll();

// call the flow monitor function
  ThroughputMonitor(&fmHelper, monitor, dataset);


  // Trace Collisions
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDrop));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDrop));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDrop));

  Simulator::Schedule(Seconds(5.0), &PrintDrop);

  Simulator::Stop (Seconds (SimulationTime));
  AnimationInterface anim ("5th_Assignement.xml");
  Simulator::Run ();

  PrintDrop();

  monitor->CheckForLostPackets ();
  //monitor->SerializeToXmlFile("Assignment4_Monitor.xml", true, true);

//Gnuplot ...continued
  gnuplot.AddDataset (dataset);
  // Open the plot file.
  std::ofstream plotFile (plotFileName.c_str());
  // Write the plot file.
  gnuplot.GenerateOutput (plotFile);
  // Close the plot file.
  plotFile.close ();

  Simulator::Destroy ();

  return 0;
}




void ThroughputMonitor (FlowMonitorHelper *fmhelper, Ptr<FlowMonitor> flowMon,Gnuplot2dDataset DataSet)
	{
        double localThrou=0;
		std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
		Ptr<Ipv4FlowClassifier> classing = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator stats = flowStats.begin (); stats != flowStats.end (); ++stats)
		{
			Ipv4FlowClassifier::FiveTuple fiveTuple = classing->FindFlow (stats->first);
			//std::cout.precision(5);

			if (stats->first == 1){
			std::cout<<"FID:\t" << stats->first
			<<"\t"<< fiveTuple.sourceAddress <<" -> "<<fiveTuple.destinationAddress
			<<"\tTxPck:\t" << stats->second.txPackets
			<<"\tRxPck:\t" << stats->second.rxPackets
            <<"\tDur:\t"<<(stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())
			<<"\tTFTxPck:\t" << stats->second.timeFirstTxPacket.GetSeconds()
			<<"\tTLRxPck:\t"<< stats->second.timeLastRxPacket.GetSeconds()

			<<"\tTHT:\t" << stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024

			<<std::endl;

            localThrou=(stats->second.rxBytes * 8.0 / (stats->second.timeLastRxPacket.GetSeconds()-stats->second.timeFirstTxPacket.GetSeconds())/1024/1024);
			// updata gnuplot data
            DataSet.Add((double)Simulator::Now().GetSeconds(),(double) localThrou);
			//std::cout<<"---------------------------------------------------------------------------"<<std::endl;
			}
		}
			Simulator::Schedule(Seconds(1),&ThroughputMonitor, fmhelper, flowMon,DataSet);
   //if(flowToXml)
      {
	flowMon->SerializeToXmlFile ("ThroughputMonitor.xml", true, true);
      }

	}
