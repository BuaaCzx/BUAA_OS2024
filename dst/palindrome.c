#include <stdio.h>
#include <string.h>
char s[100005];
int check(char *s) {
	int len = strlen(s);
	for (int l = 0, r = len - 1; l < r; l++, r--) {
		if (s[l] != s[r]) {
			return 0;
		}
	}
	return 1;
}
int main() {
	int n;
	scanf("%d", &n);
	sprintf(s, "%d", n);
	if (check(s)) {
		printf("Y\n");
	} else {
		printf("N\n");
	}
	return 0;
}
