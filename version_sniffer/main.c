#define _CRT_SECURE_NO_WARNINGS //������ʾ���� strcpy() memcpy() �Ⱥ����ľ���

#include "pcap.h"
#include <Packet32.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#include <windows.h>

#define true 1
#define false 0


//code from njit8021xclient project
#define START 1
#define REQUEST 1
#define RESPONSE 2
#define SUCCESS 3
#define FAILURE 4
#define H3CDATA 10

#define IDENTITY 1
#define NOTIFICATION 2
#define MD5 4
#define AVAILABLE 20

static void XOR(uint8_t data[], unsigned dlen, const char key[], unsigned klen);
int GetMacFromDevice(uint8_t mac[6], const char *devicename);
// From base64.c
void b64_decode(char *b64src, char *clrdst);

int main()
{
	int output_length = 0;
	char key;
	char DeviceName[256];
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_if_t *alldevs;
	pcap_if_t *dev;

	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
	}
	printf("Select an adapter by input Y\n");
	for (dev = alldevs; dev != NULL; dev = dev->next)
	{
		printf("%s (%s)\n", dev->name, dev->description);
		key = getchar();
		if (key == 'Y' || key == 'y')
		{
			strcpy(DeviceName, dev->name);
			break;
		}
	}
	pcap_freealldevs(alldevs);

	/** ������ **/
	struct pcap_pkthdr *header = NULL;
	pcap_t	*handle; // adapter handle
	const int DefaultTimeout = 1000;			//���ý��ճ�ʱ��������λms	
	handle = pcap_open_live(DeviceName, 65536, 0, DefaultTimeout, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "%s\n", errbuf);
		return -1;
	}

	/** ���ù������������ص�ַ **/
	char	FilterStr[200];
	struct bpf_program	fcode;
	uint8_t	myMAC[6];
	char clientIsFound = 0;
	int retcode;
	uint8_t	*captured = NULL;

	GetMacFromDevice(myMAC, DeviceName);
	sprintf(FilterStr, "(ether src host %02x:%02x:%02x:%02x:%02x:%02x)", myMAC[0], myMAC[1], myMAC[2], myMAC[3], myMAC[4], myMAC[5]);
	pcap_compile(handle, &fcode, FilterStr, 1, 0xff);
	pcap_setfilter(handle, &fcode);

	/** �ȴ��ͻ��˷������ **/
	printf("\n���iNode���е�¼��\n");
	printf("\nWaiting for auth\n");
	char base64[29]="";
	while (!clientIsFound)
	{
		printf(".");
		retcode = pcap_next_ex(handle, &header, &captured);
		if (retcode == 1 && captured[12] == 0x88 && captured[13] == 0x8e &&/*(EAP_Code)*/captured[15] == 0x00 && captured[18] == RESPONSE)
			// ������������0x06,0x07Ϊ��ͷ�ģ����base64��Ϣ�Ĵ�
			for (int ii = 00; ii < header->len - 1; ii++)
				if (*(captured + ii) == 0x06 && *(captured + ii + 1) == 0x07)
				{
					memcpy(base64, captured + ii + 2, 28);
					base64[28] = 0x00; //����ַ�����β
					clientIsFound = 1;
					break;
				}
	}
	printf("\nclient connected.\n");
	printf("base64: %s\n", base64);
	unsigned char version_data[32] = "";
	char H3C_key1[] = "HuaWei3COM1X";
	char H3C_key2[] = "Oly5D62FaE94W7";
	unsigned char random_key[32];
	const char H3C_VERSION[16] =
	{ 0x43, 0x48, 0x12, 0x56, 0x35, 0x2e, 0x32, 0x30, 0x2d, 0x30, 0x34, 0x30, 0x37 };

	/*ʹ����Կ HuaWei3COM1X ����*/
	// base64����
	b64_decode(base64, version_data);
	XOR(version_data, 20, H3C_key1, strlen(H3C_key1));
	sprintf(random_key, "%02hhx%02hhx%02hhx%02hhx", version_data[16], version_data[17], version_data[18], version_data[19]);
	XOR(version_data, 16, random_key, 8);
	printf("\n\n====ʹ����Կ HuaWei3COM1X ����====\n");
	printf("�汾��Ϊ: %s\n", version_data);
	printf("\n16���Ʊ�ʾΪ: ");
	for (int i = 0; i < strnlen(version_data, 20); i++)
	{
		printf("%02hhx", version_data[i]);
	}
	printf("\n\nC�������ݸ�ʽ:\nconst char H3C_VERSION[16]=\n{");
	for (int i = 0; i < strnlen(version_data, 20) - 1; i++)
	{
		printf("0x%02hhx,", version_data[i]);
	}
	printf("0x%02hhx};\n", version_data[strnlen(version_data, 20) - 1]);

	/*ʹ����Կ Oly5D62FaE94W7 ����*/
	// base64����
	b64_decode(base64, version_data);
	XOR(version_data, 20, H3C_key2, strlen(H3C_key2));
	sprintf(random_key, "%02hhx%02hhx%02hhx%02hhx", version_data[16], version_data[17], version_data[18], version_data[19]);
	XOR(version_data, 16, random_key, 8);
	printf("\n\n====ʹ����Կ Oly5D62FaE94W7 ����====\n");
	printf("�汾��Ϊ: %s\n", version_data);
	printf("\n16���Ʊ�ʾΪ: ");
	for (int i = 0; i < strnlen(version_data,20); i++)
	{
		printf("%02hhx", version_data[i]);
	}
	printf("\n\nC�������ݸ�ʽ:\nconst char H3C_VERSION[16]=\n{");
	for (int i = 0; i < strnlen(version_data, 20) - 1; i++)
	{
		printf("0x%02hhx,", version_data[i]);
	}
	printf("0x%02hhx};\n", version_data[strnlen(version_data, 20) - 1]);

	printf("\n\nPress Enter to exit.\n");
	getchar();
	getchar();
	return 0;
}


#define OID_802_3_PERMANENT_ADDRESS             0x01010101
#define OID_802_3_CURRENT_ADDRESS               0x01010102
static
int GetMacFromDevice(uint8_t mac[6], const char *devicename)
{
#if defined(WIN32)
	LPADAPTER lpAdapter;
	PPACKET_OID_DATA  OidData;
	BOOLEAN status;

	lpAdapter = PacketOpenAdapter((char *)devicename);

	if (!lpAdapter || (lpAdapter->hFile == INVALID_HANDLE_VALUE)) {
		return 0;
	}

	OidData = (PPACKET_OID_DATA)malloc(6 + sizeof(PACKET_OID_DATA));
	if (OidData == NULL) {
		return 0;
	}

	OidData->Oid = OID_802_3_CURRENT_ADDRESS;
	OidData->Length = 6;
	ZeroMemory(OidData->Data, 6);

	status = PacketRequest(lpAdapter, FALSE, OidData);
	if (status == false) {
		return 0;
	}

	memcpy((void *)mac, (void *)OidData->Data, 6);

	free(OidData);
	PacketCloseAdapter(lpAdapter);

	return 0;
#else
	int	fd;
	int	err;
	struct ifreq	ifr;

	fd = socket(PF_PACKET, SOCK_RAW, htons(0x0806));
	assert(fd != -1);

	assert(strlen(devicename) < IFNAMSIZ);
	strncpy(ifr.ifr_name, devicename, IFNAMSIZ);
	ifr.ifr_addr.sa_family = AF_INET;

	err = ioctl(fd, SIOCGIFHWADDR, &ifr);
	assert(err != -1);
	memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);

	err = close(fd);
	assert(err != -1);
	return 0;
#endif
}

// ����: XOR(data[], datalen, key[], keylen)
//
// ʹ����Կkey[]������data[]����������
//��ע���ú���Ҳ�ɷ������ڽ��ܣ�
static
void XOR(uint8_t data[], unsigned dlen, const char key[], unsigned klen)
{
	unsigned int	i, j;

	// �Ȱ�������һ��
	for (i = 0; i<dlen; i++)
		data[i] ^= key[i%klen];
	// �ٰ�������ڶ���
	for (i = dlen - 1, j = 0; j<dlen; i--, j++)
		data[i] ^= key[j%klen];
}