#define BUF_SIZE 1024


void mySerial_Setup(void);
int mySerial_Readn(char *data, int len);
void mySerial_Writen(const char *data, int len);
void mySerial_WriteString(const char *str);
