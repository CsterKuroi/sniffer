#define HAVE_REMOTE
#include "pcap.h"
#include "remote-ext.h"
#include <stdlib.h>
#include <stdio.h>
#include <pcap.h> 
#include <remote-ext.h>        //winpcap��ͷ�ļ�
#include <winsock2.h>
#include <process.h>              //���̱߳�̵�ͷ�ļ�
#include <windows.h>
#include <Iphlpapi.h>             //��ȡ�����õ�ͷ�ļ�
#pragma comment(lib,"ws2_32")
#pragma comment(lib,"wpcap")
#pragma comment(lib,"IPHlpApi")
#define LINE_LEN 16
#define outPutFile ".\\OutPut.txt"	//���������ļ�
#define MAX_PACK_NUM  1000       //���յ���౨����
#define MAX_PACK_LEN  65535     //���յ����IP����  
#define MAX_ADDR_LEN  16        //���ʮ���Ƶ�ַ����󳤶�  

#define outPutFile ".\\OutPut.txt"	//���������ļ�
#define SIO_RCVALL              _WSAIOW(IOC_VENDOR,1)  

//����IP�����ײ�
typedef struct _iphdr{
	unsigned char h_lenver;        //4λ�ײ�����+4λIP�汾��  
	unsigned char tos;             //8λ��������TOS  
	unsigned short total_len;      //16λ�ܳ��ȣ��ֽڣ�  
	unsigned short ident;          //16λ��ʶ  
	unsigned short frag_and_flags; //3λ��־λ  
	unsigned char ttl;             //8λ����ʱ�� TTL  
	unsigned char proto;           //8λЭ�� (TCP, UDP ������)  
	unsigned short checksum;       //16λIP�ײ�У���  
	unsigned int sourceIP;         //32λԴIP��ַ  
	unsigned int destIP;           //32λĿ��IP��ַ  
} IP_HEADER;

//����TCP�����ײ�
typedef struct _tcphdr{
	unsigned short th_sport;       //16λԴ�˿�  
	unsigned short th_dport;       //16λĿ�Ķ˿�  
	unsigned int  th_seq;          //32λ���к�  
	unsigned int  th_ack;          //32λȷ�Ϻ�  
	unsigned char th_lenres;       //4λ�ײ�����/6λ������  
	unsigned char th_flag;         //6λ��־λ  
	unsigned short th_win;         //16λ���ڴ�С  
	unsigned short th_sum;         //16λУ���  
	unsigned short th_urp;         //16λ��������ƫ����  
} TCP_HEADER;

//����UDP�����ײ�
typedef struct _udphdr{
	unsigned short uh_sport;    //16λԴ�˿�  
	unsigned short uh_dport;    //16λĿ�Ķ˿�  
	unsigned short uh_len;      //16λ����  
	unsigned short uh_sum;      //16λУ���  
} UDP_HEADER;

/* �ص�����ԭ�� */
void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);

int main(int argc, char **argv)
{
	pcap_if_t *alldevs;
	pcap_if_t *d;
	int inum, inum1;
	int i = 0;
	pcap_t *adhandle;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_dumper_t *dumpfile;

	u_int netmask;
	struct bpf_program fcode;
	char * packet_filter = "";

	/* ������������� */
	/*if(argc != 2)
	{
	printf("usage: %s filename", argv[0]);
	return -1;
	}*/

	/* ��ȡ�����豸�б� */
	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		exit(1);
	}

	/* ��ӡ�б� */
	for (d = alldevs; d; d = d->next)
	{
		printf("%d. %s", ++i, d->name);
		if (d->description)
			printf(" (%s)\n", d->description);
		else
			printf(" (No description available)\n");
	}

	if (i == 0)
	{
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
		return -1;
	}

	printf("Enter the interface number (1-%d):", i);
	scanf("%d", &inum);

	if (inum < 1 || inum > i)
	{
		printf("\nInterface number out of range.\n");
		/* �ͷ��б� */
		pcap_freealldevs(alldevs);
		return -1;
	}
	printf("choose the packet you want to catch.\n");
	printf("1:tcp\n");
	printf("2:udp\n");
	printf("3:arp\n");
	printf("4:icmp\n");
	printf("5:http\n");
	printf("6:ftp\n");
	printf("7:all\n");
	printf("Enter the number (1-7):");
	scanf("%d", &inum1);
	if (inum1 == 1){
		packet_filter = "ip and tcp";
	}
	else if (inum1 == 2){
		packet_filter = "ip and udp";
	}
	else if (inum1 == 3){
		packet_filter = "arp";
	}
	else if (inum1 == 4){
		packet_filter = "icmp";
	}
	else if (inum1 == 5){
		packet_filter = "tcp port 80";
	}
	else if (inum1 == 6){
		packet_filter = "tcp port 20 or tcp port 21";
	}
	/* ��ת��ѡ�е������� */
	for (d = alldevs, i = 0; i< inum - 1; d = d->next, i++);


	/* �������� */
	if ((adhandle = pcap_open_live(d->name,          // �豸��
		65536,            // Ҫ��׽�����ݰ��Ĳ���
		// 65535��֤�ܲ��񵽲�ͬ������·���ϵ�ÿ�����ݰ���ȫ������
		PCAP_OPENFLAG_PROMISCUOUS,    // ����ģʽ
		1000,             // ��ȡ��ʱʱ��
		// NULL,             // Զ�̻�����֤
		errbuf            // ���󻺳��
		)) == NULL)
	{
		fprintf(stderr, "\nUnable to open the adapter. %s is not supported by WinPcap\n", d->name);
		/* �ͷ��豸�б� */
		pcap_freealldevs(alldevs);
		return -1;
	}
	if (d->addresses != NULL)
		/* ��ýӿڵ�һ����ַ������ */
		netmask = ((struct sockaddr_in *)(d->addresses->netmask))->sin_addr.S_un.S_addr;
	else
		/* ����ӿ�û�е�ַ����ô���Ǽ���һ��C������� */
		netmask = 0xffffff;
	/* �򿪶��ļ� */
	dumpfile = pcap_dump_open(adhandle, outPutFile);

	if (dumpfile == NULL)
	{
		fprintf(stderr, "\nError opening output file\n");
		return -1;
	}
	/////
	if (pcap_compile(adhandle, &fcode, packet_filter, 1, netmask) <0)
	{
		fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");
		/* �ͷ��豸�б� */
		pcap_freealldevs(alldevs);
		return -1;
	}

	//���ù�����
	if (pcap_setfilter(adhandle, &fcode)<0)
	{
		fprintf(stderr, "\nError setting the filter.\n");
		/* �ͷ��豸�б� */
		pcap_freealldevs(alldevs);
		return -1;
	}/////
	printf("\nlistening on %s... Press Ctrl+C to stop...\n", d->description);

	/* �ͷ��豸�б� */
	pcap_freealldevs(alldevs);

	/* ��ʼ���� */
	pcap_loop(adhandle, 0, packet_handler, (unsigned char *)dumpfile);

	return 0;
}

/* �ص������������������ݰ� */
void packet_handler(u_char *dumpfile, const struct pcap_pkthdr *header, const u_char *pkt_data)
{

	/* �������ݰ������ļ� */
	int i;



	IP_HEADER     *pip;

	char         *protocol;
	u_int ip_len;
	u_short sport, dport;

	//for (i = 0; i <= header->len-1; i++)
	//{
	//	printf("%.2x ", pkt_data[i]);
	//}
	//printf("\n");//��ӡ֡

	/*�ж��Ƿ���IP����*/
	if (pkt_data[13] == 0){

		/* ���IP���ݰ�ͷ����λ�� */
		printf("    IP packet\n");
		pip = (IP_HEADER *)(pkt_data +
			14); //��̫��ͷ������

		SOCKADDR_IN    addr;
		TCP_HEADER *ptcp = (TCP_HEADER *)(pkt_data +14 + sizeof(IP_HEADER));
		UDP_HEADER *pudp = (UDP_HEADER *)(pkt_data +14 + sizeof(IP_HEADER));

		if (pip->proto == 6)
			protocol = "TCP";
		else if (pip->proto == 17)
			protocol = "UDP";
		else if (pip->proto == 1)
			protocol = "ICMP";
		else protocol = "Other";
		printf("    %s  ", protocol);
		//���IP
		addr.sin_addr.s_addr = pip->sourceIP;
		printf("%15s  --> ", inet_ntoa(addr.sin_addr));
		addr.sin_addr.s_addr = pip->destIP;
		printf("%15s  ", inet_ntoa(addr.sin_addr));
		printf("\n");
		//��� �˿� �� ������Ϣ
		if (pip->proto == 6) { //TCP
			printf("    port:");
			printf("%8d  -->  %8d", ntohs(ptcp->th_sport), ntohs(ptcp->th_dport));
			if (ntohs(ptcp->th_sport) == 80 || ntohs(ptcp->th_dport)==80)
				printf("    http");
			else if (ntohs(ptcp->th_sport) == 20 || ntohs(ptcp->th_dport) == 20 || ntohs(ptcp->th_sport) == 21 || ntohs(ptcp->th_dport) == 21)
				printf("     ftp");
			putchar('\n');

		}
		else if (pip->proto == 17){ //UDP
			printf("    port:");
			printf("%8d  -->  %8d", ntohs(pudp->uh_sport), ntohs(pudp->uh_dport));
			putchar('\n');
		}
		else{
			putchar('\n');

		}
	}
	else if (pkt_data[13] == 6){
		printf("    arp packet\n");

	}
	/*�������6����arp����*/

	printf("    SOURCE MAC:");
	for (i = 7; i <= 12; i++)
	{
		printf("%.2x:", pkt_data[i - 1]);

	}
	printf("-->");
	printf("DEST MAC:");
	for (i = 1; i <= 6; i++)
	{
		printf("%.2x:", pkt_data[i - 1]);

	}

	printf("\n");

	printf("    data length:%d\n", header->len);
	printf("\n\n");
	pcap_dump(dumpfile, header, pkt_data);
}