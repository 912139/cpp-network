#include "PcapFileDevice.h"
#include "PcapLiveDeviceList.h"
#include "SystemUtils.h"
#include "stdlib.h"
#include <iostream>

/**
 * A callback function for the async capture which is called each time a packet is captured
 */
static void onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *writer)
{
	// extract the pcapWriter object form the writer
	pcpp::PcapFileWriterDevice *pcapWriter = (pcpp::PcapFileWriterDevice *)writer;

	pcapWriter->writePacket(*packet);
}

int main(int argc, char *argv[])
{

	if (argc != 2)
	{
		std::cout << "Usage: " << argv[0] << " <ipv4>" << std::endl;
		return EXIT_FAILURE;
	}

	// IPv4 address of the interface we want to sniff
	std::string interfaceIPAddr = argv[1];

	// find the interface by IP address
	pcpp::PcapLiveDevice *dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);
	if (dev == NULL)
	{
		std::cerr << "Cannot find interface with IPv4 address of '" << interfaceIPAddr << "'" << std::endl;
		return 1;
	}

	// before capturing packets let's print some info about this interface
	std::cout << "Interface info:" << std::endl
			  << "   Interface name:        " << dev->getName() << std::endl		   // get interface name
			  << "   Interface description: " << dev->getDesc() << std::endl		   // get interface description
			  << "   MAC address:           " << dev->getMacAddress() << std::endl	   // get interface MAC address
			  << "   Default gateway:       " << dev->getDefaultGateway() << std::endl // get default gateway
			  << "   Interface MTU:         " << dev->getMtu() << std::endl;		   // get interface MTU

	if (dev->getDnsServers().size() > 0)
		std::cout << "   DNS server:            " << dev->getDnsServers().at(0) << std::endl;

	// open the device before start capturing/sending packets
	if (!dev->open())
	{
		std::cerr << "Cannot open device" << std::endl;
		return 1;
	}

	// create a pcap file writer. Specify file name and link type of all packets that
	// will be written to it
	pcpp::PcapFileWriterDevice pcapWriter("output.pcap", pcpp::LINKTYPE_ETHERNET);

	// try to open the file for writing
	if (!pcapWriter.open())
	{
		std::cerr << "Cannot open output.pcap for writing" << std::endl;
		return 1;
	}

	std::cout << std::endl << "Starting async capture (10 seconds)..." << std::endl;

	// start capture in async mode. Give a callback function to call to whenever a packet is captured and the stats
	// object as the cookie
	dev->startCapture(onPacketArrives, &pcapWriter);

	// sleep for 10 seconds in main thread, in the meantime packets are captured in the async thread
	pcpp::multiPlatformSleep(10);

	// stop capturing packets
	dev->stopCapture();

	pcapWriter.close();

	// use the IFileReaderDevice interface to automatically identify file type (pcap/pcap-ng)
	// and create an interface instance that both readers implement
	pcpp::IFileReaderDevice *reader = pcpp::IFileReaderDevice::getReader("output.pcap");

	// verify that a reader interface was indeed created
	if (reader == NULL)
	{
		std::cerr << "Cannot determine reader for file type" << std::endl;
		return 1;
	}

	// open the reader for reading
	if (!reader->open())
	{
		std::cerr << "Cannot open input.pcap for reading" << std::endl;
		return 1;
	}

	/**
	 * A struct for collecting packet statistics
	 */
	struct PacketStats
	{
		int ethPacketCount;
		int ipv4PacketCount;
		int ipv6PacketCount;
		int tcpPacketCount;
		int udpPacketCount;
		int dnsPacketCount;
		int httpPacketCount;
		int sslPacketCount;

		/**
		 * Clear all stats
		 */
		void clear()
		{
			ethPacketCount = 0;
			ipv4PacketCount = 0;
			ipv6PacketCount = 0;
			tcpPacketCount = 0;
			udpPacketCount = 0;
			tcpPacketCount = 0;
			dnsPacketCount = 0;
			httpPacketCount = 0;
			sslPacketCount = 0;
		}

		/**
		 * C'tor
		 */
		PacketStats()
		{
			clear();
		}

		/**
		 * Collect stats from a packet
		 */
		void consumePacket(pcpp::Packet &packet)
		{
			if (packet.isPacketOfType(pcpp::Ethernet))
				ethPacketCount++;
			if (packet.isPacketOfType(pcpp::IPv4))
				ipv4PacketCount++;
			if (packet.isPacketOfType(pcpp::IPv6))
				ipv6PacketCount++;
			if (packet.isPacketOfType(pcpp::TCP))
				tcpPacketCount++;
			if (packet.isPacketOfType(pcpp::UDP))
				udpPacketCount++;
			if (packet.isPacketOfType(pcpp::DNS))
				dnsPacketCount++;
			if (packet.isPacketOfType(pcpp::HTTP))
				httpPacketCount++;
			if (packet.isPacketOfType(pcpp::SSL))
				sslPacketCount++;
		}

		/**
		 * Print stats to console
		 */
		void printToConsole()
		{
			std::cout << "Ethernet packet count: " << ethPacketCount << std::endl
					  << "IPv4 packet count:     " << ipv4PacketCount << std::endl
					  << "IPv6 packet count:     " << ipv6PacketCount << std::endl
					  << "TCP packet count:      " << tcpPacketCount << std::endl
					  << "UDP packet count:      " << udpPacketCount << std::endl
					  << "DNS packet count:      " << dnsPacketCount << std::endl
					  << "HTTP packet count:     " << httpPacketCount << std::endl
					  << "SSL packet count:      " << sslPacketCount << std::endl;
		}
	};

	// create the stats object
	PacketStats stats;

	// the packet container
	pcpp::RawPacket rawPacket;

	// a while loop that will continue as long as there are packets in the input file
	while (reader->getNextPacket(rawPacket))
	{
		// parsed the raw packet
		pcpp::Packet parsedPacket(&rawPacket);

		// collect stats from packet
		stats.consumePacket(parsedPacket);
	}

	// print results
	std::cout << "Results:" << std::endl;
	stats.printToConsole();
}
