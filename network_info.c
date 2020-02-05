/*------------------------------------------------------------------------
 * network_info.c
 *              System network information
 *
 * Copyright (c) 2020, EnterpriseDB Corporation. All Rights Reserved.
 *
 *------------------------------------------------------------------------
 */

#include "postgres.h"
#include "system_stats.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

void ReadFileContent(const char *file_name, uint64 *data);
void ReadReceiveBytes(const char *interface, uint64 *rx_bytes);
void ReadTransmitBytes(const char *interface, uint64 *tx_bytes);
void ReadReceivePackets(const char *interface, uint64 *rx_packets);
void ReadTransmitPackets(const char *interface, uint64 *tx_packets);
void ReadReceiveErrors(const char *interface, uint64 *rx_errors);
void ReadTransmitErrors(const char *interface, uint64 *tx_errors);
void ReadReceiveDropped(const char *interface, uint64 *rx_dropped);
void ReadTransmitDropped(const char *interface, uint64 *tx_dropped);
void ReadSpeedMbps(const char *interface, uint64 *speed);

void ReadFileContent(const char *file_name, uint64 *data)
{
	FILE       *fp = NULL;
	char       *line_buf = NULL;
	size_t     line_buf_size = 0;
	ssize_t    line_size = 0;

	/* Read the file for network traffics */
	fp = fopen(file_name, "r");

	if (!fp)
	{
		char net_file_name[MAXPGPATH];
		snprintf(net_file_name, MAXPGPATH, "%s", file_name);

		ereport(WARNING,
				(errcode_for_file_access(),
					errmsg("can not open file %s for reading network statistics",
					net_file_name)));
		return;
	}

	/* Get the first line of the file. */
	line_size = getline(&line_buf, &line_buf_size, fp);

	/* Read the content of the file and convert to int64 from string */
	if (line_size > 0)
		*data = atoll(line_buf);

	/* Free the allocated line buffer */
	if (line_buf != NULL)
	{
		free(line_buf);
		line_buf = NULL;
	}

	fclose(fp);
}

/* This function is used to read the number of bytes received for specified network interface */
void ReadReceiveBytes(const char *interface, uint64 *rx_bytes)
{
	char file_name[MAXPGPATH];
	memset(file_name, 0, MAXPGPATH);

	/* file name used to read the number of bytes received */
	sprintf(file_name, "/sys/class/net/%s/statistics/rx_bytes", interface);
	ReadFileContent(file_name, rx_bytes);
}

/* This function is used to read the number of bytes transmitted for specified network interface */
void ReadTransmitBytes(const char *interface, uint64 *tx_bytes)
{
	char file_name[MAXPGPATH];
	memset(file_name, 0, MAXPGPATH);

	/* file name used to read the number of bytes transmitted */
	sprintf(file_name, "/sys/class/net/%s/statistics/tx_bytes", interface);
	ReadFileContent(file_name, tx_bytes);
}

/* This function is used to read the number of packets received for specified network interface */
void ReadReceivePackets(const char *interface, uint64 *rx_packets)
{
	char file_name[MAXPGPATH];
	memset(file_name, 0, MAXPGPATH);

	/* file name used to read the number of packets received */
	sprintf(file_name, "/sys/class/net/%s/statistics/rx_packets", interface);
	ReadFileContent(file_name, rx_packets);
}

/* This function is used to read the number of packets transmitted for specified network interface */
void ReadTransmitPackets(const char *interface, uint64 *tx_packets)
{
	char file_name[MAXPGPATH];
	memset(file_name, 0, MAXPGPATH);

	/* file name used to read the number of packets transmitted */
	sprintf(file_name, "/sys/class/net/%s/statistics/tx_packets", interface);
	ReadFileContent(file_name, tx_packets);
}

/* This function is used to read the number of errors during receiver for specified network interface */
void ReadReceiveErrors(const char *interface, uint64 *rx_errors)
{
	char file_name[MAXPGPATH];
	memset(file_name, 0, MAXPGPATH);

	/* file name used to read the number of errors during receiver */
	sprintf(file_name, "/sys/class/net/%s/statistics/rx_errors", interface);
	ReadFileContent(file_name, rx_errors);
}

/* This function is used to read the number of errors during transmission for specified network interface */
void ReadTransmitErrors(const char *interface, uint64 *tx_errors)
{
	char file_name[MAXPGPATH];
	memset(file_name, 0, MAXPGPATH);

	/* file name used to read the number of errors during transmission */
	sprintf(file_name, "/sys/class/net/%s/statistics/tx_errors", interface);
	ReadFileContent(file_name, tx_errors);
}

/* This function is used to read the number of packets dropped during receiver for specified network interface */
void ReadReceiveDropped(const char *interface, uint64 *rx_dropped)
{
	char file_name[MAXPGPATH];
	memset(file_name, 0, MAXPGPATH);

	/* file name used to read the number of packets dropped during receiver */
	sprintf(file_name, "/sys/class/net/%s/statistics/rx_dropped", interface);
	ReadFileContent(file_name, rx_dropped);
}

/* This function is used to read the number of packets dropped during transmission for specified network interface */
void ReadTransmitDropped(const char *interface, uint64 *tx_dropped)
{
	char file_name[MAXPGPATH];
	memset(file_name, 0, MAXPGPATH);

	/* file name used to read the number of packets dropped during transmission */
	sprintf(file_name, "/sys/class/net/%s/statistics/tx_dropped", interface);
	ReadFileContent(file_name, tx_dropped);
}

/* This function is used to read the speed in Mbps for specified network interface */
void ReadSpeedMbps(const char *interface, uint64 *speed)
{
	char file_name[MAXPGPATH];
	memset(file_name, 0, MAXPGPATH);

	/* file name used to read the speed of interface in Mbps */
	sprintf(file_name, "/sys/class/net/%s/speed", interface);
	ReadFileContent(file_name, speed);
}

void ReadNetworkInformations(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
	Datum      values[Natts_network_info];
	bool       nulls[Natts_network_info];
	char       interface_name[MAXPGPATH];
	char       ipv4_address[MAXPGPATH];
	char       ipv6_address[MAXPGPATH];
	uint64     speed_mbps = 0;
	uint64     tx_bytes = 0;
	uint64     tx_packets = 0;
	uint64     tx_errors = 0;
	uint64     tx_dropped = 0;
	uint64     rx_bytes = 0;
	uint64     rx_packets = 0;
	uint64     rx_errors = 0;
	uint64     rx_dropped = 0;

	// First find out interface and ip address of that interface
	struct ifaddrs *ifaddr;
	struct ifaddrs *ifa;
	int ret_val;
	char host[MAXPGPATH];

	memset(nulls, 0, sizeof(nulls));
	memset(interface_name, 0, MAXPGPATH);
	memset(ipv4_address, 0, MAXPGPATH);
	memset(ipv6_address, 0, MAXPGPATH);

	memset(host, 0, MAXPGPATH);

	/* Below function is used to creates a linked list of structures describing
	 * the network interfaces of the local system
	 */
	if (getifaddrs(&ifaddr) == -1)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("Failed to get network interface")));
		return;
	}

	/* Iterate through all network interfaces */
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		/* Do not get network interface name so continue reading other interfaces */
		if (ifa->ifa_addr == NULL)
			continue;

		/* Below function is used to get address to name translation */
		ret_val = getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, MAXPGPATH, NULL, 0, NI_NUMERICHOST);

		if(ifa->ifa_addr->sa_family == AF_INET)
		{
			if (ret_val != 0)
			{
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("getnameinfo() failed: %s", gai_strerror(ret_val))));
				nulls[Anum_net_ipv4_address] = true;
			}

			memcpy(interface_name, ifa->ifa_name, strlen(ifa->ifa_name));
			memcpy(ipv4_address, host, MAXPGPATH);

			ReadSpeedMbps(interface_name, &speed_mbps);
			ReadReceiveBytes(interface_name, &rx_bytes);
			ReadTransmitBytes(interface_name, &tx_bytes);
			ReadReceivePackets(interface_name, &rx_packets);
			ReadTransmitPackets(interface_name, &tx_packets);
			ReadReceiveErrors(interface_name, &rx_errors);
			ReadTransmitErrors(interface_name, &tx_errors);
			ReadReceiveDropped(interface_name, &rx_dropped);
			ReadTransmitDropped(interface_name, &tx_dropped);

			values[Anum_net_interface_name] = CStringGetTextDatum(interface_name);
			values[Anum_net_ipv4_address] = CStringGetTextDatum(ipv4_address);
			values[Anum_net_ipv6_address] = CStringGetTextDatum(ipv6_address);
			values[Anum_net_speed_mbps] = Int64GetDatumFast(speed_mbps);
			values[Anum_net_tx_bytes] = Int64GetDatumFast(tx_bytes);
			values[Anum_net_tx_packets] = Int64GetDatumFast(tx_packets);
			values[Anum_net_tx_errors] = Int64GetDatumFast(tx_errors);
			values[Anum_net_tx_dropped] = Int64GetDatumFast(tx_dropped);
			values[Anum_net_rx_bytes] = Int64GetDatumFast(rx_bytes);
			values[Anum_net_rx_packets] = Int64GetDatumFast(rx_packets);
			values[Anum_net_rx_errors] = Int64GetDatumFast(rx_errors);
			values[Anum_net_rx_dropped] = Int64GetDatumFast(rx_dropped);

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);

			//reset the value again
			memset(interface_name, 0, MAXPGPATH);
			memset(ipv4_address, 0, MAXPGPATH);
			memset(ipv6_address, 0, MAXPGPATH);
			speed_mbps = 0;
			tx_bytes = 0;
			tx_packets = 0;
			tx_errors = 0;
			tx_dropped = 0;
			rx_bytes = 0;
			rx_packets = 0;
			rx_errors = 0;
			rx_dropped = 0;
		}
	}

	freeifaddrs(ifaddr);
}