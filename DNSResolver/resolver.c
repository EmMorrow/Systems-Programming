#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<math.h>

typedef unsigned int dns_rr_ttl;
typedef unsigned short dns_rr_type;
typedef unsigned short dns_rr_class;
typedef unsigned short dns_rdata_len;
typedef unsigned short dns_rr_count;
typedef unsigned short dns_query_id;
typedef unsigned short dns_flags;

typedef struct {
	char *name;
	dns_rr_type type;
	dns_rr_class class;
	dns_rr_ttl ttl;
	dns_rdata_len rdata_len;
	unsigned char *rdata;
} dns_rr;

struct dns_answer_entry;
struct dns_answer_entry {
	char* value;
	struct dns_answer_entry *next;
};
typedef struct dns_answer_entry dns_answer_entry;

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;
	unsigned char c;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
}

void append(char* s, char c) {
        // int len = strlen(s);
		int len = sizeof(s);
        s[len] = c;
        s[len+1] = '\0';
}



void canonicalize_name(char *name) {
	/*
	 * Canonicalize name in place.  Change all upper-case characters to
	 * lower case and remove the trailing dot if there is any.  If the name
	 * passed is a single dot, "." (representing the root zone), then it
	 * should stay the same.
	 *
	 * INPUT:  name: the domain name that should be canonicalized in place
	 */

	int namelen, i;

	// leave the root zone alone
	if (strcmp(name, ".") == 0) {
		return;
	}

	namelen = strlen(name);
	// remove the trailing dot, if any
	if (name[namelen - 1] == '.') {
		name[namelen - 1] = '\0';
	}

	// make all upper-case letters lower case
	for (i = 0; i < namelen; i++) {
		if (name[i] >= 'A' && name[i] <= 'Z') {
			name[i] += 32;
		}
	}
}

int name_ascii_to_wire(char *name, unsigned char *wire) {
	 unsigned char domain[1024];

	 size_t name_size = strlen(name);

	 int per_ind = 0;
	 int count = 0;
	 for (int i = 0; i < name_size; i++) {
		 if (name[i] == '.') {
			 domain[per_ind] = count;
			 count = 0;
			 per_ind = i + 1;
		 }
		 else {
		 	// append(domain, name[i]);
			domain[i + 1] = name[i];
			count = count + 1;
	 	}
		if (i == name_size - 1) {
			domain[per_ind] = count;
			count = 0;
			per_ind = i + 2;
			domain[per_ind] = count;
		}
	 }
	 memcpy(wire + 12, domain, name_size + 2);
	 return name_size + 2;
}

char *name_ascii_from_wire(unsigned char *wire, int *indexp) {
	 char* name = malloc(1024);
	 int name_ind = 0;
	 int i = *indexp;
	 int offset = wire[i+1];
	 i = offset; // maybe
	 while(wire[i] != 0x00) {
		 int num_chars = wire[i];
		 i += 1;
		 for (int j = 0; j < num_chars; j += 1) {
			 name[name_ind] = wire[i];
			 i += 1;
			 name_ind += 1;
		 }
		 if (wire[i + 1] != 0x00) {
			 name[name_ind] = '.';
		 }
		 name_ind += 1;
	 }


	 return name;
}

char * toArray(int number) {
        int n = log10(number) + 1;
        int i;
      char *numberArray = calloc(n, sizeof(char));
        for ( i = 0; i < n; ++i, number /= 10 )
        {
            numberArray[i] = number % 10;
        }
        return numberArray;
}

char * translate_ip (unsigned char * wire, int *indexp, int size) {
	char* ip = malloc(1024);
	int i = *indexp;
	int offset = 0;
	for (int j = 0; j < size; j++) {
		int k = wire[i];
		char arr[20];
		sprintf(arr,"%i",k);
		if (k > 99) {
			// size 3
			if (j != size - 1) {
				arr[3] = '.';
				memcpy(ip + offset, arr, 4);
				offset += 4;
			}
			else {
				memcpy(ip + offset, arr, 3);
				ip[offset+3] = '\0';

			}
		}
		else if (k > 9) {
			// size 2
			if (j != size - 1) {
				arr[2] = '.';
				memcpy(ip + offset, arr, 3);
				offset += 3;
			}
			else {
				memcpy(ip + offset, arr, 2);
				ip[offset+2] = '\0';
			}

		}
		else if(k >= 0) {
			//size 1
			if (j != size - 1) {
				arr[1] = '.';
				memcpy(ip + offset, arr, 2);
				offset += 2;
			}
			else {
				memcpy(ip + offset, arr, 1);
				ip[offset+1] = '\0';

			}
		}
		else {
			// 0
		}

		i = i + 1;
	}
	return ip;
}

dns_rr rr_from_wire(unsigned char *wire, int *indexp, int query_only, int recv_len) {
	dns_rr rr;
	int ind = *indexp;
	int answer_ind = ind + 12; // index of the answer name for alias
	int* anexp = &answer_ind;

	if (query_only == 1) { // you need to get the ip address
		rr.type = wire[ind + 3];
		rr.class = wire[ind + 5];
		rr.name = name_ascii_from_wire(wire, indexp);
		rr.rdata_len = wire[ind + 11];
		rr.rdata = translate_ip(wire, anexp, rr.rdata_len);
	}
	else { // if it's the alias
		rr.type = wire[ind + 3];
		rr.class = wire[ind + 5];
		rr.name = name_ascii_from_wire(wire, indexp);
		rr.rdata = name_ascii_from_wire(wire, anexp);
	}
	return rr;
}

unsigned short create_dns_query(char *qname, dns_rr_type qtype, unsigned char *wire) {
	unsigned char header[] = {0x27, 0xd6, 0x01, 0x00,
 	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	size_t s = sizeof(header);
	memcpy(wire, header, s);

	unsigned char footer[] = {0x00, 0x01, 0x00, 0x01};

	int size = name_ascii_to_wire(qname, wire);
	int a = size + 12;
	memcpy(wire + a,footer,4);
	return 16 + size;
}

dns_answer_entry *get_answer_address(char *qname, dns_rr_type qtype, unsigned char *wire, int recv_len) {
	/*
	 * Extract the IPv4 address from the answer section, following any
	 * aliases that might be found, and return the string representation of
	 * the IP address.  If no address is found, then return NULL.
	 *
	 * INPUT:  qname: the string containing the name that was queried
	 * INPUT:  qtype: the integer representation of type of the query (type A == 1)
	 * INPUT:  wire: the pointer to the array of bytes representing the DNS wire message
	 * OUTPUT: a linked list of dns_answer_entrys the value member of each
	 * reflecting either the name or IP address.  If
	 */

	// two types of records that can be returned: 1 (ip address) and 5

	// rr_from_wire();
	dns_answer_entry* curr;
	dns_answer_entry* prev;
	dns_answer_entry* head = NULL;
	// for each value i in valsToStore:
		// head = (dns_answer_entry*) malloc(sizeof(dns_answer_entry));
		// head->value = i;
		// head = (*head)->next;
	// head = NULL;

	int a = wire[6];
	int ans_num = wire[7];
	int n = 12;
	int* offset = &n;
	int j = 0;
	int count = 0;
	int prev_was_alias = 0;

	for (int i = 0; i < ans_num; i += 1) {
		while (count < 1040) {
			if (wire[j] == 0xC0) {
				n = j;
				offset = &n;
				if (!prev_was_alias) {
					count = 1040;
				}
				else {
					prev_was_alias = 0;
				}
			}
			j++;
		}
		count = 0;
		if (wire[j + 2] == 5) {
			dns_rr rr = rr_from_wire(wire, offset, 0, recv_len);
			if (i == 0) {
				head = (dns_answer_entry*) malloc(sizeof(dns_answer_entry));
				head->value = rr.rdata;
				curr = head;
			}
			else {
				prev = curr;
				curr = (dns_answer_entry*) malloc(sizeof(dns_answer_entry));
				curr->value = rr.rdata;
				prev->next = curr;
			}
			prev_was_alias = 1;
		}
		else
		{
			dns_rr rr = rr_from_wire(wire, offset, 1, recv_len);
			if (i == 0) {
				head = (dns_answer_entry*) malloc(sizeof(dns_answer_entry));
				head->value = rr.rdata;
				curr = head;
			}
			else {
				prev = curr;
				curr = (dns_answer_entry*) malloc(sizeof(dns_answer_entry));
				curr->value = rr.rdata;
				// curr->value = "hey";
				prev->next = curr;
			}
		}
	}
	return head;
}

int send_recv_message(unsigned char *request, int requestlen, unsigned char *response, char *server, unsigned short port) {
	/*
	 * Send a message (request) over UDP to a server (server) and port
	 * (port) and wait for a response, which is placed in another byte
	 * array (response).  Create a socket, "connect()" it to the
	 * appropriate destination, and then use send() and recv();
	 */
	int skt;
	struct sockaddr_in addr;
	skt = socket(AF_INET, SOCK_DGRAM,0);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((u_short) port);
	addr.sin_addr.s_addr = inet_addr(server);

	connect(skt, (struct sockaddr *) &addr, sizeof(addr));

	// communicate using send and recv or read and write
	int s = send(skt, request, requestlen, 0);
	int r = recv(skt, response, 1024, 0);
	close(skt);
	return r;
}

dns_answer_entry *resolve(char *qname, char *server) {
	// q name is www.example.com
	// server is 8.8.8.8
	// unsigned char* wire_msg = malloc(1024);
	unsigned char wire_msg[1024];
	char* recv_msg = malloc(1024);
	// unsigned char recv_msg[1024];

	unsigned char* ans_msg = malloc(1024);
	dns_rr_type qtype = 0x01;


	// int msg_len = 33;
	int msg_len = create_dns_query(qname, qtype, wire_msg); // need to figure out how to find qtype (1)
	int recv_len = send_recv_message(wire_msg, msg_len, recv_msg, server, 53); // 53 is the default UDP port
	dns_answer_entry* entry_list = get_answer_address(qname, qtype, recv_msg, recv_len);

	// int recv_len = send_recv_message(msg, msg_len, recv_msg, server, 53); // 53 is the default UDP port
	// print_bytes(wire_msg, msg_len);
	return entry_list;
}

int main(int argc, char *argv[]) {
	dns_answer_entry *ans;
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <domain name> <server>\n", argv[0]);
		exit(1);
	}
	ans = resolve(argv[1], argv[2]);
	while (ans != NULL) {
		printf("%s\n", ans->value);
		ans = ans->next;
	}
}
