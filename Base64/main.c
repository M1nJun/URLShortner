#include <stdio.h>
#include "base64.h"

int main() {
	while(1) {
		printf("(E)ncode, (D)ecode, or (Q)uit: ");
		char buffer[16];
		unsigned int n;
		scanf("%s",buffer);
		if(buffer[0] == 'E' || buffer[0] == 'e') {
			unsigned long long limit = ((unsigned long long) 2) << 31;
			printf("Number must be less than %lld.\n",limit);
			printf("Enter an int: ");
			scanf("%u",&n);
			encode(n,buffer);
			printf("%s\n",buffer);
		} else if(buffer[0] == 'D' || buffer[0] == 'd') {
			printf("Enter code: ");
			scanf("%s",buffer);
			n = decode(buffer);
			printf("%u\n",n);
		} else
		break;
	}
}
