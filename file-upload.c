#include <stdio.h>     // standard io
#include <stdlib.h>    // standardlib
#include <math.h>      // ceil (round up)
#include <unistd.h>    // time
#include <string.h>    // strlen
#include <sys/ioctl.h> // ioctl
#include <fcntl.h>     // open
#include "base64.h"    // library for encoding and decoding base64


/*	Compile:
 *  gcc -c base64.c -o base64.o
 *  gcc file_upload.c base64.o -o file_upload
 */

#ifndef O_WRONLY
#define O_WRONLY	00000001   // Write to a fd
#endif

#ifndef TIOCSTI
#define TIOCSTI		0x5412     // Used to simulate user input
#endif

// Global variables
char LFILE[200];
char RFILE[200];
char tempFile[200];
char device_name[50];
char operating_system[15];
int delayInMicroseconds = 50000;
int threshold = 10000;            // Defines how often the the variable that holds the content is emptied into the file, 
								  // to ensure that the variable is not holding too much

char* getAndEncodeFileContent(const char *LFILE) {
	FILE *fp = fopen(LFILE, "rb");
	if (fp == NULL) {
		printf("\n\nERROR: Could not open file!\n");
		exit(1);
	}
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *buffer = (char *)malloc(fileSize + 1);
	size_t readInBytes = fread(buffer, 1, fileSize, fp);
	if (readInBytes != fileSize) {
		printf("Error while reading the file");
		exit(1);
	}
	unsigned int recommendedBufferSize = b64e_size(fileSize);
	char *encodedBuffer = (char *)malloc(recommendedBufferSize + 1);
	b64_encode(buffer, fileSize, encodedBuffer);
	free(buffer);
	return encodedBuffer;
}

void send(int fd, const char *message) {
	usleep(delayInMicroseconds);
	printf("\n\nMessage:%s", message);
	for (int i = 0; message[i]; ++i) {
		ioctl(fd, TIOCSTI, message + i);
	}
	char *nl = "\n";
	ioctl(fd, TIOCSTI, nl); 
}

void uploadFileWindows(int fd, const char encodedBufferArray[][291], int arraySize) {
	char *temp = (char *)malloc(310);
	// $s = ""
	strncpy(temp, "$s = \"\";", 9);
	send(fd, temp);

	int numberOfRows = threshold / 290;
	for (int i = 0; i < arraySize; ++i) {
		// $s += "290 ZEICHEN";
		strncpy(temp, "$s += \"",8);
		strncat(temp, encodedBufferArray[i], strlen(encodedBufferArray[i]) + 1);
		strncat(temp, "\";", 3);
		send(fd, temp);
		if (i % numberOfRows == 0) {
			// Add-Content -Path "RFILE" -Value $s; $s = "";
			strncpy(temp, "Add-Content -Path \"", 20);
			strncat(temp, RFILE, strlen(RFILE) + 1);
			strncat(temp, "\" -Value $s; $s = \"\";", 22);
			send(fd, temp);
		}
	}

	// Add-Content -Path "RFILE" -Value $s; $s = "";    // add all the rest when the program is done!
	strncpy(temp, "Add-Content -Path \"", 20);
	strncat(temp, RFILE, strlen(RFILE) + 1);
	strncat(temp, "\" -Value $s; $s = \"\";", 22);
	send(fd, temp);

	//	 $e = Get-Content -Path "RFILE"
	strncpy(temp, "$e = Get-Content -Path \"", 25);
	strncat(temp, RFILE, strlen(RFILE) + 1);
	strncat(temp, "\";", 3);
	send(fd, temp);

	// [Convert]::FromBase64String($e) | Set-Content -Path "C:\file.txt" -Encoding Byte
	strncpy(temp, "[Convert]::FromBase64String($e) | Set-Content -Path \"", 54);
	strncat(temp, RFILE, strlen(RFILE) + 1);
	strncat(temp, "\" -Encoding Byte", 17);
	send(fd, temp);
	free(temp);
}

void uploadFileLinux(int fd, const char encodedBufferArray[][291], int arraySize) {
	// echo "290" >> TEMPFILE
	// cat TEMPFILE | base64 -d > RFILE    // einmalig
	char *temp = (char *)malloc(310);
	for (int i = 0; i < arraySize; ++i) {
		strncpy(temp, "echo \"", 7);
		strncat(temp, encodedBufferArray[i], strlen(encodedBufferArray[i]) + 1);
		strncat(temp, "\" >> ",6);
		strncat(temp, tempFile, strlen(tempFile) + 1);
		send(fd, temp);
	}
	strncpy(temp, "cat ", 5);
	strncat(temp, tempFile, strlen(tempFile) + 1);
	strncat(temp, " | base64 -d > ", 16);
	strncat(temp, RFILE, strlen(RFILE) + 1);
	send(fd, temp);
	free(temp);
}

void uploadFile(const char encodedBufferArray[][291], int arraySize) {
	int fd = open(device_name, O_WRONLY);
	
	if (fd < 0) {
		printf("\n\nERROR: Could not open device!\nA possible device name is /dev/pts/3 or /dev/pts/4");
		exit(1);
	}

	if (strcmp(operating_system, "windows") == 0) {
		uploadFileWindows(fd, encodedBufferArray, arraySize);
	}else if (strcmp(operating_system, "linux") == 0) {
		uploadFileLinux(fd, encodedBufferArray, arraySize);
	}else{
		printf("\nERROR: You specified an invalid operating system! Choose between \"linux\" and \"windows\"");
		close(fd);
		exit(1);
	}
	
	close(fd);
}

void prepareEncodedBuffer(const char *encodedBuffer) {
	// Calulate the array size (290 characters per string in the array)
	double result = (double)strlen(encodedBuffer) / 290;
	int arraySize = (int)ceil(result);

	printf("\nStrlen(encodedBuffer): %d", strlen(encodedBuffer));
	printf("\nArray Size: %d\n", arraySize);
	
	char encodedBufferArray[arraySize][291];

	int elementsLeftInLastString = 290 * arraySize - strlen(encodedBuffer);
	int null_terminator_position_last_element = 290 - elementsLeftInLastString;

	int j = 0;
	for (int i = 0; i < arraySize; ++i) {
		strncpy(encodedBufferArray[i], encodedBuffer + j, 290);
		if (i != arraySize - 1 ) {
			encodedBufferArray[i][290] = '\0';
		}else{
			encodedBufferArray[i][null_terminator_position_last_element] = '\0';
		}
		j = j + 290;
	}
	free((char *)encodedBuffer);
	uploadFile(encodedBufferArray, arraySize);
}

void startProgram() {

	printf("\nEnter the path to your local file that you want to upload: ");
	scanf("%s", LFILE);
	printf("\nEnter the absolute path of the file that will be created on the target: ");
	scanf("%s", RFILE);
	printf("\nWhich operating system is running on the target? windows(powershell)/linux(bash/sh/zsh) [windows/linux]: ");
	scanf("%s", operating_system);
	if (strcmp(operating_system, "linux") == 0){
		printf("\nSpecify a temp file that can be used to store the base64 encoded content on the target (will NOT be deleted automatically): ");
		scanf("%s", tempFile);
	}
	printf("\nWhich delay should the program use between each command in microseconds. 1000000 = 1 second. \nIf you don't know what to do, specify 50000 and then increase it if the commands are sent too fast (higher value = takes longer to upload): ");
	scanf("%d", &delayInMicroseconds);

	printf("\nStarting, please don't use the terminal until the process is done! Thank you!\n");
	const char *encodedBuffer = getAndEncodeFileContent(LFILE);	
	
	prepareEncodedBuffer(encodedBuffer);	
}

int main(int argc, char **argv) {
	if (argc == 2) {
		if (strcmp(*(argv+1), "-h") == 0) {
			printf("Usage, for example: \n%s /dev/pts/3", argv[0]);
			return 1;
		}
		strcpy(device_name, argv[1]);
		startProgram();
		return 0;
	}else{
		printf("Run \"%s -h\" for usage", *argv);
		return 1;
	}
}

