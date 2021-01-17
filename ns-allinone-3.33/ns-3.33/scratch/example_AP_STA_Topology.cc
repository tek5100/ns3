//Assignement 5 AP/STA Topology


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/applications-module.h"
#include "ns3/propagation-loss-model.h"

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
	uint32_t rtsThreshold = 65535;
	double SimulationTime = 100;

	uint32_t packetSize = 512;
	bool shortGuardInterval = false;
	uint32_t chWidth = 40;
	int ap1_x = 0;
	int ap1_y = 0;
	int sta1_x = 10;
	int sta1_y = 0;

	std::string data_rate ("10Mb/s");


	//Create AP
	NodeContainer wifiApNodes;
	wifiApNodes.Create (1);

	//Create STA
	NodeContainer wifiStaNodes;
	wifiStaNodes.Create (1);

	YansWifiPhyHelper wifiPhy; // [deprecated] = YansWifiPhyHelper::Default ();
	wifiPhy.Set ("ChannelNumber", UintegerValue (11));

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	//wifiChannel.AddPropagationLoss("ns3::TwoRayGroundPropagationLossModel", "Frequency", DoubleValue(2.4e+09));
	wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency", DoubleValue(2.4e+09));
	wifiPhy.SetChannel (wifiChannel.Create ());


	WifiHelper wifi;
	// [deprecated] wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
	wifi.SetStandard (WIFI_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager", "RtsCtsThreshold", UintegerValue (rtsThreshold));

	WifiMacHelper wifiMac;
	//wifiMac.SetType("ns3::AdhocWifiMac"); //Using Access Point topology instead

	NetDeviceContainer wifiApDevices;
	NetDeviceContainer wifiStaDevices;
	NetDeviceContainer wifiDevices;


	//Configure the STA node
	Ssid ssid = Ssid ("AP");
	wifiMac.SetType ("ns3::StaWifiMac","Ssid", SsidValue (ssid));
	wifiStaDevices.Add (wifi.Install (wifiPhy, wifiMac, wifiStaNodes.Get (0)));

	//Configure the AP node
	ssid = Ssid ("AP");
	wifiMac.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssid));
	wifiApDevices.Add (wifi.Install (wifiPhy, wifiMac, wifiApNodes.Get (0)));

	//Association of both AP and STA
	wifiDevices.Add (wifiStaDevices);
	wifiDevices.Add (wifiApDevices);

	// Set channel width
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (chWidth));
	// Set guard interval
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (shortGuardInterval));


	//Mobility for the AP
	MobilityHelper APmobility;
	Ptr<ListPositionAllocator> APpositionAlloc = CreateObject<ListPositionAllocator> ();
	APpositionAlloc->Add (Vector (ap1_x, ap1_y, 0.0));
	APmobility.SetPositionAllocator (APpositionAlloc);
	APmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	APmobility.Install (wifiApNodes.Get (0));

	//Mobility for the STA
	MobilityHelper STAmobility;
	Ptr<ListPositionAllocator> STApositionAlloc = CreateObject<ListPositionAllocator> ();
	STApositionAlloc->Add (Vector (sta1_x, sta1_y,0.0));
	STAmobility.SetPositionAllocator (STApositionAlloc);
	APmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	STAmobility.Install (wifiStaNodes.Get (0));


	//Configure the IP stack
	InternetStackHelper stack;
	stack.Install (wifiApNodes);
	stack.Install (wifiStaNodes);
	Ipv4AddressHelper address;
	address.SetBase ("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer ipv4 = address.Assign (wifiDevices); //Both AP and STA are in wifiDevices object
	Ipv4Address sinkAddress = ipv4.GetAddress (0);

	uint16_t port = 9;
	//Configure the CBR generator
	PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (sinkAddress, port));
	ApplicationContainer apps_sink = sink.Install (wifiStaNodes);
	//ApplicationContainer apps_sink = sink.Install (wifiStaNodes.Get (0));

	OnOffHelper onoff ("ns3::UdpSocketFactory", InetSocketAddress (sinkAddress, port));
	onoff.SetConstantRate (DataRate (data_rate), packetSize);
	onoff.SetAttribute ("StartTime", TimeValue (Seconds (0.5)));
	onoff.SetAttribute ("StopTime", TimeValue (Seconds (SimulationTime)));
	ApplicationContainer apps_source = onoff.Install (wifiApNodes.Get (0));

	apps_sink.Start (Seconds (0.5));
	apps_sink.Stop (Seconds (SimulationTime));




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
